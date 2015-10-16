#include "kernel.h"
#include "hardware.h"
#include "linked_list.h"
#include "traps.h"
#include "pcb.h"

// Statically declared interrupt_vector
void (*interrupt_vector[8]) = {
  HANDLE_TRAP_KERNEL, 
  HANDLE_TRAP_CLOCK,
  HANDLE_TRAP_ILLEGAL,
  HANDLE_TRAP_MEMORY,
  HANDLE_TRAP_MATH,
  HANDLE_TRAP_TTY_RECEIVE,
  HANDLE_TRAP_TTY_TRANSMIT,
  HANDLE_TRAP_DISK
};

// Statically declared frame table list
struct list FrameList;

// Statically declared page table arrays
struct pte r0_pagetable[VMEM_0_PAGE_COUNT];
struct pte r1_pagetable[VMEM_1_PAGE_COUNT];

/*

  Tells kernel some basic params about data segment.
  KernelDataEnd: lowest addr not in use by kernel's instructions 
  and global data at boot time. 
  KernelDataStart: lowest addr used by data data segment. In building
  initial page entries for kernel, the page table entries covering up to
  but not including this point should be built with protection PROT_READ
  and PROT_EXEC set. Otherwise some be build w/ PROT_READ and PROT_WRITE.
 
*/
void SetKernelData(void * _KernelDataStart, void *_KernelDataEnd) { 
  kernel_data_start = _KernelDataStart; 
  kernel_data_end = _KernelDataEnd;
}

/* 
Completes init and sets up first userland process, 
and passes pointer to a UserContext
*/
void KernelStart(char *cmd_args[], 
                 unsigned int pmem_size,
                 UserContext *uctxt) { 

  // the break is initially the kernel_data_end
  kernel_brk = kernel_data_end;

  int i;

  available_process_id = 0;
  vm_en = 0;
  TracePrintf(1, "Bout to start\n");

  // store interupt vector declared at top of file
  WriteRegister(REG_VECTOR_BASE, (unsigned int) &interrupt_vector);
  TracePrintf(1, "Wrote the interrupt vector\n");

  // Frame-related variables
  total_pframes = pmem_size / PAGESIZE;
  pframes_in_kernel = (VMEM_0_LIMIT >> PAGESHIFT);
  pframes_in_use = UP_TO_PAGE(kernel_brk) >> PAGESHIFT;
  int base_frame_r1 = DOWN_TO_PAGE(VMEM_1_BASE) >> PAGESHIFT;
  int top_frame_r1 = DOWN_TO_PAGE(VMEM_1_LIMIT) >> PAGESHIFT;
  // Create the list of empty frames (storing the frame number as
  // the Node's ID)
  for (i = pframes_in_kernel; i < total_pframes; i++)
      add_data(&FrameList, (void *) NULL, i);

  // Set up page tables for what's already in use.
  for (i = 0; i < pframes_in_kernel; i++) {
    struct pte entry;
    //entry.valid = ((i < pframes_in_use) ? (u_long) 0x1 : (u_long) 0x0);
    entry.valid = (u_long) 0x1;
    if ((i < pframes_in_use) || (i >= (KERNEL_STACK_BASE >> PAGESHIFT)))
       entry.valid = 0x1;
    else
       entry.valid = 0x0; 
    
    if (i < (((unsigned int)kernel_data_start) >> PAGESHIFT))
      entry.prot = (u_long) (PROT_READ | PROT_EXEC); // exec and read protections
    else
      entry.prot = (u_long) (PROT_READ | PROT_WRITE); // read and write protections
    
    entry.pfn = (u_long) ((PMEM_BASE + (i * PAGESIZE)) >> PAGESHIFT);
    r0_pagetable[i] = entry;
  }

  // Create pte items for the pages in R1
  for (i = base_frame_r1; i < top_frame_r1; i++) {
    struct pte entry;
    entry.valid = (u_long) 0x0;
    entry.prot = (u_long) (PROT_READ | PROT_WRITE);
    entry.pfn = (u_long) 0x0;
  }

  // Set up the page tables in the right places with privileged hardware
  // registers  
  WriteRegister(REG_PTBR0, (unsigned int) &r0_pagetable);
  WriteRegister(REG_PTBR1, (unsigned int) &r1_pagetable);
  WriteRegister(REG_PTLR0, (unsigned int) VMEM_0_PAGE_COUNT);
  WriteRegister(REG_PTLR1, (unsigned int) VMEM_1_PAGE_COUNT);

  // enable virtual memory
  WriteRegister(REG_VM_ENABLE, 1);
  TracePrintf(1, "Virtual Memory Enabled!\n");
  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
  vm_en = 1; 

  processes = (struct list *) malloc(sizeof(struct list));

  // create idle process (see PCB.c)
  struct PCB *idle_proc = (struct PCB *) malloc(sizeof(struct PCB));
  idle_proc = new_process(uctxt);
  add_data(processes, (void *)idle_proc, 0);
  curr_proc = idle_proc;

  curr_proc->uc->pc = &DoIdle;
  TracePrintf(1, "Made it to the end of KernelStart\n");
} 

// idle function for testing
void DoIdle() {
  while (1) {
    TracePrintf(1, "DoIdle\n");
    Pause();
  } 
} 

/*

 Used to increase break.
 Needs to keep track of KernalDataEnd.
 addr: indicates lowest location not used (not yet needed by malloc) 
 in our kernel.
 
 SetKernelBrk should keep track of highest addr passed on any call 
 to this function. Then we can use highest addr to determine total
 amount of memory in use by kernel when virtual memory is enabled.

*/
int SetKernelBrk(void * addr) { 
  int i;  
  TracePrintf(1, "Inside SetKernelBrk\n");
  // Check that the requested address is within the proper bounds of
  // where the break should ever be allowed to be
  if ((unsigned int) addr > (unsigned int)KERNEL_STACK_BASE || addr < kernel_data_start) {
    TracePrintf(1, "SetKernelBrk Error: address requested not in bounds\n");
    return -1;
  }

  // Before enabling virtual memory
  if (!vm_en) {
      // Just keep track of the highest requested address
      if ((unsigned int)addr > (unsigned int)kernel_brk)
          kernel_brk = addr;
      return 0;
  // After enabling virtual memory
  } else {
    // Get the page values of the integers
    unsigned int bottom_page = VMEM_BASE >> PAGESHIFT;
    unsigned int addr_page = DOWN_TO_PAGE(addr) >> PAGESHIFT;
    unsigned int bottom_of_stack = DOWN_TO_PAGE(KERNEL_STACK_BASE) >> PAGESHIFT;

    // Loop through each vm page in the kernel up to the new break
    for (i = bottom_page; i <= addr_page; i++) {
        // Update pagetable if a page that should be valid is not
        if (r0_pagetable[i].valid != 0x1) 
            r0_pagetable[i].valid = (u_long) 0x1; // Update the page table entry
        
    }
    // Loop through what should be unallocated memory
    for (i = addr_page + 1; i < bottom_of_stack; i++) {
        // If the page is allocated, we need to free it
        if (r0_pagetable[i].valid == 0x1) 
            r0_pagetable[i].valid = (u_long) 0x0; // Update the page table
    }
    kernel_brk = addr;
    // Flush the TLB register
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);
    return 0;
  }
} 
