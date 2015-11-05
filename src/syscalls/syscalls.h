#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

/*
 * Syscalls implemented in gen_syscalls.c
 */
int Yalnix_Fork(UserContext *uc);

int Yalnix_Exec(UserContext *uc);

void Yalnix_Exit(int status);

int Yalnix_Wait(int *status_ptr);

int Yalnix_GetPid();

int Yalnix_Brk(void *addr);

int Yalnix_Delay(UserContext *uc, int clock_ticks);

int Yalnix_Reclaim(int id);


/*
 * Syscalls implemented in locks.c
 */
int Yalnix_LockInit(int *lock_idp);

int Yalnix_Acquire(int lock_id);

int Yalnix_Release(int lock_id); 


/*
 * Syscalls implemented in cvars.c
 */
int Yalnix_CvarInit(int *cvar_idp);

int Yalnix_CvarSignal(int cvar_id);

int Yalnix_CvarBroadcast(int cvar_id);

int Yalnix_CvarWait(int cvar_id, int lock_id);


/*
 * Syscalls implemented in tty.c
 */
int Yalnix_TtyWrite(char *msg);

char * Yalnix_TtyRead(unsigned long tty_id);


/*
 * Syscalls implemented in pipes.c
 */
int Yalnix_PipeInit(int *pip_idp);

int Yalnix_PipeRead(int pipe_id, void *buf, int len);

int Yalnix_PipeWrite(int pipe_id, void *buf, int len);


#endif // end _SYSCALL_H_
