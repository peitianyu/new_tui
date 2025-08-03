#include <stdio.h>
#include <stdlib.h>
#include "core/utf8.h"
#include "core/c_test.h"

static void test_decode(void)
{
    const char *s = "A\xC2\xA9\xE2\x98\x83\xF0\x9F\x98\x80"; /* A©☃😀 */
    uint32_t cp[4];
    const char *p = s;
    cp[0] = utf8_decode(&p);  /* 'A' */
    cp[1] = utf8_decode(&p);  /* '©' */
    cp[2] = utf8_decode(&p);  /* '☃' */
    cp[3] = utf8_decode(&p);  /* '😀' */
    ASSERT(cp[0] == 0x41 && cp[1] == 0xA9 &&
           cp[2] == 0x2603 && cp[3] == 0x1F600, "decode sequence");
}

static void test_encode(void)
{
    char buf[4];
    ASSERT(utf8_encode(0x1F600, buf) == 4 &&
           buf[0] == (char)0xF0 &&
           buf[1] == (char)0x9F &&
           buf[2] == (char)0x98 &&
           buf[3] == (char)0x80, "encode U+1F600");
}

static void test_width(void)
{
    ASSERT(utf8_width('A')   == 1, "ASCII width");
    ASSERT(utf8_width(0x4E2D) == 2, "CJK width");
    ASSERT(utf8_width(0x1F600) == 2, "Emoji width");
}

static void test_len(void)
{
    ASSERT(utf8_len("Hello") == 5, "ASCII len");
    ASSERT(utf8_len("你好") == 2, "CJK len");
    ASSERT(utf8_len("\xF0\x9F\x98\x80\xF0\x9F\x98\x81") == 2, "Emoji len");
}

static void test_swidth(void)
{
    ASSERT(utf8_swidth("ABC") == 3, "ASCII swidth");
    ASSERT(utf8_swidth("你好") == 4, "CJK swidth");
    ASSERT(utf8_swidth("A好🙂") == 5, "mixed swidth");
}

static void test_validate(void)
{
    ASSERT(utf8_validate("abc", 3) == 0, "valid");
    ASSERT(utf8_validate("\x80\x80", 2) != 0, "invalid");
    ASSERT(utf8_validate("\xC0\xAF", 2) != 0, "overlong");
}

TEST(utf8, test) {
    test_decode();
    test_encode();
    test_width();
    test_len();
    test_swidth();
    test_validate();
    puts("All tests passed.");
}