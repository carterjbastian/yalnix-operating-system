#include "kernel.h"

int Yalnix_Fork() {
  // create_child_page_tables() 
  // create_child_pcb()
  // copy_kernal_stack_data_to_child()
} 

int Yalnix_Exec(char *filename, char **argvec) { 
  // filename.main(argvec[0], argvec.count[1]) ?
}


Void Yalnix_Exit(int status) { 
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

  if ((unsigned int) addr > (unsigned int)KERNEL_STACK_BASE || addr < kernel_data_start) {
    TracePrintf(1, "Brk Error: address requested not in bounds\n");
    return ERROR;
  }
  
  if (1) { // if we don't have enough memory 
    TracePrintf(1, "Brk Error: not enough memory available");
    return ERROR;
  } 
  
}

int Yalnix_Delay(int clock_ticks) { 
  if (clock_ticks < 0) return ERROR;
  if (clock_ticks == 0) return SUCCESS;

  add_to_list(delayed_processes, curr_proc, curr_proc->pid);
  curr_proc->delayed_clock_ticks = clock_ticks;
  PCB *next_proc = pop(ready_processes)->data;
  perform_context_switch(curr_proc, next_proc);
  
  return SUCCESS;
} 

int Yalnix_Reclaim(int id) { 
  // for lock/cvar/pipe in locks + cvars + pipes
  //   destory_and_release(item with id == id)
} 
