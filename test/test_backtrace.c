#include "../src/backtrace.h"
#include "../src/minitest.h"

TEST(test, backtrace) {
    SetConsoleOutputCP(65001);
    
    int* p = NULL;
    *p = 42;
}