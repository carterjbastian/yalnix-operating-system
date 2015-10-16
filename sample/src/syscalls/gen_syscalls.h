// gen_syscalls.h

// Function prototypes
int Fork();

int Exec(char *filename, char **argvec);

void Exit(int status);

int Wait(int *status_ptr);

int GetPid();

int Brk(void *addr);

int Delay(int clock_ticks);

int Reclaim(int id);
