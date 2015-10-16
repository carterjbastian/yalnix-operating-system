#include "PageTable.h"

struct PCB { 
  unsigned int proc_id;
  UserContext *uc; 
  struct pte *region0_pt;
  struct pte *region1_pt;
  struct list *children;
  struct list *exited_children;
  int *delay_clock_ticks;
};
  
struct PCB* new_process(UserContext *uc);

// Functions to work with the Process Control Blocks

