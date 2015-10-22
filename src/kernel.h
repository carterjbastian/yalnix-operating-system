// kernel globals 
#include <hardware.h>
// Constants
#define VMEM_1_PAGE_COUNT     (((VMEM_1_LIMIT - VMEM_1_BASE) / PAGESIZE))
#define VMEM_0_PAGE_COUNT     (((VMEM_0_LIMIT - VMEM_0_BASE) / PAGESIZE))

// virtual memory
int vm_en;
void *kernel_data_start;
void *kernel_data_end;
void *kernel_brk;
unsigned int pframes_in_use;
unsigned int pframes_in_kernel;

// locks/cvars/pipes
struct list *locks;
struct list *cvars;
struct list *pipes;

// processes 
struct PCB *curr_proc;
unsigned int available_process_id;
unsigned int total_pframes;
struct list *ready_processes;
struct list *processes;
struct list *terminated_processes;


void SetKernelData(void * _KernelDataStart, void *_KernelDataEnd);
void KernelStart(char *cmd_args[], 
                 unsigned int pmem_size,
                 UserContext *uctxtf); 
int SetKernelBrk(void * addr);
void DoIdle();
