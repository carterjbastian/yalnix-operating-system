// traps.c
/*
 * System Includes
 */
#include <hardware.h>
#include <yalnix.h>
#include "tty.h"
#include "syscalls.h"
#include "kernel.h"
#include "linked_list.h"
#include "traps.h"

// used by a couple different traps
void abort_current_process(int exit_code, UserContext *uc) {
  TracePrintf(1, "Kernel Trap Handler: Aborting Current Process. PID: %d, exit_code: %d\n", 
      curr_proc->proc_id, exit_code);
  
  // Call the Exit syscall as normal
  Yalnix_Exit(exit_code, uc);
} 

/*

Results from kernall/system/sys call
Used to request some type of service from os kernel,
like creating new process, mem allocation or I/O.

*/
void HANDLE_TRAP_KERNEL(UserContext *uc) { 
  // arguments to kernel call are in uc->regs
  // execute requested kernal call (uc->code)

  
  int retval;

  // Local variables for syscall arguments
  void *addr;               // Address for YALNIX_GETPID
  int clock_ticks;          // Number of clock ticks for YALNIX_DELAY
  int exit_status;          // The exit status for YALNIX_EXIT
  
  // ttys:
  int tty_id;
  void *buf;
  int len;
  int *stat_ptr;

  switch(uc->code) { 
      case YALNIX_FORK: 
        retval = Yalnix_Fork(uc);
        break;

      case YALNIX_EXEC:
        retval = Yalnix_Exec(uc);
        break;

      case YALNIX_EXIT:
        exit_status = (int) uc->regs[0];
        Yalnix_Exit(exit_status, uc);
        break;

      case YALNIX_WAIT:
        stat_ptr = (int *) uc->regs[0];
        retval = Yalnix_Wait(stat_ptr, uc);
        break;

      case YALNIX_GETPID:
        retval = Yalnix_GetPid();
        break;

      case YALNIX_BRK:
        addr = (void *) uc->regs[0];
        retval = Yalnix_Brk(addr);
        break;

      case YALNIX_DELAY:
        clock_ticks = (int) uc->regs[0];
        retval = Yalnix_Delay(uc, clock_ticks);
        break;
        
      case YALNIX_TTY_WRITE:
        tty_id = (int) uc->regs[0];
        buf = (void *) uc->regs[1];
        len = (int) uc->regs[2];
        retval = Yalnix_TtyWrite(tty_id, buf, len);
        break;

      case YALNIX_TTY_READ:
        tty_id = (int) uc->regs[0];
        buf = (void *) uc->regs[1];
        len = (int) uc->regs[2];
        retval = Yalnix_TtyRead(tty_id, buf, len);
        break;

      case YALNIX_CVAR_INIT:
        retval = Yalnix_CvarInit((int*)uc->regs[0]);
        break;

      case YALNIX_CVAR_SIGNAL:
        retval = Yalnix_CvarSignal((int)uc->regs[0]);
        break;

      case YALNIX_CVAR_BROADCAST:
        retval = Yalnix_CvarBroadcast((int)uc->regs[0]);
        break;
        
      case YALNIX_CVAR_WAIT:
        retval = Yalnix_CvarWait((int)uc->regs[0], (int)uc->regs[1]);
        break;

      case YALNIX_LOCK_INIT:
        retval = Yalnix_LockInit((int*)uc->regs[0]);
        break;
    
      case YALNIX_LOCK_ACQUIRE:
        retval = Yalnix_Acquire((int)uc->regs[0]);
        break;

      case YALNIX_LOCK_RELEASE:
        retval = Yalnix_Release((int)uc->regs[0]);
        break;
        /*
      case YALNIX_PIPE_INIT:
        retval = Yalnix_PipeRead((int*)uc->regs[0]);
        break;

      case YALNIX_PIPE_READ:
        retval = Yalnix_PipeRead((int*)uc->regs[0]);
        break;

      case YALNIX_PIPE_WRITE:
        retval = Yalnix_PipeWrite((int*)uc->regs[0]);
        break;
        */
      case YALNIX_RECLAIM:
        retval = Yalnix_Reclaim((int)uc->regs[0]);
        break;        

      default:
        TracePrintf(3, "Unrecognized syscall: %d\n", uc->code);
        break;
    } 

    uc->regs[0] = retval;

  
} 

/* 

Results from hardware clock 
which generates periodic clock interupts

*/
void HANDLE_TRAP_CLOCK(UserContext *uc) { 

  TracePrintf(1, "Start: HANDLE_TRAP_CLOCK\n");

  // perform context switch to ready_proccesses.first()
  // should implement round-robin process scheduling with 
  // cpu quantum per process of 1 clock tick
  
  int i;
  if (count_items(blocked_procs) > 0) {
      ListNode *iterator = blocked_procs->first;

    // Remove all the process that are no longer blocked from the blocked queue
    iterator = blocked_procs->first;
    while(iterator->next != NULL) {
        PCB_t *data = iterator->data;
        iterator = iterator->next;

        // NOTE: This call to check_block auto-decrements the delay count
        if (check_block(data->block) == UNBLOCKED) {
          // Remove it from the blocked queue and add it to ready queue
          remove_from_list(blocked_procs, data);
          add_to_list(ready_procs, data, data->proc_id);
        }
    }

    // Check the last item in the list
    if (check_block(((PCB_t *)iterator->data)->block) == UNBLOCKED) {
      PCB_t *data = (PCB_t *) iterator->data;
      remove_from_list(blocked_procs, data);
      add_to_list(ready_procs, data, data->proc_id);
    } 
  }

  // Are there more processes waiting?
  TracePrintf(1, "Proc Id: %d\n", curr_proc->proc_id);
  if (count_items(ready_procs) > 0) { 
    TracePrintf(1, "Switching processes\n");
    switch_to_next_available_proc(uc, 1);
  } else { 
    TracePrintf(1, "No process to switch to or in proc 1- gonna keep going\n");
  }
  
  TracePrintf(1, "End: HANDLE_TRAP_CLOCK\n");
} 

/*

Results from execution of illegal instruction by 
currently executing user process. E.g. undefined 
machine language opcode, illegal addressing mode, 
or privileged instruction while not in kernel mode.

*/

void HANDLE_TRAP_ILLEGAL(UserContext *uc) {
  // Provide a Trace for the User
  TracePrintf(3, "Kernel Trap Handler: Illegal Instruction Exception (pid=%d)\n",
      curr_proc->proc_id);
  TracePrintf(3, "\tThe type of illegal instruction is %d\n", uc->code); 
  
  // Non-Obtrusively abort the process
  abort_current_process(ERROR, uc);
}

/*

Results from disallowed memory access by current 
user process. 

*/
void HANDLE_TRAP_MEMORY(UserContext *uc) { 
  TracePrintf(1, "Start: HANDLE_TRAP_MEMORY\n");
  TracePrintf(1, "addr %p .. sp %p .. pc %p \n", uc->addr, uc->sp, uc->pc);

  // Check if this is a permissions error
  if (uc->code == YALNIX_ACCERR) {
      // Trace for the User
      TracePrintf(1, "Process %d had a permissions error at addr %p\n",
              curr_proc->proc_id, uc->code);
      
      // Abort the process
      abort_current_process(ERROR, uc);

  } else if (uc->code == YALNIX_MAPERR) {
    // To decide whether this is a request to grow the stack or a genuine mapping
    // error, check if the offending address is b/t the break and the stack
    // pointer. If it is, assume, it's a request to grow the stack.
    if ((unsigned int)uc->addr < curr_proc->brk_addr || 
        (unsigned int)uc->addr > (unsigned int) uc->sp) {
      
      // Trace for the User
      TracePrintf(1, "Process %d had a mapping error \n", curr_proc->proc_id);
    
      // Abort the Process
      abort_current_process(ERROR, uc);
    }
  }

  
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

      // Abort the process
      abort_current_process(ERROR, uc);
  }

  /* Loop through each page in virtual memory and allocate a physical frame */
  for (i = addr_pg - 128; i <= top_stack_pg - 128; i++) {
      temp_ent = (curr_proc->region1_pt + i);

      // If there's no page allocated for this
      if (temp_ent->valid == (u_long) 0x0) {

        // Check that there are enough physical pages
        if (count_items(&FrameList) <= 0) { /* This counting is inefficient... */
            TracePrintf(3, "\tProcess %d requested more memory for the stack, but there are not enough physical frames\n",
                    curr_proc->proc_id);
            
            // Abort the Process
            abort_current_process(ERROR, uc);
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
  TracePrintf(1, "End: HANDLE_TRAP_MEMORY\n");
} 

/*

Results from division by 0, overflow, etc.

*/
void HANDLE_TRAP_MATH(UserContext *uc) { 
  // Trace for the user
  TracePrintf(3, "Kernel Trap Handler: Arithmetic Exception (PID:%d)\n",
      curr_proc->proc_id);
  TracePrintf(3, "\tType of Arithmetic Error: %d\n", uc->code);

  // Abort the offending process non-obtrusively
  abort_current_process(ERROR, uc);
} 

/*
Results from complete line of input being available 
from one of the terminals attached to system.
*/
void HANDLE_TRAP_TTY_RECEIVE(UserContext *uc) { 

  TracePrintf(1, "Start: Handle_trap_tty_receive\n");
  
  int id = uc->code; 
  ListNode *tty_node = find_by_id(ttys, id);
  TTY_t *tty = tty_node->data;
  
  buffer *new_buf = (buffer *)malloc(sizeof(buffer));
  new_buf->buf = (buffer *)malloc(TERMINAL_MAX_LINE*sizeof(char));
  new_buf->len = TtyReceive(tty->id, new_buf->buf, TERMINAL_MAX_LINE);
  
  // now that we've grabbed the text, let's store it: 
  add_to_list(tty->buffers, new_buf, 0);
  
  // if a reader is waiting, let's wake him/her up:
  List *readers = tty->readers;
  ListNode *waiter_node = pop(readers);
  PCB_t *waiter;
  if (waiter_node) { 
    waiter = waiter_node->data;
    remove_from_list(tty->readers, waiter);
    add_to_list(ready_procs, waiter, 0);
  } 

  /* 
  // handle case if reader wants to read less than 
  // what we have
  if (strlen(stored_buf->buf) > len) {   
    
    int leftover_length = strlen(stored_buf->buf) - len;
    char* leftover_char_buf;
    strncpy(leftover_char_buf, stored_buf->buf + len, leftover_length);
    buffer *leftover_buf = malloc(sizeof(buffer));
    leftover_buf->len = leftover_length;
    leftover_buf->buf = leftover_char_buf;
    add_to_list(tty->buffers, leftover_buf, 0);
    
  } else { 
    
    // only copy len chars, don't "overcopy"
    if (strlen(stored_buf->buf) < len) { 
      len = strlen(stored_buf->buf);
    }
  }
  */

  TracePrintf(1, "End: Handle_trap_tty_receive\n");
} 

/* 

Current buffer of data (prev. given to controller
on a TtyTransmit) has been completely sent to terminal.

*/
void HANDLE_TRAP_TTY_TRANSMIT(UserContext *uc) { 

  TracePrintf(1, "Start: Handle_trap_tty_transmit\n");
  
  int id = uc->code; 
  ListNode *tty_node = find_by_id(ttys, id);
  TTY_t *tty = tty_node->data;
  
  ListNode *writer_node = pop(tty->writers);
  PCB_t *writer = writer_node->data;
  
  add_to_list(ready_procs, writer, writer->proc_id);
  add_to_list(tty->buffers, writer->write_buf, 0);

  TracePrintf(1, "End: Handle_trap_tty_transmit\n");
} 

void HANDLE_TRAP_DISK(UserContext *uc) {
  // do things
}
