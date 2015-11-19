/*
 * System includes
 */
#include <hardware.h>
#include <string.h>
#include "tty.h"
#include "cvar.h"
#include "lock.h"

/*
 * Local Includes
 */
#include "kernel.h"
#include "PCB.h"
#include "syscalls.h"
#include "blocks.h"
#include "pipe.h"

/*
 * Function: Yalnix_Wait
 *  @status_ptr: A pointer to an integer to hold the child's return status
 *
 * Description:
 *
 * Pseudocode:
 *
 * Returns either the PID of the child that exited or ERROR.
 *
 * ToDo:
 *    - Clean this all up. It's messy.
 *    - Document better.
 *
 */
int Yalnix_Wait(int *status_ptr, UserContext *uc) { 
  TracePrintf(1, "Starting: Yalnix_Wait\n");
  /* Local Variables */
  int block_retval;
  PCB_t *parent;
  PCB_t *returning_child;
  int ret_child_pid;
  ListNode *found_item;
  void *data_ptr;

  /* Check that the process has children */
  parent = curr_proc;
  TracePrintf(1, "Waiting id: %d\n", curr_proc->proc_id);
  if (parent->children == NULL) {
    TracePrintf(3, "Process %d tried to call Wait without ever having children\n", parent->proc_id);
    return(ERROR);
  }

  if (parent->exited_children == NULL) {
    if (count_items(parent->children) <= 0) {
      TracePrintf(3, "Process %d has no active or dead children on which to wait\n", parent->proc_id);
      return(ERROR);
    }
  } else {
    if (count_items(parent->children) <= 0 && 
        count_items(parent->exited_children) <= 0) {
      TracePrintf(3, "Process %d has no active or dead children on which to call wait\n", parent->proc_id);
      return(ERROR);
    }
  }

  /* Check if any of the process' children have exited already */
  if (parent->exited_children != NULL && count_items(parent->exited_children) > 0) {
    TracePrintf(1, "Wait found a child process already exited!\n");
    // Remove exited child from list and get its id
    ListNode *ret_child = pop(parent->exited_children);
    ret_child_pid = ret_child->id;
    free(ret_child);

    // Find this in the dead_proc global list to retreive its return info
    found_item = find_by_id(dead_procs, ret_child_pid);
    if (found_item == NULL) {
      TracePrintf(3, "Exited child was found but not in the dead procs global list\n");
      return(ERROR);
    }

    // Copy the return info into the integer pointer passed into this call
    data_ptr = found_item->data;

    if (data_ptr == NULL) {
      TracePrintf(3, "Exited child had null status pointer\n");
      return(ERROR);
    }

    // Copy the return value and clean up unused info
    *status_ptr = *((int *) data_ptr);
    remove_from_list(dead_procs, data_ptr);
    free(data_ptr);
    
    // Return the pid of the returning child
    return(ret_child_pid);
  }

  /* Otherwise, set up the block to represent a wait call */
  bzero((char *) parent->block, sizeof(block_t)); // Just in case
  parent->block->active = BLOCK_ACTIVE;
  parent->block->type = WAIT_BLOCK;
  // The obj_ptr field gets a pointer to the blocking process
  parent->block->obj_ptr = (void *)parent;

  /* Update Kernel globals */
  add_to_list(blocked_procs, parent, parent->proc_id);

  /* Switch to the next avaialble process */
  if (count_items(ready_procs) <= 0) {
      TracePrintf(3, "No Items on the ready queue to switch to.\n");
      exit(ERROR);
  } else {
    if (switch_to_next_available_proc(uc, 0) != SUCCESS) {
      TracePrintf(3, "Failed to switch to the next process\n");
      exit(ERROR);
    }
  }

  /* Having returned from being blocked, collect info and return */
  TracePrintf(1, "Wait found a child process after blocking!\n");
  // Double check that this all worked correctly
  if (count_items(parent->exited_children) <= 0) {
    TracePrintf(3, "Wait returned after blocking incorrectly\n");
    return(ERROR);
  }

  // Remove exited child from list and get its id
  ListNode *ret_child = pop(parent->exited_children);
  ret_child_pid = ret_child->id;
  free(ret_child);

  // Find this in the dead_proc global list to retreive its return info
  found_item = find_by_id(dead_procs, ret_child_pid);
  if (found_item == NULL) {
    TracePrintf(3, "Exited child was found but not in the dead procs global list\n");
    return(ERROR);
  }

  // Copy the return info into the integer pointer passed into this call
  data_ptr = found_item->data;

  if (data_ptr == NULL) {
    TracePrintf(3, "Exited child had null status pointer\n");
    return(ERROR);
  }

  // Copy the return value and clean up unused info
  *status_ptr = *((int *) data_ptr);
  remove_from_list(dead_procs, data_ptr);
  free(data_ptr);
    
  // Return the pid of the returning child
  return(ret_child_pid);
  TracePrintf(1, "Done: Yalnix_Wait");
}


/*
 * Function: Yalnix_Exit
 *  @status: the integer status code upon exiting
 *  @uc: The UserContext passed into the Trap Handler
 *
 * Description:
 *
 * Psuedocode:
 *
 * Returns nothing.
 *
 * ToDo:
 *    - Sychronization Call clean up
 *    - PID recycling
 *    - Figure out how to halt the machine
 *    - 
 */
void Yalnix_Exit(int status, UserContext *uc) {
  TracePrintf(1, "Start: Yalnix_Exit\n");

  /*
   * Local Variables
   */
  int *status_p;                  // A pointer to the proc's exit status
  int pid;                        // PID of the exiting process
  int child_pid;                  // PID for a child

  int has_kids;                   // 1 if proc has active children, 0 otherwise
  int has_exited_kids;            // 1 if proc has exited kids, 0 otherwise
  int has_parent;                 // 1 if proc is child of living process, 0 otherwise

  PCB_t *parent;                  // Pointer to the parent process
  PCB_t *proc;                    // Pointer to the process exiting
  PCB_t *child;                   // Pointer to a child process (an iterator)
  PCB_t *next;                    // Pointer to the next process to run

  ListNode *iterator;             // For iterating through lists
  ListNode *temp_node;            // For storing info returned from lists

  int i;                          // Reusable loop iterator


  /*
   * Validate Input and store exit information
   */

  // Allocate kernel heap space for the exit status
  status_p = (int *) malloc(sizeof(int));
  *status_p = status;     // Copy the exit status' value
  proc = curr_proc;

  // Store the pid for ease of access
  pid = curr_proc->proc_id;

  // Control variables
  has_kids = ((proc->children == NULL) ? 0 : 1);
  has_exited_kids = ((proc->exited_children == NULL) ? 0 : 1);
  has_parent = ((proc->parent == NULL) ? 0 : 1);

  // Are we exiting the root process (init) with nothing to take its place?
  if (pid == 0 && (count_items(ready_procs) <= 0)) {
    TracePrintf(3, "\t===>\n\tHALTING MACHINE: About to halt machine by exiting init\n");
    exit(SUCCESS);
  }


  /*
   * Remove all traces of the proc from the kernel
   */
 
  /* WARNING: come back for this once synchronizations calls are done */ 
  /* Release Locks, Cvars, and Pipes */
    // DO THAT HERE

  /* Disown Living Children */
  child = NULL;

  if (has_kids) {
    // Remove pointer to self from all children's PCBs
    
    while ((iterator = pop(proc->children)) != NULL) {
      child = (PCB_t *) iterator->data;
      child->parent = NULL;   // You rat bastard
      free(iterator);
    }
  }

  /* Discard exited_children's uncollected exit statuses */
  /* 
   * NOTE: only the PIDs are stored in the exited children list.
   *        There is no data item - only the id.
   *        The exit status is stored only in the dead_procs global list.
   */
  if (has_exited_kids) {
    // We don't need to store them any more in the dead_procs list
    while ((iterator = pop(proc->exited_children)) != NULL) {
      // First, find the node in the dead_procs list
      child_pid = iterator->id;
      free(iterator);
      temp_node = find_by_id(dead_procs, child_pid);
     
      // If the node is in the global dead_procs list, remove it
      if (temp_node == NULL) {        // Does find_by_id return NULL?
        TracePrintf(3, "Couldn't find the exited child (PID = %d) in global list\n", child_pid);
      } else {
        void *dead_stat_ptr = temp_node->data;
        remove_from_list(dead_procs, dead_stat_ptr);
        free(dead_stat_ptr);
      }
    }
    // TO-DO: Recycle child_pid
  }
  
  /* Update Parent's list of children / exited children */
  if (has_parent) {
    parent = proc->parent;

    // Remove exiting proc from parent's children list
    if (remove_from_list(parent->children, (void *)proc) != 0)
      TracePrintf(3, "Failed to remove exiting proc from parent's list of children\n");

    // Add exiting proc to parent's exited children list
    if (parent->exited_children == NULL) {
      parent->exited_children = (List *) init_list();
      bzero(parent->exited_children, sizeof(List));
    }

    add_to_list(parent->exited_children, (void *)NULL, pid);
  }


  /* Move self from all_procs to dead_procs */
  if (remove_from_list(all_procs, (void *)proc) != 0)
    TracePrintf(3, "Failed to remove exiting proc from global list of all procs\n");

  add_to_list(dead_procs, (void *) status_p, pid);


  /*
   * Trash the Page Tables
   */

  /* Deallocate frames in Kernel Stack */
  for (i = 0; i < KS_NPG; i++) {
    if ( (*(proc->region0_pt + i)).valid == 0x1 ) {
      // Set it to invalid, free the physical frame, and reset the pfn
      (*(proc->region0_pt + i)).valid = (u_long) 0x0;
      add_to_list(&FrameList, (void *)NULL, PFN_TO_FNUM( (*(proc->region0_pt + i)).pfn ));
      (*(proc->region0_pt + i)).pfn = (u_long) 0x0;
    }
  }

  /* Deallocate frames in Region 1 */
  for (i = 0; i < VMEM_1_PAGE_COUNT; i++) {
    if ( (*(proc->region1_pt + i)).valid == 0x1 ) {
      // Set it to invalid, free the physical frame, and reset the pfn
      (*(proc->region1_pt + i)).valid = (u_long) 0x0;
      add_to_list(&FrameList, (void *)NULL, PFN_TO_FNUM( (*(proc->region1_pt + i)).pfn ));
      (*(proc->region1_pt + i)).pfn = (u_long) 0x0;
    }
  }

  /* 
   * I'm scared of flushing this. Are any of the variables in THIS function in
   * the kernel stack that will be lost or changed?
   */
  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);


  /*
   * Garbage Collection: Deallocate kernel heap resources
   */
  // Free Pointers to Family History Lists
  if (has_kids)
    free(proc->children);
  if (has_exited_kids)
    free(proc->exited_children);

  // Free Pointers to Page Tables
  free(proc->region1_pt);
  free(proc->region0_pt);

  // Free Pointers to UserContext and KernelContext
  free(proc->uc);
  free(proc->kc_p);

  // Free Pointer to the Process Control Block itself
  free(proc);

  /*
   * Move on to Next Process
   */
  // Get the next available process
  ListNode *next_node = pop(ready_procs);
  next = (PCB_t *) next_node->data;
  free(next_node);
  
  // Ensure that we don't run a brand new (uninitialized) proc out of Exit call
  while (next->kc_set == 0) {
    add_to_list(ready_procs, (void *) next, next->proc_id);
    next_node = pop(ready_procs);
    next = (PCB_t *) next_node->data;
    free(next_node);
  }

  TracePrintf(1, "End: Yalnix_Exit\n");

  // Call perform_context_switch with a null curr_proc pointer
  if (perform_context_switch((PCB_t *) NULL, next, uc) != SUCCESS) {
    TracePrintf(3, "Fatal Error: Failed to switch out of Exited Process (PID=%d)\n", pid);
    // NOTE: more Trace info would be helpful here for the user
    exit(ERROR);
  }
} 


 
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
  ListNode *node;


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
    node = pop(&FrameList);
    pfn_temp = node->id;
    free(node);
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
      node = pop(&FrameList);
      pfn_temp = node->id;
      free(node);
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
  // Mark the child as the parent's
  child->parent = parent;

  // Add record of the child to the parent's List
  if (parent->children == NULL) {
    parent->children = (List *) init_list();
    bzero(parent->children, sizeof(List));
  }
  add_to_list(parent->children, (void *)child, child->proc_id);

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



/*
 * Function: Yalnix_Exec
 *  @uc: The user context passed into the trap handler
 *
 * Description:
 *
 * PsuedoCode:
 *
 * Returns: Error to the original program on failure
 *    Nothing to the new program if it's loaded successfully.
 */
int Yalnix_Exec(UserContext *uc) { 
  TracePrintf(1, "\tStart: Yalnix_Exec\n");

  /*
   * Local Variables
   */
  int argc;                   // The number of arguments passed into new program 
  char *filename;             // The filename of the new program
  char **argv;                // Null-terminated list of arguments to new program

  char *fname_p;              // The pointer to the filename passed via the uc
  char **arg_p;                // The pointer to the filename passed via the uc

  PCB_t *proc;                // The process that's being switched out of

  int i;                      // Iterator variable for loops
  int len;                    // A length variable for string size counting
  int rc;                     // Return code variable
  /*
   * Parse the filename, argc, argv[] arguments.
   */

  // Get the pointers from the UC
  fname_p = (char *) uc->regs[0];
  arg_p = (char **) uc->regs[1];

  // Copy the filename into a permanent, kernel location
  filename = (char *) malloc((strlen(fname_p) + 1) * sizeof(char));
  if (!filename) { // Malloc Check
    TracePrintf(3, "Failed to allocate memory for the file to be execed into\n");
    return(ERROR);
  }
  
  memcpy((void *)filename, (void *)fname_p, ((strlen(fname_p) + 1) * sizeof(char)));

  // Count the number of arguments
  argc = 0;
  while ((*(arg_p + argc)) != '\0')
    argc++;

  argc++; // The null pointer counts

  // Allocate space for argc character pointers in argv
  argv = (char **) malloc (argc * sizeof(char *));
  if (!argv) { // Malloc check
    TracePrintf(3, "Failed to allocate memory for the file to be execed into\n");
    return(ERROR);
  }

  // Loop through the arguments, copying them into argv
  for (i = 0; i < argc - 1; i++) { // Don't include the null pointer
    len = strlen(*(arg_p + i)) + 1; // include the null pointer
    
    // Allocate space for this argument
    argv[i] = (char *) malloc(len * sizeof(char));
    if (!argv[i]){
      TracePrintf(3, "Failed to allocate memory for the file to be execed into\n");
      return(ERROR);
    }
    memcpy((void *)argv[i], (void *)(*(arg_p + i)), len);
  }

  // Add the null pointer
  //argv[argc - 1] = (char *) malloc(1);
  argv[argc - 1] = '\0';

  /* WARNING: need to do more thorough input checking on these arguments! */
  TracePrintf(1, "Exec: Finished copything the arguments and filename\n");


  /*
   * Edit the PCB_t and UC pointers to resemble a blank process
   *
   * For the UC:
   *  vector, code, and addr are left alone
   *  pc, sp, and ebp will be changed inside LoadProgram
   *  regs[8] is cleared to zeros
   *
   * For the PCB:
   *  proc_id, uc, and  are left alone
   *  kc_p will be re-initialized in MyKCSClone, so kc_set is set to 0
   *  region0_pt and region1_pt are left alone, but their contents will change
   *    in the next section
   *  WARNING: What do we do with children and exited_children??
   *  brk_addr and heap_break_page will be set in LoadProgram
   */

  // Store the current process
  proc = curr_proc;

  // Clear the registers in the UserContext
  bzero((void *)uc->regs, (8 * sizeof(u_long)));

  // set kc_set to 0
  proc->kc_set = 0;
  

  /*
   * Trash the old Region 1 and reinitialize it to be blank
   */
  for (i = 0; i < VMEM_1_PAGE_COUNT; i++) {
    if ( (*(proc->region1_pt + i)).valid == 0x1) {
      // Put old frame back onto the frame list
      add_to_list(&FrameList, (void *) NULL, PFN_TO_FNUM( (*(proc->region1_pt + i)).pfn ));
      // Reset pte to defaults (defaults for protections should still apply)
      (*(proc->region1_pt + i)).pfn = (u_long) 0x0;
      (*(proc->region1_pt + i)).valid = (u_long) 0x0;
    }
  }


  /*
   * Trash the old kernel stack and reinitialize it
   */
  for (i = 0; i < KS_NPG; i++) {
    // Keep default prot and valid settings, but give it a new physical frame
    add_to_list(&FrameList, (void *) NULL, PFN_TO_FNUM((*(proc->region0_pt + i)).pfn));
    // No need to verify that we have enough frames because we just added one
    ListNode *node = pop(&FrameList);
    (*(proc->region0_pt + i)).pfn = FNUM_TO_PFN(node->id);
    free(node);
  }
  
  // Flush the TLB with the new info
  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);


  /*
   * Load in the next program with call to LoadProgram
   */
  rc = LoadProgram(filename, argv, proc);
  if (rc != SUCCESS) {
    /* WARNING: We need to figure out what to do here w/o a proc to return to */
    TracePrintf(3, "FATAL ERROR: LoadProgram failed to load execed program\n");
    return(ERROR);
  }


  /*
   * Update the UserContext Pointer passed into the trap handler and 
   * return successfully.
   */
  memcpy((void *)uc, (void *)proc->uc, sizeof(UserContext));
  return 0;
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
          ListNode *node = pop(&FrameList);
          (*(curr_proc->region1_pt + i)).pfn = (u_long) ( (PMEM_BASE + 
                  (node->id * PAGESIZE) ) >> PAGESHIFT);
          free(node);
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

  // Set up the process' block to represent a delay
  curr_proc->block->active = BLOCK_ACTIVE;
  curr_proc->block->type = DELAY_BLOCK;
  curr_proc->block->data.delay_count = clock_ticks;

  // Add this process to the list of blocked processes
  add_to_list(blocked_procs, curr_proc, curr_proc->proc_id);

  if (count_items(ready_procs) <= 0) {
    TracePrintf(3, "No Items on the Ready queue to switch to!\n");
    exit(ERROR);
  } else {
    if (switch_to_next_available_proc(uc, 0) != SUCCESS)
        TracePrintf(3, "Switch to next available process failed\n");
  }  
  
  return SUCCESS;
} 

int Yalnix_TtyWrite(int tty_id, void *buf, int len) { 

  TracePrintf(1, "Start: TtyWrite\n");
  ListNode *node = find_by_id(ttys, tty_id);
  TTY_t *tty = node->data;
  
  // setting up the new buffer we'll write to 
  // needs to be on heap to survive context switch
  if (curr_proc->write_buf->buf) 
    free(curr_proc->write_buf->buf);

  curr_proc->write_buf->buf = calloc(len, sizeof(char));
  memcpy(((buffer*)curr_proc->write_buf)->buf, buf, len);
  curr_proc->write_buf->len = len;
  
  // now we put ourselves on list of writers and start writing
  add_to_list(tty->writers, curr_proc, curr_proc->proc_id);
  
  // if we're the only one, transmit
  // otherwise we'll call transmit from trap
  if (count_items(tty->writers) == 1) { 
    TracePrintf(1, "PID: %d I'm the only writer - transmitting now.\n", curr_proc->proc_id);
    if (len > TERMINAL_MAX_LINE) {
      TtyTransmit(tty_id, buf, TERMINAL_MAX_LINE);
    } else { 
      TtyTransmit(tty_id, buf, len); 
    } 
    
    // we've returned from transmit, but aren't finished writing. 
    // so switch to another proc until we're done
    switch_to_next_available_proc(curr_proc->uc, 0);
    TracePrintf(1, "PID: %d Finished transmitting.\n", curr_proc->proc_id);
  } else {
    TracePrintf(1, "PID: %d Other writers exist, I'll do my writing when I'm woken up in trap transmit\n", curr_proc->proc_id);
  }



  TracePrintf(1, "End: TtyWrite\n");
  return len;
} 



/* 
   See if we have a buffer stored (ie. somebody already wrote). 
   If not, wait until we're woken up (should be when a buffer 
   is waiting for us...)

   Todo: handle extremely long inputs (use multiple buffers)
 */
int Yalnix_TtyRead(int tty_id, void *buf, int len) { 
  
  TracePrintf(1, "Start: TtyRead\n");
  ListNode *node = find_by_id(ttys, tty_id);
  TTY_t *tty = node->data;
  List *buffers = tty->buffers;
  ListNode *buf_node = pop(buffers);
  buffer *stored_buf;

  // if there's not stored buf, switch procs, then grab 
  // it when we're awake
  if (!buf_node) { 
    TracePrintf(1, "PID: %d No buffer for us. Going to wait. Should be woken up by trap tty_receive when one is available\n", curr_proc->proc_id);
    curr_proc->read_len = len;
    add_to_list(tty->readers, curr_proc, curr_proc->proc_id);
    switch_to_next_available_proc(curr_proc->uc, 0);

    // we just woke up, so now there should be a buff! 
    TracePrintf(1, "PID: %d Woken up.\n", curr_proc->proc_id);
    buf_node = pop(buffers);
    stored_buf = buf_node->data;
  } 
  
  // should be a sanity check... 
  if (!stored_buf) { 
    TracePrintf(3, "Something went wrong in TtyRead..\n");
    if (buf_node)
      free(buf_node);
    return ERROR;
  }
  
  TracePrintf(1, "PID: %d Grabbing buffer.\n", curr_proc->proc_id);
  curr_proc->read_len = 0;
  memcpy(buf, stored_buf->buf, len);
  
  int stored_buf_chars = strlen(stored_buf->buf);
  int len_to_store = stored_buf_chars - len;
  
  // check if we didn't read full buffer, 
  // put it back on list if we didn't
  if (len_to_store > 0) {
    TracePrintf(1, "PID: %d Didn't read entire buffer - storing the rest.\n", curr_proc->proc_id);
    len = len - len_to_store;
    char *buff_to_store = stored_buf->buf + len;
    memcpy(stored_buf->buf, buff_to_store, len_to_store);
    stored_buf->len = len_to_store;
    add_to_list(tty->buffers, stored_buf, 0);
  } else { 
    TracePrintf(1, "PID: %d Read the entire buffer.\n", curr_proc->proc_id);
  } 
  
  TracePrintf(1, "End: TtyRead\n");
  free(buf_node);
  return len;
}

int Yalnix_CvarInit(int *cvar_idp) { 
  TracePrintf(1, "Starting: Yalnix_CvarInit\n");  
  // todo: return ERROR if 
  // validate (cvar_idp) == false

  CVAR_t *cvar = (CVAR_t*)malloc(sizeof(CVAR_t));
  cvar->id = next_resource_id;
  next_resource_id++;
  cvar->waiters = (List*)init_list();
  
  *cvar_idp = cvar->id;

  add_to_list(cvars, cvar, cvar->id);

  TracePrintf(1, "Finishing: Yalnix_CvarInit\n");
  return SUCCESS;
} 

int Yalnix_CvarSignal(int cvar_id) { 
  TracePrintf(1, "Starting: Yalnix_CvarSignal\n");
  ListNode *cvar_node = find_by_id(cvars, cvar_id);
  if (!cvar_node) { 
    return ERROR;
  } 

  CVAR_t *cvar = cvar_node->data;
  ListNode *waiter_node = pop(cvar->waiters);

  // if no waiters, do nothing.
  if (!waiter_node) {
    return SUCCESS;
  } 

  PCB_t *waiter = waiter_node->data;
  free(waiter_node);
  add_to_list(ready_procs, waiter, waiter->proc_id);
   TracePrintf(1, "Finishing: Yalnix_CvarSignal\n");
  return SUCCESS;
} 

int Yalnix_CvarBroadcast(int cvar_id)  {
  TracePrintf(1, "Starting: Yalnix_CvarBroadcast\n");
  ListNode *cvar_node = find_by_id(cvars, cvar_id);
  if (!cvar_node) { 
    return ERROR;
  } 
  
  CVAR_t *cvar = cvar_node->data;
  
  ListNode *waiter_node = pop(cvar->waiters);

  while(waiter_node) { 
    PCB_t *waiter = waiter_node->data;
    add_to_list(ready_procs, waiter, waiter->proc_id);
    waiter_node = waiter_node->next;
  } 
  TracePrintf(1, "Finishing: Yalnix_CvarBroadcast\n"); 
  return SUCCESS;
} 

int Yalnix_CvarWait(int cvar_id, int lock_id) { 
  TracePrintf(1, "Starting: Yalnix_CvarWait\n");
  ListNode *cvar_node = find_by_id(cvars, cvar_id);
  if (!cvar_node) { 
    return ERROR;
  }   
  CVAR_t *cvar = cvar_node->data;
  
  ListNode *lock_node = find_by_id(locks, lock_id);
  if (!lock_node) { 
    return ERROR;
  } 
  LOCK_t *lock = lock_node->data;
  
  Yalnix_Release(lock->id);
  
  add_to_list(cvar->waiters, curr_proc, curr_proc->proc_id);
  switch_to_next_available_proc(curr_proc->uc, 0);

  Yalnix_Acquire(lock->id);
  TracePrintf(1, "Finishing: Yalnix_CvarWait\n");
}


int Yalnix_LockInit(int *lock_idp) { 
  
  // todo: return ERROR if 
  // validate (lock_idp) == false
  
  LOCK_t *lock = (LOCK_t *)malloc(sizeof(LOCK_t));
  lock->id = next_resource_id;
  next_resource_id++;
  lock->is_claimed = 0;
  lock->owner_id = -1;
  lock->waiters = (List*)init_list();

  *lock_idp = lock->id;

  add_to_list(locks, lock, lock->id);

  return SUCCESS;
} 

int Yalnix_Acquire(int lock_id) { 
  ListNode *lock_node = find_by_id(locks, lock_id);
  if (!lock_node) { 
    return ERROR;
  } 
  LOCK_t *lock = lock_node->data;
  
  if (lock->owner_id == curr_proc->proc_id) { 
    return SUCCESS;
  } 

  if (!lock->is_claimed) { 
    lock->is_claimed = 1;
    lock->owner_id = curr_proc->proc_id;
    return SUCCESS;
  } 
  
  add_to_list(lock->waiters, curr_proc, curr_proc->proc_id);
  switch_to_next_available_proc(curr_proc->uc, 0);
 
  // when we return from the above, we'll have the lock! 
  // (it's given to us in release) 

  return SUCCESS;
  
} 

int Yalnix_Release(int lock_id) { 
  ListNode *lock_node = find_by_id(locks, lock_id);
  if (!lock_node) { 
    return ERROR;
  } 
  LOCK_t *lock = lock_node->data;
  
  if (!lock->is_claimed || lock->owner_id != curr_proc->proc_id) { 
    return ERROR;
  }
  
  ListNode *waiter_node = pop(lock->waiters);
  PCB_t *waiter = NULL;
  if (waiter_node) {
    waiter = waiter_node->data;
    free(waiter_node);
  }
  
  if (!waiter) { 
    lock->is_claimed = 0;
    lock->owner_id = -1;
    return SUCCESS;
  } 

  // else, there's a waiter, so let's give them the lock 
  // and let them wake up (add to ready_procs)
  lock->owner_id = waiter->proc_id;
  add_to_list(ready_procs, waiter, waiter->proc_id);
  
  return SUCCESS;
} 

/*
 * Yalnix_PipeInit
 *  @pip_idp Pointer to the int that will hold the pipe's identifier
 *
 * TO DO:
 *  - Check that we haven't exhausted all of the resource identifiers
 */
int Yalnix_PipeInit(int *pip_idp) {
  TracePrintf(1, "Starting: Yalnix_PipeInit\n");
  // Local Variables
  pipe_t *pipe;

  // Allocate space for a new Pipe
  pipe = (pipe_t *) malloc( sizeof(pipe_t) );
  if (pipe == NULL)
    return ERROR;
  
  pipe->buf = (char *) malloc( sizeof(char) * MAX_PIPE_LEN );
  if (pipe->buf == NULL) {
    free(pipe);
    return ERROR;
  }

  pipe->waiters = (List *) init_list();
  if (pipe->waiters == NULL) {
    free(pipe->buf);
    free(pipe);
    return ERROR;
  }

  // Initialize the new pipe to be empty
  // TO DO: Check if we've reached the max resource count
  pipe->id = next_resource_id++;
  pipe->len = 0;
  pipe->waiters->first = NULL;
  bzero(pipe->buf, sizeof(char) * MAX_PIPE_LEN);

  // Add the new pipe to the list of pipes
  add_to_list(pipes, (void *)pipe, pipe->id);

  // Copy the pipe identifier into the userland variable
  *pip_idp = pipe->id;

  TracePrintf(1, "Finishing: Yalnix_PipeInit\n");
  return SUCCESS;
} 

/*
 * Function: Yalnix_PipeRead
 *  @pipe_id
 *  @buf
 *  @len
 *
 * Description:
 *
 * To Do:
 *
 */
int Yalnix_PipeRead(int pipe_id, void *buf, int len) {
  TracePrintf(1, "Starting: Yalnix_PipeRead\n");
  // Local variables
  ListNode *pipe_node;
  pipe_t *pipe = NULL;

  // Check that the pipe exists
  pipe_node = find_by_id(pipes, pipe_id);

  if (pipe_node)
    pipe = pipe_node->data;

  if (!pipe)
    return(ERROR);

  // Check the validity of the number of characters to read from the pipe
  if (len > MAX_PIPE_LEN)
    return(ERROR);

  // If there are not len characters to read, block until there is
  if (len > pipe->len) {
    // Note that it's id in the list is the number of characters it needs
    add_to_list(pipe->waiters, (void *) curr_proc, len);
    switch_to_next_available_proc(curr_proc->uc, 0);
  }

  // When we get back here, we'll be able to do the read
  memmove((void *)buf, (void *)pipe->buf, len);

  // Shift the contents of the pipe's buffer down to the beginning
  memmove((void *)pipe->buf, (void *)(pipe->buf + len), MAX_PIPE_LEN - len);
  bzero(pipe->buf + (MAX_PIPE_LEN - len), len);

  // Reset the index for the pipe
  pipe->len -= len;

  TracePrintf(1, "Finishing: Yalnix_PipeRead\n");
  return(len);
} 


// BUGS: only works with one waiter at a time!
int Yalnix_PipeWrite(int pipe_id, void *buf, int len) { 
  TracePrintf(1, "Starting: Yalnix_PipeWrite\n");
  // Local varialbes
  ListNode *pipe_node;
  pipe_t *pipe = NULL;
  ListNode *waiter_node;
  PCB_t *waiter_proc;
  int required_len;
  

  // Check that the pipe exists
  pipe_node = find_by_id(pipes, pipe_id);

  if (pipe_node)
    pipe = pipe_node->data;

  if (!pipe)
    return(ERROR);

  // Check the validity of len parameter
  //   Can't write more to the pipe than the space we have left
  if (len > (MAX_PIPE_LEN - pipe->len))
    return(ERROR);

  // Copy the characters in and update the pipe's index
  memmove((void *)(pipe->buf + pipe->len), (void *)buf, len);
  pipe->len += len;
  
  // Take the next waiter off of the list of waiters if possible
  waiter_proc = NULL;
  // TO DO: Change this to be a non-removing version of pop!
  waiter_node = pop(pipe->waiters);
  int waiter_id;
  if (waiter_node) { 
    waiter_proc = (PCB_t *) waiter_node->data;
    waiter_id = waiter_node->id;
  }
  // Check to see if this waiter should be kept on the list or put back on to
  // waiters
  if (waiter_proc) {
    required_len = waiter_node->id;

    if (required_len > pipe->len) {
      // Put it back on the wait list
      add_to_list(pipe->waiters, (void *)waiter_proc, required_len);
    } else { // Put it on the ready queue
      add_to_list(ready_procs, (void *)waiter_proc, waiter_proc->proc_id);
    }

    // Free the waiter_node returned from pop
    free(waiter_node);
  }

  TracePrintf(1, "Finishing: Yalnix_PipeWrite\n");
  if (waiter_node)
    free(waiter_node);

  return len;
} 

/* 
   Destory lock, condition variable, or piped identified by id
   and release resources. 
   If this function is called and processes
   are waiting on the resource, or have it locked, this returns ERROR
   Otherwise returns SUCCESS
*/
int Yalnix_Reclaim(int id) { 

  ListNode *node; 
  
  if (node = find_by_id(locks, id)) { 
    
    LOCK_t *lock = node->data;

    if (count_items(lock->waiters) > 0) { 
      TracePrintf(1, "Can't reclaim lock: people are waiting on it.");
      return ERROR;
    } 
    
    if (lock->is_claimed) {
      TracePrintf(1, "Can't reclaim lock: someone has it");
      return ERROR;
    }
    
    remove_from_list(locks, node->data);
    free(lock);
    
  } else if (node = find_by_id(cvars, id)) { 
    
    CVAR_t *cvar = node->data;
    
    if (count_items(cvar->waiters) > 0) { 
      TracePrintf(1, "Can't reclaim cvar: people are waiting on it.");
      return ERROR;
    } 
    
    remove_from_list(cvars, node->data);
    free(cvar);
    
  } else if (node = find_by_id(pipes, id)) { 

    pipe_t *pipe = node->data;
    
    if (count_items(pipe->waiters) > 0) { 
      TracePrintf(1, "Can't reclaim pipe: people are waiting on it.");
      return ERROR;
    } 
    
    remove_from_list(pipes, node->data);
    free(pipe->buf);
    free(pipe);
  } 
  
  return SUCCESS;
  
}
