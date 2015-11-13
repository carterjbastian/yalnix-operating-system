#ifndef _CVAR_H_
#define _CVAR_H_

typedef struct CVAR_t { 
  int id; 
  List *waiters; 
} CVAR_t;

#endif 
