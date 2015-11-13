/*
 * File: Pipe.h
 *  Carter J. Bastian, KC Beard
 *  CS58 15F
 */

#ifndef _PIPE_H_
#define _PIPE_H_

// Maximum info stored in a pipe at one time is 1 KB
#define MAX_PIPE_LEN  1024

typedef struct pipe_t {
  int id;
  int len;
  char *buf;
  List *waiters;
} pipe_t;

#endif // _PIPE_H_
