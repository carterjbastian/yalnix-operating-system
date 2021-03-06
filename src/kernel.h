#ifndef _KERNEL_H_
#define _KERNEL_H_

/*
 * System Includes
 */
#include <hardware.h>
#include <yalnix.h>
/*
 * Local Includes
 */
#include "linked_list.h"
#include "PCB.h"
#include "traps.h"

/*
 * Constants
 */
#define VMEM_1_PAGE_COUNT     (((VMEM_1_LIMIT - VMEM_1_BASE) / PAGESIZE))
#define VMEM_0_PAGE_COUNT     (((VMEM_0_LIMIT - VMEM_0_BASE) / PAGESIZE))
#define KS_NPG  (KERNEL_STACK_MAXSIZE / PAGESIZE)

#define KILL    -1
#define SUCCESS 0

/*
 * Usefull Macros
 */
#define FNUM_TO_PFN(n) ((u_long) (((int)(n) * PAGESIZE) >> PAGESHIFT))
#define PFN_TO_FNUM(n) (((n) << PAGESHIFT) / PAGESIZE)

/*
 * Global Variables
 */

// Statically declared page table arrays                                        
struct pte r0_pagetable[VMEM_0_PAGE_COUNT];                                     
struct pte r1_pagetable[VMEM_1_PAGE_COUNT];  

// virtual memory
int vm_en;
void *kernel_data_start;
void *kernel_data_end;
void *kernel_brk;
unsigned int pframes_in_use;
unsigned int pframes_in_kernel;


// locks/cvars/pipes/ttys
int next_resource_id;
List *locks;
List *cvars;
List *pipes;
List *ttys;

// processes 
PCB_t *idle_proc; 
PCB_t *curr_proc;
unsigned int available_process_id;
unsigned int total_pframes;

// FrameList
List FrameList;

// process queues
List *ready_procs;
List *blocked_procs;
List *all_procs;
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

void *MyKCSClone(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p);
KernelContext *MyKCSSwitch(KernelContext *kc_in, void *curr_pcb_p, void *next_pcb_p);
int bufferIsValid(char *buff, int len, void* permissions);
#endif // _KERNEL_H_
