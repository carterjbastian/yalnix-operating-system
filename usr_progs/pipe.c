int main(int argc, char *argv[]) {
  TracePrintf(1, "\t===>pipe.c\n");
  int pipe_id;
  int rc;

  if (PipeInit(&pipe_id) != 0)
    TracePrintf(1, "Failed to initialize pipe\n");
  else
    TracePrintf(1, "Pipe Initialized. ID = %d\n", pipe_id);

  rc = Fork();
  if (rc == 0) {
    char *to_write = (char *)malloc(5 * sizeof(char));
    to_write[0] = 'a';
    to_write[1] = 'b';
    to_write[2] = 'c';
    to_write[3] = 'd';
    to_write[4] = 'e';

    char *second_write = (char *)malloc(2 * sizeof(char));
    second_write[0] = 'e';
    second_write[1] = 'f';

    int written;


    Pause();
    TracePrintf(1, "\tCHILD: about to write to the lock\n");
    written = PipeWrite(pipe_id, to_write, 4);
    TracePrintf(1, "\tCHILD: wrote %d characters to the lock\n", written);

    Pause();
    TracePrintf(1, "\tCHILD: pausing once\n");

    Pause();
    written = PipeWrite(pipe_id, second_write, 2);
    TracePrintf(1, "\tCHILD: wrote to the pipe again (%d chars)\n", written);

    Pause();
    Exit(0);
  } else {
    char *to_read = (char *)malloc(6 * sizeof(char));
    int read;
    Pause();
    Pause();
    TracePrintf(1, "\tPARENT: reading 6 chars from the pipe (before it's ready\n");
    read = PipeRead(pipe_id, to_read, 6);
    TracePrintf(1, "\tPARENT: read %d chars from the pipe. Results: %s\n", read, to_read);

    Pause();
    Exit(0);
  }

  return 0;
}
