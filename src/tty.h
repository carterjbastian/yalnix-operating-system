#ifndef _TTY_H_
#define _TTY_H_
#include "linked_list.h"

typedef struct buffer { 
  void *buf;  // starting logical addr
  int len; // bytelength
} buffer;


typedef struct tty { 
  int id; 
  List *readers; 
  List *writers; 
  List *buffers;
} tty;
  
#endif // _TTY_H_
