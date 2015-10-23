// cvars.h

int CvarInit(int *cvar_idp);
int CvarSignal(int cvar_id);
int CvarBroadcast(int cvar_id);
int CvarWait(int cvar_id, int lock_id);
