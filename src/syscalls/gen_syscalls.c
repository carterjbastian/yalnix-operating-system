/*
 * System includes
 */
#include <hardware.h>

/*
 * Local Includes
 */
#include "../kernel.h"
#include "../PCB.h"
#include "syscalls.h"
// int Yalnix_Fork(UserContext *uc) ? not sure 
// what input args are here
int Yalnix_Fork() {
  /*

  rough draft: 

  // save uc (if we can grab it in input)
  memcpy(curr_proc->uc, uc, sizeof(UserContext));

  // create child
  PCB_t *child = new_process(uc);
  // create child page tables? 
  add_to_list(curr_proc->children, child, child->proc_id);
  
  // copy region 0 pointers? region 1 pointers? 
  // along with maybe some other data: 
  child->heap_base_page = curr_proc->heap_base_page;
  
  add_to_list(ready_procs, curr_proc, curr_proc->id);
  perform_context_switch(curr_proc, child, uc);

  */
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
  unsigned int bottom_pg_heap = curr_proc->heap_base_page;
  unsigned int top_pg_heap = UP_TO_PAGE(addr) >> PAGESHIFT;
  unsigned int bottom_pg_stack = DOWN_TO_PAGE(curr_proc->uc->sp) >> PAGESHIFT;
  unsigned int top_pg_stack = (VMEM_1_LIMIT >> PAGESHIFT);

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
      if (r1_pagetable[i].valid != 0x1) {
          
          if (count_items(&FrameList) <= 0) {
            TracePrintf(1, "Brk Error: not enough memory available");
            return ERROR;
          }
             
          r1_pagetable[i].valid = (u_long) 0x1;
          r1_pagetable[i].prot = (u_long) (PROT_READ | PROT_WRITE);
          r1_pagetable[i].pfn = (u_long) ( (PMEM_BASE + 
                  (pop(&FrameList)->id * PAGESIZE) ) >> PAGESHIFT);
      }
  }

  for (i = top_pg_heap; i < bottom_pg_stack; i++) {
      if (r1_pagetable[i].valid == 0x1) {
        r1_pagetable[i].valid = (u_long) 0x0;
        add_to_list(&FrameList, (void *)NULL, (r1_pagetable[i].pfn << PAGESHIFT) / PAGESIZE);
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
