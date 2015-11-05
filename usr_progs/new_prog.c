int main(int argc, char *argv[]) {
  int pid;
  int i;
  pid = GetPid();
  TracePrintf(1, "\t\tMade it to new_prog!\n");

  TracePrintf(1, "\t\tNew_prog: argc = %d\n", argc);

  for (i = 0; i < argc; i++){
    TracePrintf(1, "\t\tnew_prog: argv[%d] = %s\n", i, argv[i]);
  }

  while (1) {
    TracePrintf(1, "\t\tIn new_prog: PID = %d\n", pid);
    Pause();
  }

  return 0;
}
