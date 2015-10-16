#include "PCB.h"
#include "kernel.h"

struct PCB * new_process(UserContext *uc) { 
  struct PCB *pcb = (struct PCB *)malloc( sizeof(struct PCB) );
  pcb->uc->vector = uc->vector;
  pcb->uc->code = uc->code;
  pcb->uc->addr = uc->addr;
  pcb->uc->pc = uc->pc;
  pcb->uc->sp = uc->pc;

  pcb->proc_id = available_process_id; 
  available_process_id++;
  return pcb;
  // assign ptes here? 
}
