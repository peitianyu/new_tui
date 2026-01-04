#include "src/minitest.h"
#include "src/backtrace.h"

int main() {
    BT_INSTALL();

    RUN_ALL_TESTS();
    return 0;
}