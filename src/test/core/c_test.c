#include "c_test.h"
#include <stdlib.h> 
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>

void Tester_Is(char* name, int line, int x, char* msg) {
    if (!x) { printf("\033[31m[      !OK ] %s:%d %s\033[0m\n", name, line, msg); }
}

typedef struct ContextTest {
    char* name;
    void (*test)();
}ContextTest;

char* m_run_name;
ContextTest m_tests[1024];
int m_tests_size = 0;

static unsigned long GetTime() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}


static void RunTest(const ContextTest* test) {
    printf("\033[32m[=======================================================================]\033[0m\n");
    printf("\033[32m[ RUN      ] %s\033[0m\n", test->name);
    unsigned long start_time = GetTime();
    test->test();
    printf("\033[32m[     PASS ] %s (%ld ms)\033[0m\n", test->name, (GetTime() - start_time));
    printf("\033[32m[=======================================================================]\033[0m\n");
}

void Register(char* name, void (*test)())
{
    if(!m_tests) m_tests_size = 0;
    m_tests[m_tests_size].name = name;
    m_tests[m_tests_size].test = test;
    m_tests_size++;
}

void RegisterJustRun(char* name) { m_run_name = name;}

void RunAllTests() {
    if (m_tests_size == 0) return;

    printf("\n===== Available Tests =====\n");
    for (int i = 0; i < m_tests_size; ++i) {
        printf("%2d : %s\n", i, m_tests[i].name);
    }
    printf("Enter the test number to run: ");
    fflush(stdout);

    char buf[32];
    if (!fgets(buf, sizeof(buf), stdin)) return;
    if (buf[0] == '\n') return;

    int idx = atoi(buf);
    if (idx < 0 || idx >= m_tests_size) {
        printf("Invalid number—exiting.\n");
        return;
    }
    RunTest(&m_tests[idx]);
}