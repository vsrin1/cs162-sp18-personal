#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

#include "int_handler.h"

void int_handler(int sig) {
    pid_t pid = getpid();
    printf("SIGINT received by %i\n", pid);
    fflush(stdout);
    exit(sig);
}