/*
 * Local Includes
 */
#include "kernel.h"
#include "hardware.h"
#include "linked_list.h"
#include "traps.h"
#include "PCB.h"

// Private Function Declarations
int first_context_switch(PCB_t *curr, PCB_t *next, char *fname, char *args[]);

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
 * Completes init and sets up first userland process, 
 * and passes pointer to a UserContext
 */
void KernelStart(char *cmd_args[], 
                 unsigned int pmem_size,
                 UserContext *uctxt) { 

  // the break is initially the kernel_data_end
  kernel_brk = kernel_data_end;

  // set up initial global variables, declare a local iterator
  int i;
  available_process_id = 0;
  vm_en = 0;

  // store interupt vector declared at top of file
  WriteRegister(REG_VECTOR_BASE, (unsigned int) &interrupt_vector);
  TracePrintf(1, "Wrote the interrupt vector\n");

  // Set up Frame-related variables
  total_pframes = pmem_size / PAGESIZE;
  pframes_in_kernel = (VMEM_0_LIMIT >> PAGESHIFT);
  pframes_in_use = UP_TO_PAGE(kernel_brk) >> PAGESHIFT;
  int base_frame_r1 = DOWN_TO_PAGE(VMEM_1_BASE) >> PAGESHIFT;
  int top_frame_r1 = UP_TO_PAGE(VMEM_1_LIMIT) >> PAGESHIFT;

  // Allocate kernel heap space for the Kernel's linked lists / queues
  ready_procs = (List *)malloc( sizeof(List) );
  blocked_procs = (List *)malloc( sizeof(List) );
  all_procs = (List *)malloc( sizeof(List) ); 
  dead_procs = (List *)malloc( sizeof(List) ); 

  // Create the list of empty frames (storing the frame number as
  // the Node's ID)
  for (i = pframes_in_kernel; i < total_pframes; i++)
      add_to_list(&FrameList, (void *) NULL, i);

  // Set up page tables for what's already in use.
  for (i = 0; i < pframes_in_kernel; i++) {
    // Create an empty page table entry structure
    struct pte entry;
    
    // Assign validity
    if ((i < pframes_in_use) || (i >= (KERNEL_STACK_BASE >> PAGESHIFT)))
       entry.valid = (u_long) 0x1;
    else
       entry.valid = (u_long) 0x0; 
    
    // Assign permissions
    if (i < (((unsigned int)kernel_data_start) >> PAGESHIFT))
      entry.prot = (u_long) (PROT_READ | PROT_EXEC); // exec and read protections
    else
      entry.prot = (u_long) (PROT_READ | PROT_WRITE); // read and write protections
    
    // Assign pfn
    entry.pfn = (u_long) ((PMEM_BASE + (i * PAGESIZE)) >> PAGESHIFT);
    r0_pagetable[i] = entry;
  }

  // Create pte items for the pages in R1 (pre-allocating pages)
  for (i = base_frame_r1; i < top_frame_r1; i++) {
    struct pte entry; // New pte entry
    // So far this page is invalid, has read/write protections, and has no
    // physical frame.
    entry.valid = (u_long) 0x0;
    entry.prot = (u_long) (PROT_READ | PROT_WRITE);
    entry.pfn = (u_long) 0x0;
    // Actually add the page to the pagetable (accounting for 0-indexing)
    r1_pagetable[i - base_frame_r1] = entry;
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

  /* create idle process (see PCB.c) */
  PCB_t *idle_proc = new_process(uctxt); // Allocates internally

  /* Change the pc and sp of the new process' UserContext */
  idle_proc->uc->pc = &DoIdle; // pc points to idle function

  // Allocate two physical frames for the idle process' stack
  // and leave the very top one empty (so the hardware doesn't throw a fit)
  ListNode *free_frame = pop(&FrameList);
  ListNode *free_frame2 = pop(&FrameList);
  int idle_stack_fnum = free_frame->id;
  int idle_stack_fnum2 = free_frame2->id;

  // Update the r1 page table with validity & pfn
  r1_pagetable[VMEM_1_PAGE_COUNT - 1].valid = (u_long) 0x1;
  r1_pagetable[VMEM_1_PAGE_COUNT - 1].pfn = (u_long) ((idle_stack_fnum * PAGESIZE) >> PAGESHIFT);
  r1_pagetable[VMEM_1_PAGE_COUNT - 2].valid = (u_long) 0x1;
  r1_pagetable[VMEM_1_PAGE_COUNT - 2].pfn = (u_long) ((idle_stack_fnum2 * PAGESIZE) >> PAGESHIFT);

  // Assign the new process' context's stack pointer to the top of the stack
  idle_proc->uc->sp = (void *) (VMEM_1_LIMIT - PAGESIZE);

  /* Store pointers to the first page table entries in the PCB instance */
  idle_proc->region0_pt = &r0_pagetable[KERNEL_STACK_BASE >> PAGESHIFT];
  idle_proc->region1_pt = r1_pagetable;

  // flush the TLB region 1 since we changed r1_pagetable
  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);

  /* Internal Book Keeping with new process */ 
  add_to_list(all_procs, (void *)idle_proc, idle_proc->proc_id);    // update processes list with idle process
  curr_proc = idle_proc;

  TracePrintf(1, "Starting to load the program in from mem\n");

  // Make the init process
  PCB_t *init_proc = new_process(idle_proc->uc);
  add_to_list(ready_procs, (void *)init_proc, init_proc->proc_id);
  add_to_list(all_procs, (void *)init_proc, init_proc->proc_id);

  // Allocate space for init's Kernel Stack and Region 1 ptes
  init_proc->region0_pt = (struct pte *)malloc((KERNEL_STACK_MAXSIZE / PAGESIZE)  * sizeof(struct pte));
  init_proc->region1_pt = (struct pte *)malloc(VMEM_1_PAGE_COUNT * sizeof(struct pte));

  // Initialize this memory space
  for (i = 0; i < VMEM_1_PAGE_COUNT; i++) {
    
    (*(init_proc->region1_pt + i)).valid = (u_long) 0x0;
    (*(init_proc->region1_pt + i)).prot = (u_long) (PROT_READ | PROT_WRITE);
  }
  for (i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++) {
    (*(init_proc->region0_pt + i)).valid = (u_long) 0x0;
    (*(init_proc->region0_pt + i)).prot = (u_long) (PROT_READ | PROT_WRITE);
    (*(init_proc->region0_pt + i)).pfn = (u_long) ((pop(&FrameList)->id * PAGESIZE) >> PAGESHIFT);
  }

  char *arglist[] = {"init", '\0'};
  char *progname = "./usr_progs/init";
  //int c_switch_rc = first_context_switch(curr_proc, init_proc, progname, arglist);
  
  int lp_rc = LoadProgram(progname, arglist, init_proc);

  
  TracePrintf(1, "Made it to the end of KernelStart\n");



  /* 
   * Copy the newly created idle process' UserContext into the current
   * UserContext with memcpy.
   */
  memcpy(uctxt, idle_proc->uc, sizeof(UserContext));

} 

// idle function for testing
void DoIdle() {
  while (1) {
    TracePrintf(1, "\tDoIdle\n");
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
    unsigned int bottom_page = VMEM_0_BASE >> PAGESHIFT;
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
    
    // Set the new kernel break
    kernel_brk = addr;

    // Flush the TLB register
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    TracePrintf(1, "Finishing SetKernelBrk\n");
    return 0;
  }
}


KernelContext *MyKCS(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p) {
    TracePrintf(1, "\t==> MyKCS\n");
    int i, j;
    PCB_t *curr = (PCB_t *) curr_pcb_p;
    PCB_t *next = (PCB_t *) next_pcb_p;
    ((PCB_t *)curr_pcb_p)->kc = kc_in; // Store the kernel context
    

    // Restore the next_pcb_p's kernel stack
    for (i = (KERNEL_STACK_BASE >> PAGESHIFT); 
        i < (DOWN_TO_PAGE(KERNEL_STACK_LIMIT) >> PAGESHIFT);
            i++) {
        j = i - (KERNEL_STACK_BASE >> PAGESHIFT);
        r0_pagetable[i] = *(next->region0_pt + j);
    }


    // Restore the next proc's region 1 page table
    for (i = 0; i < VMEM_1_PAGE_COUNT; i++)
        r1_pagetable[i] = *(next->region1_pt + i);
   
    // Flush the TLB
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

    TracePrintf(1, "\t==> MyKCS done\n");
    return next->kc;
}


int perform_context_switch(PCB_t *curr, PCB_t *next) {
    int rc;

    // Store current proc's region 0 and 1 pointers
    curr->region0_pt = &r0_pagetable[KERNEL_STACK_BASE >> PAGESHIFT];
    curr->region1_pt = &r1_pagetable[0];

    // Put the old process on the ready queue
    add_to_list(ready_procs, (void *) curr, curr->proc_id);

    // Update the current process global variable
    curr_proc = next;

    // Do the switch with magic function
    rc = KernelContextSwitch(MyKCS, (void *) curr, (void *) next);

    return rc;
}

int first_context_switch(PCB_t *curr, PCB_t *next, char *fname, char *args[]) {
    TracePrintf(1, "\t==> first_context_switch\n");
    int rc;

    // Store current proc's region 0 and 1 pointers
    curr->region0_pt = &r0_pagetable[KERNEL_STACK_BASE >> PAGESHIFT];
    curr->region1_pt = &r1_pagetable[0];

    // Put the old process on the ready queue
    add_to_list(ready_procs, (void *) curr, curr->proc_id);

    // Update the current process global variable
    curr_proc = next;

    // Do the switch with magic function
    rc = KernelContextSwitch(MyKCS, (void *) curr, (void *) next);
    if (rc != 0) {
        TracePrintf(3, "\t!!! KernelContextSwitch Failed\n");
    } else {
        rc = LoadProgram(fname, args, next);
        if (rc == ERROR)
            TracePrintf(3, "\t!!! LoadProgram Failed\n");
    }
    TracePrintf(1, "\t==> first_context_switch complete\n");
    return rc;
}
