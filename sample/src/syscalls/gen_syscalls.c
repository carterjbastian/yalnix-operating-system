#include "kernel.h"

int Fork() {
  // create_child_page_tables() 
  // create_child_pcb()
  // copy_kernal_stack_data_to_child()
} 

int Exec(char *filename, char **argvec) { 
  // filename.main(argvec[0], argvec.count[1]) ?
}


void Exit(int status) { 
  // release_resources(GetPid());
  //   release locks cvars
  //   set children's parent to null
  //   handler memory pointers
  // store status 
} 


int Wait(int *status_ptr) { 
  // empty exited_children
  // switch to another available process

} 

int GetPid() { 
  return curr_proc->proc_id;
} 

int Brk(void *addr) {

}

int Delay(int clock_ticks) { 
  // if (clock_ticks < 0) return ERROR
  // else if (clock_ticks == 0) return 0;
  // else 
  //   curr_proc->delay_clock_ticks = clock_ticks
  //   return 0 when curr_proc->delay_clock_ticks == 0
} 

int Reclaim(int id) { 
  // for lock/cvar/pipe in locks + cvars + pipes
  //   destory_and_release(item with id == id)
} 
