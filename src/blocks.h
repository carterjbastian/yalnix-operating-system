/*
 * File:  blocks.h 
 *    Carter J. Bastian & KC Beard
 *    CS58 15F, Yalnix Project
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

#ifndef _BLOCKS_H_
#define _BLOCKS_H_

/*
 * System includes
 */
#include <hardware.h>

/*
 * Public Constant Definitions
 */
// Activity constants
#define BLOCK_INACTIVE    ((u_long) 0x0)
#define BLOCK_ACTIVE      ((u_long) 0x1)

// Block type constants
#define NO_BLOCK          ((u_long) 0x0)
#define DELAY_BLOCK       ((u_long) 0x1)
#define WAIT_BLOCK        ((u_long) 0x2)
#define PIPE_BLOCK        ((u_long) 0x3)
#define TTY_READ_BLOCK    ((u_long) 0x4)
#define TTY_WRITE_BLOCK   ((u_long) 0x5)

// Block Action constants
#define UNBLOCKED         ((int) 0x0)
#define BLOCKED           ((int) 0x1)



/*
 * block_t datatype
 *
 * Used to define the bookkeeping datatype for blocking
 * a process.
 */
typedef struct block_t {
  u_long active;            // Is the block currently active?
  u_long type;              // What is the type of block?
  union {
    int delay_count;        // The count of the delay for a delayed process
    int ret_val;            // The value returned from the block if any
  } data;
  void *obj_ptr;            // A pointer to the relevant object for the block
                            //  type. EG. for waiting, it's a List *

} block_t;

/*
 * Public Prototypes
 */
int check_block(block_t *block);





#endif // _BLOCKS_H_
