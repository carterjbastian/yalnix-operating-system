/*
 * Local Includes
 */
#include "kernel.h"
#include "hardware.h"
#include "linked_list.h"
#include "traps.h"
#include "tty.h"
#include "PCB.h"


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
  TracePrintf(1, "Start: SetKernelData \n");

  kernel_data_start = _KernelDataStart; 
  kernel_data_end = _KernelDataEnd;

  TracePrintf(1, "End: SetKernelData \n");
}

/* 
 * Completes init and sets up first userland process, 
 * and passes pointer to a UserContext
 *
 * PseudoCode:
 *  - Initalize global variables;
 */
void KernelStart(char *cmd_args[], 
                 unsigned int pmem_size,
                 UserContext *uctxt) { 

  TracePrintf(1, "Start: KernelStart \n");


  /*
   * =========================================
   *        Declare local variables
   * =========================================
   */

  int i;                                // Iterator for loops
  int base_frame_r1;                    // Lowest frame in region 1
  int top_frame_r1;                     // Highest frame in region 1

  PCB_t *idle_proc;                     // An idle process (the parent proc)
  int idle_stack_fnum1;                 // Number of idle's 1st stack frame
  int idle_stack_fnum2;                 // Number of idle's 2nd stack frame

  int lp_rc;                            // Return code of load program
  int arg_count;                        // The number of arguments passed into cmd_args

  /*
   * =========================================
   *    Initialize Global Kernel Variables,
   *      Privileged registers, and local
   *      variables for KernelStart.
   * =========================================
   */

  // Global Variable Initialziation
  kernel_brk = kernel_data_end;         // break starts as kernel_data_end
  available_process_id = 0;             // PIDs start at 0
  vm_en = 0;                            // VM is initially disabled

  // Physical Frame-related variables
  total_pframes = pmem_size / PAGESIZE;
  pframes_in_kernel = (VMEM_0_LIMIT >> PAGESHIFT);
  pframes_in_use = UP_TO_PAGE(kernel_brk) >> PAGESHIFT;     // Is this necessary forever?
  
  base_frame_r1 = DOWN_TO_PAGE(VMEM_1_BASE) >> PAGESHIFT;
  top_frame_r1 = UP_TO_PAGE(VMEM_1_LIMIT) >> PAGESHIFT;

  // Allocate kernel heap space for the Kernel's Process Queues
  ready_procs = (List *)malloc( sizeof(List) );
  blocked_procs = (List *)malloc( sizeof(List) );
  all_procs = (List *)malloc( sizeof(List) ); 
  dead_procs = (List *)malloc( sizeof(List) ); 

  /*
   * =========================================
   *    Create additional data structs 
   *    ( buffers, (todo: locks, cvars, pipes))
   * =========================================
   */
  ttys = (List *)malloc( sizeof(List) );
  for (i = 0; i < NUM_TERMINALS; i++) { 
    TTY_t *tmp = (TTY_t *)malloc( sizeof(TTY_t) );
    tmp->buffers = (List *)malloc( sizeof(List) );
    tmp->writers = (List *)malloc( sizeof(List) );
    tmp->readers = (List *)malloc( sizeof(List) );
    tmp->id = i;
    add_to_list(ttys, (void *)tmp, i); 
  }

  next_resource_id = 0;
  locks = (List *)malloc( sizeof(List) );
  cvars = (List *)malloc( sizeof(List) );
  

// Create the list of empty frames
    // NOTE: in FrameList, the number of the physical frame is
    //      stored in the id field of the node, NOT data.
  for (i = pframes_in_kernel; i < total_pframes; i++)
      add_to_list(&FrameList, (void *) NULL, i);


  /*
   * =========================================
   *    Set up the page tables for what's 
   *    already in use by the kernel.
   * =========================================
   */

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

    // So far, page is invalid, has read/write protections, and no pfn
    entry.valid = (u_long) 0x0;
    entry.prot = (u_long) (PROT_READ | PROT_WRITE);
    entry.pfn = (u_long) 0x0;

    // Add the page to the pagetable (accounting for 0-indexing)
    r1_pagetable[i - base_frame_r1] = entry;
  }


  /*
   * =========================================
   *    Configure the Privileged Registers
   *      and enable Virtual Memory  
   * =========================================
   */

  // Set up the page tables in the right places in the hardware regs
  WriteRegister(REG_PTBR0, (unsigned int) &r0_pagetable);
  WriteRegister(REG_PTBR1, (unsigned int) &r1_pagetable);
  WriteRegister(REG_PTLR0, (unsigned int) VMEM_0_PAGE_COUNT);
  WriteRegister(REG_PTLR1, (unsigned int) VMEM_1_PAGE_COUNT);

  // store interupt vector declared at top of file
  WriteRegister(REG_VECTOR_BASE, (unsigned int) &interrupt_vector);

  // Enable Virtual memory and flush the TLB
  WriteRegister(REG_VM_ENABLE, 1);
  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
  vm_en = 1; 
  TracePrintf(1, "Virtual Memory Enabled!\n");


  /*
   * =========================================
   *    Create the idle process
   * =========================================
   */

  // Create a shell process
  idle_proc = new_process(uctxt); // Allocates internally (see PCB.c)

  // Set up the user stack by allocating two frames
  // leaving the very top one empty (for the hardware)
  idle_stack_fnum1 = pop(&FrameList)->id;
  idle_stack_fnum2 = pop(&FrameList)->id;

  // Update the r1 page table with validity & pfn of idle's stack
  r1_pagetable[VMEM_1_PAGE_COUNT - 1].valid = (u_long) 0x1;
  r1_pagetable[VMEM_1_PAGE_COUNT - 1].pfn = (u_long) ((idle_stack_fnum1 * PAGESIZE) >> PAGESHIFT);
  r1_pagetable[VMEM_1_PAGE_COUNT - 2].valid = (u_long) 0x1;
  r1_pagetable[VMEM_1_PAGE_COUNT - 2].pfn = (u_long) ((idle_stack_fnum2 * PAGESIZE) >> PAGESHIFT);

  // flush the TLB region 1 since we changed r1_pagetable
  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);


  // Manually assign values to the PCB structure and UserContext for idle
  idle_proc->uc->pc = &DoIdle;                                  // pc points to idle function
  idle_proc->uc->sp = (void *) (VMEM_1_LIMIT - PAGESIZE);       // Manually created stack

  idle_proc->region1_pt = &r1_pagetable[0];
 
  // Allocate space for the KernelContext (in the idle proc)
  idle_proc->kc_p = (KernelContext *)malloc( sizeof(KernelContext) );
  idle_proc->kc_set = 1;    // we'll set it on first clock trap no matter what

  // The idle process has no parent
  idle_proc->parent = NULL;

  // Allocate idle's kernel stack page table
  idle_proc->region0_pt = (struct pte *)malloc( KS_NPG * sizeof(struct pte));

  // Copy idle's kernel stack page table
  memcpy((void *)idle_proc->region0_pt,
          (void *) &(r0_pagetable[KERNEL_STACK_BASE >> PAGESHIFT]), 
          KS_NPG * sizeof(struct pte));

  // Allocate idle's region 1 page table
  idle_proc->region1_pt = (struct pte *)malloc( VMEM_1_PAGE_COUNT * sizeof(struct pte));

  // Copy over idle's region 1 page table
  memcpy((void *)idle_proc->region1_pt, (void *) r1_pagetable, VMEM_1_PAGE_COUNT * sizeof(struct pte));


  /*
   * =========================================
   *    Create the init process
   * =========================================
   */

  // Make a shell process based on idle_proc
  PCB_t *init_proc = new_process(idle_proc->uc);

  // Allocate space for init's Kernel Stack and Region 1 ptes
  init_proc->region0_pt = (struct pte *)malloc(KS_NPG  * sizeof(struct pte));
  init_proc->region1_pt = (struct pte *)malloc(VMEM_1_PAGE_COUNT * sizeof(struct pte));
  bzero((char *)(init_proc->region1_pt), VMEM_1_PAGE_COUNT * sizeof(struct pte));

  // Initialize R1 memory to be invalid, with r/w protections, and no pfn 
  for (i = 0; i < VMEM_1_PAGE_COUNT; i++) {
    (*(init_proc->region1_pt + i)).valid = (u_long) 0x0;
    (*(init_proc->region1_pt + i)).prot = (u_long) (PROT_READ | PROT_WRITE);
    (*(init_proc->region1_pt + i)).pfn = (u_long) 0x0;
  }

  // Initialize kernel stack memory to be valid, with r/w protections, and a pfn
  for (i = 0; i < KS_NPG; i++) {
    (*(init_proc->region0_pt + i)).valid = (u_long) 0x1;
    (*(init_proc->region0_pt + i)).prot = (u_long) (PROT_READ | PROT_WRITE);
    (*(init_proc->region0_pt + i)).pfn = (u_long) ((pop(&FrameList)->id * PAGESIZE) >> PAGESHIFT);  
  }
  
  // Flush the TLB having updated pagetables
  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
  
  // init_proc is the first child of idle_proc!
  init_proc->parent = idle_proc;

  // Allocate idle_proc's child list and add init to it
  idle_proc->children = (List *) malloc(sizeof(List));
  add_to_list(idle_proc->children, (void *)init_proc, init_proc->proc_id);
 
  // Get the argument list and program name from args passed to KernelStart
  if (cmd_args[0] == '\0') {
    // Default arguments
    char *arglist[] = {"init", '\0'};
    char *progname = "./usr_progs/init";

    // Load the program from the text file
    if ((lp_rc = LoadProgram(progname, arglist, init_proc)) != SUCCESS) {
      TracePrintf(3, "LoadProgram failed with code %d\n", lp_rc);
      // Is there more that needs to be done here?
    }

  } else {
      arg_count = 0;
      // Count the number of command line arguments
      while (cmd_args[arg_count] != '\0') {
          arg_count++;
      }
      arg_count++; // Account for the null termination
      
      // Copy them into an argument list (null terminated)
      char *arglist[arg_count];
      for (i = 0; i < arg_count; i++) {
        arglist[i] = cmd_args[i];
      }

      // First argument is the program name
      char *progname = arglist[0];

    // Load the program from the text file
    if ((lp_rc = LoadProgram(progname, arglist, init_proc)) != SUCCESS) {
      TracePrintf(3, "LoadProgram failed with code %d\n", lp_rc);
      
      // Is there more that needs to be done here?
    }

  }


  /*
   * =========================================
   *    Kernel Bookkeeping for new processes
   * =========================================
   */

  // update the all_procs queue
  add_to_list(all_procs, (void *)idle_proc, idle_proc->proc_id);
  add_to_list(all_procs, (void *)init_proc, init_proc->proc_id);

  // We're going to start with idle and then switch to init
  add_to_list(ready_procs, (void *)init_proc, init_proc->proc_id);
  curr_proc = idle_proc; 

  // Copy idle's UserContext into the current UserContext
  memcpy(uctxt, idle_proc->uc, sizeof(UserContext));


  TracePrintf(1, "End: KernelStart\n");
} 



/*
 * function: MyKCSClone
 *  @kc_in:         Pointer to the kernel context to clone
 *  @curr_pcb_p:    The process with a kernel context to clone
 *  @next_pcb_p:    The process with no kernel context yet
 *
 *  Returns a pointer to the kernel context that's been cloned
 */
void *MyKCSClone(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p) {
    TracePrintf(1, "Start: MyKCSClone \n");

    PCB_t *curr = (PCB_t *) curr_pcb_p;
    PCB_t *next = (PCB_t *) next_pcb_p;

    int i;
    u_long old_pfns[KS_NPG];
    u_long old_validity[KS_NPG];
    unsigned int dest;
    unsigned int src;

    // Clone current kernel context into the next proc's kc pointer
    memcpy( (void *) (next->kc_p), (void *) (curr->kc_p), sizeof(KernelContext));

    // Temporarily map the kernel stack into frames in the current region 1
    for (i = 0; i < KS_NPG; i++) {
        old_pfns[i] = r0_pagetable[(KERNEL_STACK_BASE >> PAGESHIFT) - KS_NPG + i].pfn;
        old_validity[i] = r0_pagetable[(KERNEL_STACK_BASE >> PAGESHIFT) - KS_NPG + i].valid;

        r0_pagetable[(KERNEL_STACK_BASE >> PAGESHIFT) - KS_NPG + i].valid = (u_long) 0x1;
        r0_pagetable[(KERNEL_STACK_BASE >> PAGESHIFT) - KS_NPG + i].pfn = ((*(next->region0_pt + i)).pfn);
    }

    // Having changed the R0 pagetables, flush the TLB
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    // Copy the contents of the current kernel stack into the new kernel stack
    dest = ((KERNEL_STACK_BASE >> PAGESHIFT) - KS_NPG) << PAGESHIFT;
    src = KERNEL_STACK_BASE;
    memcpy( (void *)dest, (void *)src, KERNEL_STACK_MAXSIZE);

    // Restore the old kernel page mappings
    for (i = 0; i < KS_NPG; i++) {
        r0_pagetable[(KERNEL_STACK_BASE >> PAGESHIFT) - KS_NPG + i].pfn = old_pfns[i];
        r0_pagetable[(KERNEL_STACK_BASE >> PAGESHIFT) - KS_NPG + i].valid = old_validity[i];
    }

    // Having changed the R0 pagetables, flush the tlb again
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    TracePrintf(1, "End: MyKCSClone \n");
    return next->kc_p;
}


/*
 * function: MyKCSSwitch
 *  @kc_in:         Pointer to the kernel context of the current process
 *  @curr_pcb_p:    A (void *) pointer to the current process
 *  @next_pcb_p:    A (void *) pointer to the process to switch to
 *
 *  returns a pointer to the kernel context for the process being restored
 */
KernelContext *MyKCSSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p) {
    TracePrintf(1, "Start: MyKCSSwitch\n");

    int i, j;
    PCB_t *curr = (PCB_t *) curr_pcb_p;
    PCB_t *next = (PCB_t *) next_pcb_p;

    // Save the current process' kernel context
    if (curr != NULL)
      memcpy( (void *) (curr->kc_p), (void *)kc_in, sizeof(KernelContext));

    // If the next process has no kernel context, Clone the current one
    /*
     * WARNING: Could break if calling a new process out of an Exit call */
    if (next->kc_set == 0) {
      MyKCSClone(kc_in, curr_pcb_p, next_pcb_p);
      next->kc_set = 1;
      TracePrintf(2, "Copied Kernel Context into next\n"); 
    }

    // Save the current process' kernel stack
    if (curr != NULL) {
      memcpy((void *) curr->region0_pt,
              (void *) (&(r0_pagetable[KERNEL_STACK_BASE >> PAGESHIFT])),
              KS_NPG * (sizeof(struct pte)));
    }

    // Restore the next region's kernel stack
    memcpy((void *) (&(r0_pagetable[KERNEL_STACK_BASE >> PAGESHIFT])),
            (void *) next->region0_pt,
            KS_NPG * (sizeof(struct pte)));

    // Having changed the r0 page tables, flush the TLB
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    // Restore the next Process' Region 1
    WriteRegister(REG_PTBR1, (unsigned int) next->region1_pt);
 
    // Having changed the r1 page tables, flush the TLB
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_1);


    TracePrintf(1, "End: MyKCSSwitch\n");
    return next->kc_p;
}


/*
 * function: switch_to_next_available_proc
 *  @uc: Pointer to the UserContext to be saved for the current proccess
 *  @should_run_again: 1 if process should be put on ready queue, 0 otherwise
 *
 * Returns 0 on success, -1 on failure
 *
 * IMPT: Assumes ready_procs is not empty
 */
int switch_to_next_available_proc(UserContext *uc, int should_run_again){ 
  TracePrintf(1, "Start: switch_to_next_available_proc \n");

  // Put the old process on the ready queue if needed
  if (should_run_again) 
    add_to_list(ready_procs, (void *) curr_proc, curr_proc->proc_id);
 
  // Tries to switch to the next process in the ready queue
  PCB_t *next_proc = (pop(ready_procs))->data;
  if (perform_context_switch(curr_proc, next_proc, uc) != 0) {
    TracePrintf(1, "Context Switch failed\n");
    TracePrintf(1, "End: switch_to_next_available_proc \n");
    return ERROR;
  }

  TracePrintf(1, "End: switch_to_next_available_proc \n");
  return SUCCESS;
} 

/*
 * funtion: perform_context_switch
 *  @curr: pointer to the process block of the currently running process
 *  @next: pointer to the process block of the process to switch to
 *  @uc: pointer to the user context of the currently running process
 *
 *  Returns the return code of the KernelContextSwitch function
 */
int perform_context_switch(PCB_t *curr, PCB_t *next, UserContext *uc) {
    TracePrintf(1, "Start: perform_context_switch\n");

    int rc;

    // Store the user context of currently running process
    if (curr != NULL)
      memcpy((void *)curr->uc, (void *) uc, sizeof(UserContext) );      

    // Store the next process' user context in the uc variable
    memcpy((void *) uc, (void *) next->uc, sizeof(UserContext) );

    // Update the current process global variable
    curr_proc = next;
    
    // Do the switch with magic function
    rc = KernelContextSwitch(MyKCSSwitch, (void *) curr, (void *) next);

    // Store the currently running process' user context in uc variable
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
