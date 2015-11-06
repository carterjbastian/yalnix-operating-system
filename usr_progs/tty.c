int main(int argc, char *argv[]) { 
  
  TracePrintf(1, "\t===>Start: tty.c.c\n");
  char write_msg[] = "Hello World!";
  int size = sizeof(write_msg);
  
  int rc;
  rc = Fork();
  if (rc == 0) {
    TtyWrite(1, write_msg, size);
    TracePrintf(1, "\t===>Just wrote to terminal 1.\n");
    return 0;
  } else {
    char read_msg[10];
    size = sizeof(read_msg);
    TtyRead(2, read_msg, size);
    TracePrintf(1, "\t===>Just read from terminal 1: %s.\n", read_msg);
  }
  
  TracePrintf(1, "\t===>End: tty.c.c\n");
  
  return 0;
} 
