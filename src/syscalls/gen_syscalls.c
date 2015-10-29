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
                  (pop(&FrameList)->id * PAGE_SIZE) ) >> PAGESHIFT);
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

int Yalnix_Delay(int clock_ticks) { 
  if (clock_ticks < 0) return ERROR;
  if (clock_ticks == 0) return SUCCESS;

  add_to_list(blocked_procs, curr_proc, curr_proc->pid);
  curr_proc->delay_clock_ticks = clock_ticks;
  PCB *next_proc = pop(ready_procs)->data;
  perform_context_switch(curr_proc, next_proc);
  
  return SUCCESS;
} 

int Yalnix_Reclaim(int id) { 
  // for lock/cvar/pipe in locks + cvars + pipes
  //   destory_and_release(item with id == id)
} 
