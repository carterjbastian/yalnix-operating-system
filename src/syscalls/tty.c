#include <hardware.h> // for TERMINAL_MAX_LINE

#include "syscalls.h"
int TtyWrite(char *msg) { 
  // verify that msg.len < TERMINAL_MAX_LINE
  return 0;
} 

char * TtyRead(unsigned long tty_id) { 
  return "test";
} 
