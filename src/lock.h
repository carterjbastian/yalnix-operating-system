#ifndef _LOCK_H_
#define _LOCK_H_

typedef struct LOCK_t { 
  int id; 
  int is_claimed; 
  int owner_id; 
  List *waiters;
} LOCK_t;

#endif 
