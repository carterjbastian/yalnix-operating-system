#ifndef _TTY_H_
#define _TTY_H_
#include "linked_list.h"

typedef struct buffer { 
  void *buf;  // starting logical addr
  int len; // bytelength
} buffer;


typedef struct TTY_t { 
  int id; 
  List *readers; 
  List *writers; 
  List *buffers;
} TTY_t;
  
#endif // _TTY_H_
