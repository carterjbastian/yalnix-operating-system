// PCB.c

/*
 * System includes
 */
#include <hardware.h>

/*
 * Local includes
 */
#include "tty.h"
#include "PCB.h"
#include "kernel.h"
#include "blocks.h"

PCB_t *new_process(UserContext *uc) {   
  TracePrintf(1, "Start: new_process\n");
  
  // Allocate a new Process Control Block
  PCB_t *pcb = (PCB_t *) malloc( sizeof(PCB_t) );
  
  // Allocate a new UserContext for the PCB
  pcb->uc = (UserContext *) malloc( sizeof(UserContext) );

  // Allocate a new KernelContext for the PCB
  pcb->kc_p = (KernelContext *) malloc(sizeof(KernelContext));

  // Allocate a new Block for the PCB
  pcb->block = (block_t *) malloc( sizeof(block_t) );
  bzero((char *)pcb->block, sizeof(block_t) );

  // UserContext inherits the vector and code from 
  pcb->uc->vector = uc->vector;
  pcb->uc->code = uc->code;

  // Give it a new process ID
  pcb->proc_id = available_process_id; 
  available_process_id++;

  pcb->children = NULL;
  pcb->exited_children = NULL;
  pcb->parent = NULL;
//  pcb->delay_clock_ticks = 0;
  pcb->heap_base_page = 0;
  pcb->brk_addr = 0;
  pcb->kc_set = 0;

  pcb->write_buf = (buffer *)malloc(sizeof(buffer));
  pcb->write_buf->buf = NULL;
  pcb->write_buf->len = 0;
  
  TracePrintf(1, "End: new_process\n");

  return pcb;
}
