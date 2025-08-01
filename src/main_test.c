#include <stdio.h>
#include <execinfo.h>
#include <stdlib.h>
#include "signal.h"
#include "core/c_test.h"

static void print_backtrace() {
    void *buffer[100];
    int size = backtrace(buffer, 100);
    char **symbols = backtrace_symbols(buffer, size);

    printf("Backtrace:\n");
    for (int i = 0; i < size; i++) {
        printf("%s\n", symbols[i]);
    }

    free(symbols);

    exit(1);
}

void main() {
    signal(SIGSEGV, print_backtrace);
    signal(SIGINT, print_backtrace);
    signal(SIGABRT, print_backtrace);
    signal(SIGFPE, print_backtrace);
    signal(SIGILL, print_backtrace);
    signal(SIGBUS, print_backtrace);

    RUN_ALL_TESTS();
}