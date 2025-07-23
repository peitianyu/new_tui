#include "core/c_test.h"
#include "core/c_log.h"


TEST(hello, test)
{
    LOG_TEST("hello world %d %d %d", 1, 2, 3);
}