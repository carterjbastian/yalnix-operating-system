#ifndef _TRAPS_H_
#define _TRAPS_H_

// traps.h
#include <hardware.h>

typedef void (*trap_handler_func)();

// Interrupt Vector Table is an array of function pointers
// to 
trap_handler_func INTERRUPT_VECTOR_TABLE[TRAP_VECTOR_SIZE];


// Functions to handle traps as they arrive
void abort_process(int pid);
void HANDLE_TRAP_KERNEL(UserContext *uc);
void HANDLE_TRAP_CLOCK(UserContext *uc);
void HANDLE_TRAP_ILLEGAL(UserContext *uc);
void HANDLE_TRAP_MEMORY(UserContext *uc);
void HANDLE_TRAP_MATH(UserContext *uc);
void HANDLE_TRAP_TTY_RECEIVE(UserContext *uc);
void HANDLE_TRAP_TTY_TRANSMIT(UserContext *uc);
void HANDLE_TRAP_DISK(UserContext *uc);
#endif // _TRAPS_H_
