// locks.c

#include <pthread.h>   // for threads

int LockInit(int *lock_idp) { 
  // lock = init lock with lock_idpOB
  // locks.add(lock);
} 

int Acquire(int lock_id) { 
  // lock = find_lock(lock_id) ;
  // pthread_mutex_lock(&lock);
} 

int Release(int lock_id) { 
  // lock = find_lock(lock_id);
  // pthread_mutex_unlock(&mutex);
} 


