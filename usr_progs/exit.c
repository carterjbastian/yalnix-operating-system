int main(int argc, char *argv[]) {
  TracePrintf(1, "\t\tIn exit.c\n");
  int pid;
  int rc;
  int i;

  pid = GetPid();

  TracePrintf(1, "\t\tParent PID: %d\n", pid);

  rc = Fork();

  if (rc == 0) {
    pid = GetPid();
    TracePrintf(1, "\t\tIn CHILD loop, PID = %d\n", pid);
    for (i = 0; i < 2; i++) {
      TracePrintf(1, "\t\tCHILD: iteration %d\n");
      Pause();
    }
    TracePrintf(1, "\t\tCHILD: looping done, now exiting.\n");
    Exit(rc);
  } else {
    TracePrintf(1, "\t\tIn PARENT loop, PID = %d, CHILD PID = %d\n", pid, rc);
    for (i = 0; i < 4; i++) {
      TracePrintf(1, "\t\tPARENT: iteration %d\n", i);
      Pause();
    }
    TracePrintf(1, "\t\tPARENT: Done Looping, now exiting.\n");
    Exit(rc);
  }
}
