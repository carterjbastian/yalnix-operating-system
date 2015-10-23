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
void abort_process(int pid) { 
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
  ListNode *iterator = blocked_procs->first;


  for (i = 0; i < count_items(blocked_procs); i++) { 
    PCB_t *proc = iterator->data;
    proc->delay_clock_ticks -= 1;
    if (proc->delay_clock_ticks <= 0) { 
      add_to_list(ready_procs, proc, proc->proc_id);
    }
   iterator = iterator->next; 
  } 

  // Are there more processes waiting?
  if (count_items(ready_procs) > 0) { 
    PCB_t *next_proc = pop(ready_procs)->data;

    if (perform_context_switch(curr_proc, next_proc) != 0) {
      TracePrintf(1, "Context Switch failed\n");
    }
  }
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
  
  TracePrintf(1, "HANDLE_TRAP_MEMORY\n");

  // if (exception_is_requesting_more_stack) 
  // then enlarge stack to cover uc->addr

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
