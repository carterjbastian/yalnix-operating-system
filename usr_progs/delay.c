int main(int argc, char *argv[]) {
    TracePrintf(1, "====>Entering delay.c\n");
    while (1) {
        TracePrintf(1, "About to Delay for 3 clock cycles!\n");
        Delay(3);
        TracePrintf(1, "Back from Delaying!\n");
    }
    return 0;
}
