#ifndef __C_TEST_H__
#define __C_TEST_H__

void Tester_Is(char* name, int line, int x, char* msg);

#define ASSERT(x, msg) Tester_Is(__FILE__, __LINE__, (x), #x ":" msg)
#define ASSERT_TRUE(x) Tester_Is(__FILE__, __LINE__, (x), #x)
#define ASSERT_FALSE(x) Tester_Is(__FILE__, __LINE__, !(x), #x)
#define ASSERT_EQ(a, b) Tester_Is(__FILE__, __LINE__, (a) == (b), #a " == " #b)
#define ASSERT_NE(a, b) Tester_Is(__FILE__, __LINE__, (a) != (b), #a " != " #b)
#define ASSERT_LT(a, b) Tester_Is(__FILE__, __LINE__, (a) < (b), #a " < " #b)
#define ASSERT_LE(a, b) Tester_Is(__FILE__, __LINE__, (a) <= (b), #a " <= " #b)
#define ASSERT_GT(a, b) Tester_Is(__FILE__, __LINE__, (a) > (b), #a " > " #b)
#define ASSERT_GE(a, b) Tester_Is(__FILE__, __LINE__, (a) >= (b), #a " >= " #b)

void Register(char* name, void (*test)());
void RegisterJustRun(char* name);
void RunAllTests();

#define TEST(base, name) void base##name##_Test(); \
    __attribute__((constructor)) void base##name##_Test_Register() { Register(#base "." #name, base##name##_Test); } \
    void base##name##_Test()
#define RUN_ALL_TESTS() RunAllTests()

#endif // __C_TEST_H__