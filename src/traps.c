// traps.c
#include <hardware.h>
#include "syscalls/cvars.h"
#include "syscalls/gen_syscalls.h"
#include "syscalls/locks.h"
#include "syscalls/pipes.h"
#include "syscalls/tty.h"

#include "kernel.h"
#include "linked_list.h"

// used by a couple different traps
void abort_current_process(UserContext *uc) { 


  TracePrintf(1, "Aborting Process with PID: %d", curr_proc->proc_id);
  add_to_list(dead_procs, curr_proc, curr_proc->proc_id);
  remove_from_list(all_procs, curr_proc);
  switch_to_next_available_proc(uc, 0);

} 

/*

Results from kernall/system/sys call
Used to request some type of service from os kernel,
like creating new process, mem allocation or I/O.

*/
void HANDLE_TRAP_KERNEL(UserContext *uc) { 
  // arguments to kernel call are in uc->regs
  // execute requrest kernal call (uc->code)

  /*
    int return_val;
    switch(uc->code) { 
      case SYS_CALL_CONSTANT_HERE (see yalnix.h): 
        return_val = sys_call_goes_here(uc->regs[x], uc->regs[x+1] ...);
        break;
      case ...
      case ...
    } 

    uc->regs[0] = return_val;

  */
} 

/* 

Results from hardware clock 
which generates periodic clock interupts

*/
void HANDLE_TRAP_CLOCK(UserContext *uc) { 

  TracePrintf(1, "HANDLE_TRAP_CLOCK\n");

  // perform context switch to ready_proccesses.first()
  // should implement round-robin process scheduling with 
  // cpu quantum per process of 1 clock tick
  
  int i;
  if (count_items(blocked_procs) > 0) {
      ListNode *iterator = blocked_procs->first;


    for (i = 0; i < count_items(blocked_procs); i++) { 
        PCB_t *proc = iterator->data;
        proc->delay_clock_ticks -= 1;
        
        if (proc->delay_clock_ticks <= 0) { 
            add_to_list(ready_procs, proc, proc->proc_id);
        }
        iterator = iterator->next; 
    } 
  }

  // Are there more processes waiting?
  if (count_items(ready_procs) > 0) { 
    switch_to_next_available_proc(uc, 1);
  } // else just continue running current process
  
  TracePrintf(1, ">>> HANDLE_TRAP_CLOCK\n");

} 

/*

Results from execution of illegal instruction by 
currently executing user process. E.g. undefined 
machine language opcode, illegal addressing mode, 
or privileged instruction while not in kernel mode.

*/

void HANDLE_TRAP_ILLEGAL(UserContext *uc) { 
  // uc->addr contains mem address whose referenced 
  // caused the exception
  
  // abort_process(uc->addr)
  // continue_running_other_processes()
}

/*

Results from disallowed memory access by current 
user process. 

*/
void HANDLE_TRAP_MEMORY(UserContext *uc) { 
  TracePrintf(1, "HANDLE_TRAP_MEMORY: addr %p\n", uc->addr);

  // Check if this is a permissions error
  if (uc->code == YALNIX_ACCERR) {
      TracePrintf(3, "\tProcess %d had a permissions error at addr %p\n",
              curr_proc->proc_id, uc->code);
      exit(-1);
  }

  // Otherwise, it's an access error
  
  /* Get the values of pages to loop through */
  unsigned int addr_pg = DOWN_TO_PAGE(uc->addr) >> PAGESHIFT;
  unsigned int top_stack_pg = VMEM_1_LIMIT >> PAGESHIFT;
  unsigned int usr_brk_pg = UP_TO_PAGE(curr_proc->brk_addr) >> PAGESHIFT;
  unsigned int usr_heap_base = curr_proc->heap_base_page;
  struct pte *temp_ent;
  int i;
  /* Check if the requested address would interfere with the heap */
  if (addr_pg - usr_brk_pg <= 2) {
      TracePrintf(3, "\tProcess %d does not have enough memory to grow the stack\n",
              curr_proc->proc_id, uc->code);
      exit(-1);
  }

  /* Loop through each page in virtuall memory and allocate a physical frame */
  for (i = addr_pg; i <= top_stack_pg; i++) {
      temp_ent = (curr_proc->region1_pt + i);

      // If there's no page allocated for this
      if (temp_ent->valid == (u_long) 0x0) {

        // Check that there are enough physical pages
        if (count_items(&FrameList) <= 0) { /* This counting is inefficient... */
            TracePrintf(3, "\tProcess %d requested more memory for the stack, but there are not enough physical frames\n",
                    curr_proc->proc_id);
            exit(-1);
        }

        // Allocate the pages
        // Set it to valid with the proper permissions
        temp_ent->valid = (u_long) 0x1;
        temp_ent->prot = (u_long) (PROT_READ | PROT_WRITE);
        // Get it a page
        temp_ent->pfn = (u_long) (((pop(&FrameList))->id * PAGESIZE) >> PAGESHIFT);
      }
  }
  
  WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_ALL);
  /* Is there anything I'm forgetting? */
  // otherwise imitate TRAP_ILLEGAL(uc)
} 

/*

Results from division by 0, overflow, etc.

*/
void HANDLE_TRAP_MATH(UserContext *uc) { 
  // imitate TRAP_ILLEGAL
} 

/*

Results from complete line of input being available 
from one of the terminals attached to system.

*/
void HANDLE_TRAP_TTY_RECEIVE(UserContext *uc) { 
  // read input using TtpReceive hardware op
  // if necessary, buffer input line for a subsequent 
  // TtyRead kernall call by some user process

  // (terminal indicated by uc->code) 
} 

/* 

Current buffer of data (prev. given to controller
on a TtyTransmit) has been completely sent to terminal.

*/
void HANDLE_TRAP_TTY_TRANSMIT(UserContext *uc) { 
  // (terminal indicated by uc->code) 
} 

void HANDLE_TRAP_DISK(UserContext *uc) {
  // do things
}
