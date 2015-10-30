int main(int argc, char *argv[]) {
    TracePrintf("===> Entering brk.c\n");
    int j;
    int *i_arr;
    while (1) {
        i_arr = (int *) malloc(100 * sizeof(int));
        TracePrintf(1, "Allocated an integer array of 100 items\n");
        for (j = 0; j < 100; j++) {
            i_arr[j] = j * 2;
        }
        TracePrintf(1, "First and last values of mallocd array: %d, %d\n", i_arr[0], i_arr[99]);
        free(i_arr);
        TracePrintf(1, "Freed the mallocd array\n");
        Pause();
    }
    TracePrintf("===> Exiting brk.c\n");
    return 0;
}
