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
int ks_npg = KERNEL_STACK_MAXSIZE / PAGESIZE;

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
  TracePrintf(1, "Start: SetKernelData \n");
  kernel_data_start = _KernelDataStart; 
  kernel_data_end = _KernelDataEnd;
  TracePrintf(1, "End: SetKernelData \n");
}

/* 
 * Completes init and sets up first userland process, 
 * and passes pointer to a UserContext
 */
void KernelStart(char *cmd_args[], 
                 unsigned int pmem_size,
                 UserContext *uctxt) { 
  TracePrintf(1, "Start: KernelStart \n");

  // the break is initially the kernel_data_end
  kernel_brk = kernel_data_end;

  // set up initial global variables, declare a local iterator
  int i;
  available_process_id = 0;
  vm_en = 0;

  // store interupt vector declared at top of file
  WriteRegister(REG_VECTOR_BASE, (unsigned int) &interrupt_vector);

  //r1_pagetable = (struct pte *)malloc(VMEM_1_PAGE_COUNT * sizeof(struct pte));
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
  
  
  // Allocate space for the KernelContext (in the idle proc)
  idle_proc->kc_p = (KernelContext *)malloc( sizeof(KernelContext) );
  idle_proc->kc_set = 1; // we'll set it on first clock trap no matter what
  /* Store pointers to the first page table entries in the PCB instance */
  idle_proc->region0_pt = (struct pte *)malloc( ks_npg * sizeof(struct pte));
  memcpy((void *)idle_proc->region0_pt, (void *) &(r0_pagetable[KERNEL_STACK_BASE >> PAGESHIFT]), ks_npg * sizeof(struct pte));

  idle_proc->region1_pt = (struct pte *)malloc( VMEM_1_PAGE_COUNT * sizeof(struct pte));
  memcpy((void *)idle_proc->region1_pt, (void *) r1_pagetable, VMEM_1_PAGE_COUNT * sizeof(struct pte));
  //idle_proc->region1_pt = r1_pagetable;
  

  // flush the TLB region 1 since we changed r1_pagetable
  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);

  /* Internal Book Keeping with new process */ 
  add_to_list(all_procs, (void *)idle_proc, idle_proc->proc_id);    // update processes list with idle process
  curr_proc = idle_proc;
  

  // Make the init process
  PCB_t *init_proc = new_process(idle_proc->uc);


  // Allocate space for init's Kernel Stack and Region 1 ptes
  init_proc->region0_pt = (struct pte *)malloc((KERNEL_STACK_MAXSIZE / PAGESIZE)  * sizeof(struct pte));
  init_proc->region1_pt = (struct pte *)malloc(VMEM_1_PAGE_COUNT * sizeof(struct pte));

  TracePrintf(1, "Initializing r1 mem\n");

  // Initialize this memory space
  for (i = 0; i < VMEM_1_PAGE_COUNT; i++) {
    
    (*(init_proc->region1_pt + i)).valid = (u_long) 0x0;
    (*(init_proc->region1_pt + i)).prot = (u_long) (PROT_READ | PROT_WRITE);
  }
  TracePrintf(1, "Initializing r0 mem\n");

  for (i = 0; i < KERNEL_STACK_MAXSIZE / PAGESIZE; i++) {
    (*(init_proc->region0_pt + i)).valid = (u_long) 0x1;
    (*(init_proc->region0_pt + i)).prot = (u_long) (PROT_READ | PROT_WRITE);
    (*(init_proc->region0_pt + i)).pfn = (u_long) ((pop(&FrameList)->id * PAGESIZE) >> PAGESHIFT);  
  }
  
  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
  
  TracePrintf(1, "Starting to load the program in from mem\n");
  char *arglist[] = {"init", '\0'};
  char *progname = "./usr_progs/init";
  
  
  int lp_rc = LoadProgram(progname, arglist, init_proc);
  TracePrintf(1, "LoadProgram returned with code %d\n", lp_rc);
  add_to_list(ready_procs, (void *)init_proc, init_proc->proc_id);
  add_to_list(all_procs, (void *)init_proc, init_proc->proc_id);

  /* 
   * Copy the newly created idle process' UserContext into the current
   * UserContext with memcpy.
   */
  memcpy(uctxt, idle_proc->uc, sizeof(UserContext));

  TracePrintf(1, "End: KernelStart\n");
} 

void *MyKCSClone(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p) {
    TracePrintf(1, "Start: MyKCSClone \n");

    PCB_t *curr = (PCB_t *) curr_pcb_p;
    PCB_t *next = (PCB_t *) next_pcb_p;

    int i;
    u_long old_pfns[ks_npg];
    unsigned int dest;
    unsigned int src;

    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
    // Temporarily map the kernel stack into frames in the current 
    for (i = 0; i < ks_npg; i++) {
        old_pfns[i] = r0_pagetable[(KERNEL_STACK_BASE >> PAGESHIFT) - ks_npg + i].pfn;
        r0_pagetable[(KERNEL_STACK_BASE >> PAGESHIFT) - ks_npg + i].valid = (u_long) 0x1;
        r0_pagetable[(KERNEL_STACK_BASE >> PAGESHIFT) - ks_npg + i].pfn = ((*(next->region0_pt + i)).pfn);
    }
    TracePrintf(1, "Temporarily mapped kstack frames into current mapping\n");
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
    TracePrintf(1, "Flushed the TLB\n");

    // Copy the contents of the current kernel stack into the new kernel stack
    dest = ((KERNEL_STACK_BASE >> PAGESHIFT) - ks_npg) << PAGESHIFT;
    src = KERNEL_STACK_BASE;
    memcpy( (void *)dest, (void *)src, KERNEL_STACK_MAXSIZE);
    TracePrintf(1, "Copied the kstack contents\n");

    // Restore the old kernel page mappings
    for (i = 0; i < ks_npg; i++) {
        r0_pagetable[(KERNEL_STACK_BASE >> PAGESHIFT) - ks_npg + i].pfn = old_pfns[i];
        r0_pagetable[(KERNEL_STACK_BASE >> PAGESHIFT) - ks_npg + i].prot = (u_long)0x0;
    }
    TracePrintf(1, "Restored old kernel page mappings\n");
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

    TracePrintf(1, "End: MyKCSClone \n");
    return next->kc_p;
}

KernelContext *MyKCSSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p) {
    TracePrintf(1, "Start: MyKCSSwitch\n");
    int i, j;
    PCB_t *curr = (PCB_t *) curr_pcb_p;
    PCB_t *next = (PCB_t *) next_pcb_p;

    // Copy the kernel context to save it
    memcpy( (void *) (curr->kc_p), (void *)kc_in, sizeof(KernelContext));

    if (next->kc_set == 0) { 
      next->kc_p = curr->kc_p;
      MyKCSClone(kc_in, curr_pcb_p, next_pcb_p);
      next->kc_set = 1;
      TracePrintf(1, "Copied Kernel Context into next\n"); 
    }

    // Store the current region's kernel stack with a memcpy
    memcpy((void *) curr->region0_pt,
            (void *) (&(r0_pagetable[KERNEL_STACK_BASE >> PAGESHIFT])),
            ks_npg * (sizeof(struct pte)));

    TracePrintf(1, "Stored current procs kernel stack\n");
    //Restore the next region's kernel stack
    memcpy((void *) (&(r0_pagetable[KERNEL_STACK_BASE >> PAGESHIFT])),
            (void *) next->region0_pt,
            ks_npg * (sizeof(struct pte)));
    TracePrintf(1, "Restored next procs kernel stack\n");
    // Restore the next region's Region 1 by setting the pointer to the
    // page table
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
    WriteRegister(REG_PTBR1, (unsigned int) next->region1_pt);
    TracePrintf(1, "Restored next procs r1\n");
    // Flush the TLB
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);

    TracePrintf(1, "End: MyKCSSwitch\n");
    return next->kc_p;
}


// IMPT: assumes ready_procs.count > 0
int switch_to_next_available_proc(UserContext *uc, int should_run_again){ 
    TracePrintf(1, "Start: switch_to_next_available_proc \n");
  // Put the old process on the ready queue if needed
  if (should_run_again) { 
    add_to_list(ready_procs, (void *) curr_proc, curr_proc->proc_id);
  } 
  
  PCB_t *next_proc = pop(ready_procs)->data;
  if (perform_context_switch(curr_proc, next_proc, uc) != 0) {
    TracePrintf(1, "Context Switch failed\n");
    TracePrintf(1, "End: switch_to_next_available_proc \n");
    return -1;
  }
  
  TracePrintf(1, "End: switch_to_next_available_proc \n");
  return 0;
} 

int perform_context_switch(PCB_t *curr, PCB_t *next, UserContext *uc) {
  TracePrintf(1, "Start: perform_context_switch\n");
    int rc;
      
    memcpy((void *)curr->uc, (void *) uc, sizeof(UserContext) );      
    
    // Store current proc's region 0 and 1 pointers
    curr->region1_pt = &r1_pagetable[0];
      
    // Update the current process global variable
    curr_proc = next;
    memcpy((void *) uc, (void *) next->uc, sizeof(UserContext) );

    // Do the switch with magic function
    rc = KernelContextSwitch(MyKCSSwitch, (void *) curr, (void *) next);
        
    memcpy((void *)uc, (void *) curr_proc->uc, sizeof(UserContext) );

    TracePrintf(1, "End: perform_context_switch\n");
    return rc;
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
  TracePrintf(1, "Start: SetKernelBrk\n");
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

    TracePrintf(1, "End: SetKernelBrk\n");
    return 0;
  }
}

