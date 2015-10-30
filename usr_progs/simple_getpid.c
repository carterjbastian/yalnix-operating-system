int main(int argc, char *argv[]) {
    TracePrintf(1, "\t===>In simple_getpid.c\n");
    int my_pid;
    while (1) {
	    my_pid = GetPid();
	    TracePrintf(1, "\t===>My pid is %d\n", my_pid);
	
	    TracePrintf(1, "\t===>Exiting simple_getpid.c\n");
	    Pause();
    }
    return 0;
}
