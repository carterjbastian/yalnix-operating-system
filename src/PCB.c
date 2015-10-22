#include "PCB.h"
#include "kernel.h"

struct PCB * new_process(UserContext *uc) { 
  struct PCB *pcb = (struct PCB *)malloc( sizeof(struct PCB) );
  UserContext *new_uc;
  pcb->uc = new_uc;
  pcb->uc->vector = uc->vector;
  pcb->uc->code = uc->code;

  pcb->uc->pc = uc->pc;
  pcb->uc->sp = uc->sp;

  pcb->proc_id = available_process_id; 
  available_process_id++;
  return pcb;
  // assign ptes here? 
}
