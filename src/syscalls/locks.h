// locks.h

int LockInit(int *lock_idp);
int Acquire(int lock_id);
int Release(int lock_id);
