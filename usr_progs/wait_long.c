/*
 * BUG: weird stuff happens if
 */
int main(int argc, char *argv[]) {
  TracePrintf(1, "\t===>In wait_short.c\n");

  int pid;
  int rc;
  int i;
  pid = GetPid();
  int *stat_ptr = (int *) malloc(sizeof(int));

  TracePrintf(1, "\tForking inside parent (pid:%d)\n", pid);
  rc = Fork();
  if (rc == 0) {
    // CHILD
    TracePrintf(1, "\tCHILD: my rc = %d\n", rc);
    pid = GetPid();
    TracePrintf(1, "\tCHILD: my pid is %d\n", pid);
    for (i = 0; i < 5; i++) {
      TracePrintf(1, "\tCHILD: iteration %d\n", i);
      Pause();
    }
    TracePrintf(1, "\tCHILD: exiting with return value 420\n");
    Exit(420);
  
  } else { 
    // PARENT
    TracePrintf(1, "\tPARENT: child's pid is %d\n", rc);
    Pause();

    // Wait call should return automatically
    *stat_ptr = 0;
    TracePrintf(1, "PARENT about to wait for child\n");
    rc = Wait(stat_ptr);

    TracePrintf(1, 
        "\tPARENT: Wait Completed. Child %d returned with val %d\n",
        rc,
        *stat_ptr); 
    return(0);
  }

  TracePrintf(1, "\t===>Exiting wait_short.c\n");
  return(-1);
}
