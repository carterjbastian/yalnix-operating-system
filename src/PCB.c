// PCB.c

/*
 * System includes
 */
#include <hardware.h>

/*
 * Local includes
 */
#include "PCB.h"
#include "kernel.h"


PCB_t *new_process(UserContext *uc) { 
  // Allocate a new Process Control Block
  PCB_t *pcb = (PCB_t *) malloc( sizeof(PCB_t) );

  // Allocate a new UserContext for the PCB
  pcb->uc = (UserContext *)malloc( sizeof(UserContext) );

  // UserContext inherits the vector and code from 
  pcb->uc->vector = uc->vector;
  pcb->uc->code = uc->code;
  pcb->kc_set = 0;
  // Give it a new process ID
  pcb->proc_id = available_process_id; 
  available_process_id++;
  pcb->kc_p = (KernelContext *)malloc(sizeof(KernelContext));
  // Default value for kc
  return pcb;
}
