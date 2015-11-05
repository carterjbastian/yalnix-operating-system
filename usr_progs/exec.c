#include <string.h>

int main(int argc, char *argv[]) {
  int rc;
  int pid;
  pid = GetPid();
  int arg_count = 3;

  char *filename = (char *) malloc(21);
  strcpy(filename, "./usr_progs/new_prog");

  char *arg0 = (char *) malloc(9);
  strcpy(arg0, "new_prog");

  char *arg1 = (char *) malloc(5);
  strcpy(arg1, "arg1");

  char *arg2 = (char *) malloc(1);
  arg2 = '\0';
  
  char **args = (char **)malloc(sizeof(char *) * arg_count);
  args[0] = arg0;
  args[1] = arg1;
  args[2] = arg2;

  TracePrintf(1, "\tIn exec: My PID is %d\n", pid);
  TracePrintf(1, "\tIn Exec: About to exec into new_prog\n");
  
  rc = Exec(filename, args);
  
  TracePrintf(1, "\tOOPS: returned from Exec... rc = %d\n", rc);
  return 0;
}
