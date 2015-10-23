#include "PCB.h"
#include "kernel.h"

/*
 * What does this function actually do?
 * Run through this step by step to figure it out...
 */
PCB_t *new_process(UserContext *uc) { 
  PCB_t *pcb = (PCB_t *) malloc( sizeof(PCB_t) );
  UserContext *new_uc = (UserContext *)malloc( sizeof(UserContext) );

  pcb->uc = new_uc;
  pcb->uc->vector = uc->vector;
  pcb->uc->code = uc->code;

  pcb->uc->pc = uc->pc;
  pcb->uc->sp = uc->sp;


  pcb->proc_id = available_process_id; 
  available_process_id++;
  return pcb;
}
