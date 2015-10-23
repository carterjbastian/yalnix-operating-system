#ifndef _PCB_H_
#define _PCB_H_

#include "linked_list.h"
#include "PageTable.h"

typedef struct { 
  unsigned int proc_id;
  UserContext *uc; 
  struct pte *region0_pt;
  struct pte *region1_pt;
  List *children;
  List *exited_children;
  int *delay_clock_ticks;
} PCB_t;
  
PCB_t *new_process(UserContext *uc);

// Functions to work with the Process Control Blocks
#endif // _PCB_H_
