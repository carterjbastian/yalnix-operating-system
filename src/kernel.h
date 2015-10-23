#ifndef _KERNEL_H_
#define _KERNEL_H_

/*
 * System Includes
 */
#include <hardware.h>

/*
 * Local Includes
 */
#include "linked_list.h"
#include "PCB.h"

/*
 * Constants
 */
#define VMEM_1_PAGE_COUNT     (((VMEM_1_LIMIT - VMEM_1_BASE) / PAGESIZE))
#define VMEM_0_PAGE_COUNT     (((VMEM_0_LIMIT - VMEM_0_BASE) / PAGESIZE))


/*
 * Global Variables
 */
// virtual memory
int vm_en;
void *kernel_data_start;
void *kernel_data_end;
void *kernel_brk;
unsigned int pframes_in_use;
unsigned int pframes_in_kernel;

// locks/cvars/pipes
List *locks;
List *cvars;
List *pipes;

// processes 
PCB_t *curr_proc;
unsigned int available_process_id;
unsigned int total_pframes;

// process queues
List *ready_procs;
List *blocked_procs;
List *procs;
List *dead_procs;


/*
 * Public Function Declarations
 */

void SetKernelData(void * _KernelDataStart, void *_KernelDataEnd);

void KernelStart(char *cmd_args[], 
                 unsigned int pmem_size,
                 UserContext *uctxtf); 

int SetKernelBrk(void *addr);

void DoIdle();

#endif // _KERNEL_H_
