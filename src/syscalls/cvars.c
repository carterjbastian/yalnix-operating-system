// cvars.c

#include "syscalls.h"

int CvarInit(int *cvar_idp) { 
  // pthread_cond_t cvar = init with cvar_idp
  // cvars.add(cvar);
} 

int CvarSignal(int cvar_id) { 
  // cvar = find_in_list(cvar_id, cvars);
  // pthread_cond_signal(&cvar); 
} 

int CvarBroadcast(int cvar_id)  {
  // cvar = find_in_list(cvar_id, cvars);
  // pthread_cond_broadcast(&cvar); 
} 

int CvarWait(int cvar_id, int lock_id) { 
  // cvar = find_in_list(cvar_id, cvars)
  // lock = find_in_list(lock_id, locks)
  // pthread_cond_wait(&cvar1,&mutex);
}

