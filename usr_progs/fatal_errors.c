/*
 * File: fatal_errors.c
 * 
 * Description:
 *
 * Tests the following:
 *    1. abort_current_process helper function in traps.c
 *    2. HANDLER_TRAP_ILLEGAL
 *    3. HANDLER_TRAP_MEMORY
 *    4. HANDLER_TRAP_MATH
 *
 * NOTE: this does not test each sub-type of error. This can and should be 
 *    improved, but it tests the basic functionality.
 * 
 * TO DO:
 *    - Figure out how to simulate a TRAP_ILLEGAL signal
 */

#include <hardware.h>
#include <string.h>

#define SUCCESS   0
#define ERROR     -1


int main(int argc, char *argv[]) {
  TracePrintf(1, "===>In fatal_errors.c\n");

  int rc;
  int i;
  int pid;

  char *uninitialized_str;
  char *defined_str = "This will cause an issue\n";
  char *illegal_ptr;
  char nonexistant;
  int positive = 5;
  int zero = 0;
  int quotient;

  rc = Fork();

  if (rc == 0) { // CHILD 1: ILLEGAL
    pid = GetPid();
    TracePrintf(1, "\tIssuing Illegal instruction (PID:%d)\n", pid);
    Pause();
    TracePrintf(1, "\tILLEGAL INSTRUCTION TEST IS NOT IMPLEMENTED\n"); 
    Exit(ERROR);

  } else { // Parent
    rc = Fork();
    if (rc == 0) { // CHILD 2: MEMORY TRAP
      pid = GetPid();
      TracePrintf(1, "\tAttempting to dereference a null pointer (PID:%d)\n", pid);
      Pause();
      // Memcpy into uninitialized string
      memcpy((void *) uninitialized_str, (void *) defined_str, 26);

      TracePrintf(1, "\tThis statement should never be printed\n");
      Exit(ERROR);

    } else { // Parent
      rc = Fork();
      if (rc == 0) { // CHILD 3: MATH TRAP
        pid = GetPid();
        TracePrintf(1, "\tTrying to divide by zero (PID:%d)\n", pid);
        Pause();
        quotient = positive / zero;

        TracePrintf(1, "\tThis statement should never be printed\n");
        Exit(ERROR);

      } else { // PARENT: Successful Completion
        pid = GetPid();
        Pause();
        // Waste some cycles so that we know the parent lived past its
        // irresponsible children
        for (i = 0; i < positive; i++) {
          TracePrintf(1, "\tParent Loop number %d\n", i);
          Pause();
        }

        // Successful Completion
        TracePrintf(1,
            "\tSuccessfully finished the non-problematic process (PID:%d)\n",
            pid);
        
        // Exit with success
        Exit(SUCCESS);
      }
    }
  }
}
