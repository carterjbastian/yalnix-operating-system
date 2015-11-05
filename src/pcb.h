#ifndef _PCB_H_
#define _PCB_H_

#include "linked_list.h"
//#include "PageTable.h"

/*
 * Type Definitions and Structures
 */
typedef struct PCB_t { 
  // Initialized and/or allocated in new_process()
  unsigned int proc_id;
  UserContext *uc;      // Be sure to copy in Fork 
  KernelContext *kc_p;


  // Must be allocted when creating the process
  struct pte *region0_pt;
  struct pte *region1_pt;

  // Initialized to 0 or null
  List *children;         // Allocate in Fork
  List *exited_children;  // Allocate in Fork
  struct PCB_t *parent;          // A pointer to this process' parent
  int delay_clock_ticks;  // Set upon delay
  int heap_base_page;
  unsigned int brk_addr;
  int kc_set;             // Set to 1 after a MyKCSClone call
} PCB_t;
  
/*
 * Function Declarations
 */
PCB_t *new_process(UserContext *uc);

// Functions to work with the Process Control Blocks
#endif // _PCB_H_
