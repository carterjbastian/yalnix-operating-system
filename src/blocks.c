/*
 * File: blocks.c
 * Carter J. Bastian & KC Beard
 * CS58 15F, Yalnix Project
 *
 * Description:
 *
 * Contents:
 *
 * To Do:
 *
 * Warnings:
 *
 */

/* System Includes */
#include <yalnix.h>

/* Local Includes */
#include "blocks.h"
#include "PCB.h"
#include "linked_list.h"
/*
 * Public Function Defitions
 */

/*
 * Function: check_block
 */
int check_block(block_t *block) {
  /*
   * Local Variables
   */
  unsigned int delay_clock_ticks;
  PCB_t *blocked_proc;

  /* If the block is inactive, return indicating UNBLOCKED */
  if (block->active == BLOCK_INACTIVE) {
    return(UNBLOCKED);
  } else if (block->active != BLOCK_ACTIVE) {
    TracePrintf(3, "Block has invalid value for 'active' field\n");
    return(ERROR);
  }


  switch(block->type) {

    // With no block, zero out block contents and return UNBLOCKED
    case NO_BLOCK :
      bzero((char *) block, sizeof(block_t));
      return(UNBLOCKED);
      break;

    // Check and decrement the length of the delay.
    // Modify active field and return as appropriate
    // Upon finishing, the block is zerod out (no data stored)
    case DELAY_BLOCK :
      delay_clock_ticks = (unsigned int) block->data.delay_count;
      
      if (delay_clock_ticks > 1) { // Still Counting down
        // Decrement the count and return BLOCKED
        block->data.delay_count--;
        return(BLOCKED);
      } else {
        // last decrement, so zero out the block but keep blocking this time
        bzero((char *) block, sizeof(block_t));
        return(UNBLOCKED);
      } 
      break;

    // Check if the process has a child that has updated since last
    // checked. If so, zero out the block and return UNBLOCKED
    case WAIT_BLOCK :
      blocked_proc = (PCB_t *) block->obj_ptr;

      if (blocked_proc->exited_children != NULL &&
          count_items(blocked_proc->exited_children) > 0) {
        bzero((char *)block, sizeof(block_t));
        return(UNBLOCKED);
      } else {
        return(BLOCKED);
      }
      break;

    case PIPE_BLOCK :
      break;

    case TTY_READ_BLOCK :
      break;

    case TTY_WRITE_BLOCK :
      break;

    default :
      TracePrintf(3, "Referenced Block has unrecognized 'type' field\n");
      return(ERROR);

  }

  // This should never be reached
  return ERROR; 

}
