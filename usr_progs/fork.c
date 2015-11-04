int main(int argc, char *argv[]) {
  TracePrintf(1, "\t===>In fork.c\n");
  int pid;
  pid = GetPid();
  int rc;
  TracePrintf(1, "\t===>Parent PID is %d\n", pid);
  rc = Fork();
  if (rc == 0) {
    pid = GetPid();
    while(1) {
      TracePrintf(1, "\t===>Inside CHILD Process: my pid is %d!\n", pid);
      TracePrintf(1, "\t===>Exiting CHILD\n");
      Pause();
    }
  } else {
    while(1) {
      TracePrintf(1, "\t===>Inside PARENT Process: pid is %d, child is %d\n", pid, rc);
      TracePrintf(1, "\t===>Exiting PARENT\n");
      Pause();
    }
  }
  return 0;
}
