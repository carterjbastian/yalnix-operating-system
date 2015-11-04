/*
 * System includes
 */
#include <hardware.h>
#include <string.h>

/*
 * Local Includes
 */
#include "../kernel.h"
#include "../PCB.h"
#include "syscalls.h"

/*
 * Function: Yalnix_Fork
 *  @uc: The user context of the current process passed into the trap handler
 *  
 *  PseudoCode:
 *
 *  Returns:
 *   0 to the child on success
 *   The process ID of the child to the parent on success
 *   ERROR to the parent on failure (without creating the child)
 */
int Yalnix_Fork(UserContext *uc) {
  TracePrintf(1, "Start: Yalnix_Fork()\n");


  /* Local variables */
  PCB_t *child;                     // Process Control Block of the Child
  PCB_t *parent;                    // Process Control Block of the parent
  unsigned int child_pid;           // Process ID of the child process

  int pfn_temp;                     // A variable to hold the pfn most recently
                                    // popped from FrameList

  int dest_page;                    // Page in r0 for mapping a frame from child
                                    // process to a page currently in memory
  int dest_old_pfn;                 // pfn of dest_page before being mapped elsewhere
  int dest_old_valid;               // validity of dest_page before being mapped
  unsigned int dest;                // Address to copy to
  unsigned int src;                 // Address to copy from
  int retval;                       // Return value
  int i;                            // Iterator for loops


  /* Store the the UserContext of the parent process */  
  parent = curr_proc;
  memcpy((void *) parent->uc, (void *) uc, sizeof(UserContext));

  /* 
   * Create a new shell process
   */

  // Create a new Process Controll Block shell for the child
  child = new_process(parent->uc);

  // Copy the user context completely into the child
  memcpy((void *) child->uc, (void *) parent->uc, sizeof(UserContext));

  // Dynamically allocate space for child's kernel stack and region 1 PTEs
  child->region0_pt = (struct pte *) malloc(KS_NPG * sizeof(struct pte));
  child->region1_pt = (struct pte *) malloc(VMEM_1_PAGE_COUNT * sizeof(struct pte));
  
  if (child->region0_pt == NULL || child->region1_pt == NULL) {
    TracePrintf(3, "Failed to allocate kernel space for child process' pagetables\n");
    return(ERROR);
  }

  // Set up remaining PCB variables of child process
  child->heap_base_page = parent->heap_base_page;
  child->brk_addr = parent->brk_addr;


  /* 
   * Create the PageTables for the child process 
   */
  TracePrintf(1, "Creating the Page Table Mappings\n");
  // First copy the pagetables exactly for permissions and validity
  memcpy((void *) child->region0_pt, 
      (void *) parent->region0_pt, 
      (KS_NPG * sizeof(struct pte)) );

  memcpy((void *) child->region1_pt,
      (void *) parent->region1_pt,
      (VMEM_1_PAGE_COUNT * sizeof(struct pte)) );


  // Allocate new physical frames for each page in the kernel stack
  for (i = 0; i < KS_NPG; i++) {
    /* 
     * WARNING: this code is inefficient. Figure out a way to remove calls to
     * count_items
     */
    if (count_items(&FrameList) <= 0) {
      TracePrintf(1, "Not enough frames for child process' kernel stack\n");
      return(ERROR);
    }
    pfn_temp = pop(&FrameList)->id;
    (*(child->region0_pt + i)).pfn = FNUM_TO_PFN(pfn_temp);
  }

  // Allocate new physical frames for each valid page in region 1
  for (i = 0; i < VMEM_1_PAGE_COUNT; i++) {
    // Is this a valid page in region 1?
    if ( (*(child->region1_pt + i)).valid == (u_long) 0x1) {
      // If so give it a new physical frame

      /* WARNING: see warning above on inefficiency */
      if (count_items(&FrameList) <= 0) {
        TracePrintf(1, "Not enough frames for child process' region 1\n");
        return(ERROR);
      }
      pfn_temp = pop(&FrameList)->id;
      (*(child->region1_pt + i)).pfn = FNUM_TO_PFN(pfn_temp);
    
    } else {
      // If not, ensure its pfn is 0
      (*(child->region1_pt + i)).pfn = (u_long) 0x0;
    }
  }
  TracePrintf(1, "Finished Allocating Frames\n");

  /* 
   * Manually Copy all of the contents of the parent's memory into
   *   child process
   */
  // Set up a mapping to the page direclty beneath the ks for mapping
  dest_page = (KERNEL_STACK_BASE >> PAGESHIFT) - 1;
  
  // Store old settings of that entry in the page table
  dest_old_pfn = r0_pagetable[dest_page].pfn;
  dest_old_valid = r0_pagetable[dest_page].valid;

  // Make the destination page valid
  r0_pagetable[dest_page].valid = (u_long) 0x1;

  // The dest is the virtual address of the dest_page in memory
  dest = dest_page << PAGESHIFT;
  
  
  TracePrintf(1, "About to copy kernel pages\n");
  // Loop through the pages in the kernel stack, copying each to the dest page
  for (i = 0; i < KS_NPG; i++) {
    // The src is the virtual address of this page of the kernel stack
    src = KERNEL_STACK_BASE + (i * PAGESIZE);

    // Set up the destination page to have the pfn of corresponding page in
    // child proc's page table
    r0_pagetable[dest_page].pfn = (*(child->region0_pt + i)).pfn;
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    // Copy the memory contents from the parent's frame for the page to the
    // child's frame for the page (aka dest)
    memcpy((void *) dest, (void *) src, PAGESIZE);
  }

  TracePrintf(1, "About to copy Region 1 Pages\n");
  // Loop through the pages in region 1, copying each valid page to dest
  for (i = 0; i < VMEM_1_PAGE_COUNT; i++) {
    // Only copy this if it's a valid page in memory for the parent
    if ( (*(child->region1_pt + i)).valid == 0x1 ) {
      // The src is the virtual address in memory of the page in r1
      src = VMEM_1_BASE + (i * PAGESIZE);

      // Set up destination page for have pfn for page in child proc's region1
      // page table.
      r0_pagetable[dest_page].pfn = (*(child->region1_pt + i)).pfn;
      WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

      // Copy the memory contents from the parent's frame for the page to the
      // child's frame for page (aka dest)
      memcpy((void *)dest, (void *) src, PAGESIZE);
    }
  }

  TracePrintf(1, "About to restore the PTE for the destination page\n");
  // Restore old PTE for destination page
  r0_pagetable[dest_page].valid = dest_old_valid;
  r0_pagetable[dest_page].pfn = dest_old_pfn;
  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);


  /*
   * Kernel Bookkeeping
   */
  // Create (if necessary) and add to the parent's list of children
  if (parent->children == NULL)
      parent->children = (List *) malloc( sizeof(List) );

  add_to_list(parent->children, (void *)child, child->proc_id);

  // Create parent's list of exited children if necessary
  if (parent->exited_children == NULL)
    parent->exited_children = (List *) malloc( sizeof(List) );

  // Update the kernel queues
  add_to_list(all_procs, (void *)child, child->proc_id);
  add_to_list(ready_procs, (void *)parent, parent->proc_id);


  /*
   * Context Switch to Child Process
   */
  if (perform_context_switch(parent, child, uc) != 0) {
    TracePrintf(3, "Context Switch to child process failed.\n");
    return(ERROR);
  }
  
  /*
   * Return the appropriate value.
   * By now, we might be in the same place in a different process,
   * so we need to return differently in each case.
   */
    if (curr_proc == parent) {
    TracePrintf(1, "Returning to the parent function\n");
    TracePrintf(1, "End: Yalnix_Fork()\n"); 
    return child->proc_id;
  } else if (curr_proc == child) {
    TracePrintf(1, "Returning to the child function\n");
    TracePrintf(1, "End: Yalnix_Fork()\n");
    return 0;
  } else {
    TracePrintf(1, "Unrecognized curr process returning from context_switch\n");
    return ERROR;
  }

} 





int Yalnix_Exec(char *filename, char **argvec) { 
  /* 
     rough draft:
     
     ? PCB_t *new_proc = new_process(curr_proc->uc) ?
     
     // this might do more than we need though... 
     // walkthrough seems to imply that it does, 
     // not sure though
     load_program(filename, argvec, new_proc); 

     we don't want perform_context_switch because exec 
     is supposed to literally replace the running process
     with the new one...

  */
}


void Yalnix_Exit(int status) { 
  // release_resources(GetPid());
  //   release locks cvars
  //   set children's parent to null
  //   handler memory pointers
  // store status 
} 


int Yalnix_Wait(int *status_ptr) { 
  // empty exited_children
  // switch to another available process
} 
int Yalnix_GetPid() { 
  return curr_proc->proc_id;
} 

int Yalnix_Brk(void *addr) {
  // Local variables
  unsigned int bottom_pg_heap = curr_proc->heap_base_page; // Already relative
  unsigned int top_pg_heap = (UP_TO_PAGE(addr) >> PAGESHIFT) - VMEM_0_PAGE_COUNT;
  unsigned int bottom_pg_stack = (DOWN_TO_PAGE(curr_proc->uc->sp) >> PAGESHIFT) - VMEM_0_PAGE_COUNT;
  unsigned int top_pg_stack = ((VMEM_1_LIMIT >> PAGESHIFT) - VMEM_1_PAGE_COUNT);

  int i;

  // Input checking
  if ((unsigned int) addr > (unsigned int)(bottom_pg_stack << PAGESHIFT) || 
          (unsigned int)addr < (unsigned int)(bottom_pg_heap << PAGESHIFT)) {
    TracePrintf(1, "Brk Error: address requested not in bounds\n");
    return ERROR;
  }

  /*
  if (count_items(&FrameList) <= 0) { // if we don't have enough memory 
  } 
  */

  for (i = bottom_pg_heap; i <= top_pg_heap; i++) {
      if ((*(curr_proc->region1_pt + i)).valid != 0x1) {
          
          if (count_items(&FrameList) <= 0) {
            TracePrintf(1, "Brk Error: not enough memory available");
            return ERROR;
          }
             
          (*(curr_proc->region1_pt + i)).valid = (u_long) 0x1;
          (*(curr_proc->region1_pt + i)).prot = (u_long) (PROT_READ | PROT_WRITE);
          (*(curr_proc->region1_pt + i)).pfn = (u_long) ( (PMEM_BASE + 
                  (pop(&FrameList)->id * PAGESIZE) ) >> PAGESHIFT);
      }
  }

  for (i = top_pg_heap; i < bottom_pg_stack; i++) {
      if ((*(curr_proc->region1_pt + i)).valid == 0x1) {
        (*(curr_proc->region1_pt + i)).valid = (u_long) 0x0;
        add_to_list(&FrameList, (void *)NULL, ((*(curr_proc->region1_pt + i)).pfn << PAGESHIFT) / PAGESIZE);
      }
  }
  curr_proc->brk_addr = top_pg_heap << PAGESHIFT;
  return SUCCESS;
}

int Yalnix_Delay(UserContext *uc, int clock_ticks) { 
  // Check parameters for validity
  if (clock_ticks < 0) return ERROR;
  if (clock_ticks == 0) return SUCCESS;

  // Add this process to the list of blocked processes
  add_to_list(blocked_procs, curr_proc, curr_proc->proc_id);
  curr_proc->delay_clock_ticks = clock_ticks;

  if (count_items(ready_procs) <= 0) {
    TracePrintf(3, "No Items on the Ready queue to switch to!\n");
    // crash here?
  } else {
    if (switch_to_next_available_proc(uc, 0) != SUCCESS)
        TracePrintf(3, "Switch to next available process failed\n");
  }  
  
  return SUCCESS;
} 

int Yalnix_Reclaim(int id) { 
  // for lock/cvar/pipe in locks + cvars + pipes
  //   destory_and_release(item with id == id)
} 
