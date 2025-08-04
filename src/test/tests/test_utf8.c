#include <stdio.h>
#include <string.h>
#include "core/utf8.h"
#include "core/c_test.h"

/* ---------- 1. 解码正确性 ---------- */
static void test_decode(void)
{
    const struct {
        const char *bytes;   /* UTF-8 字节流 */
        uint32_t    cp;      /* 期望码点 */
    } tbl[] = {
        /* ASCII */
        {"A", 0x41},
        /* 中文 */
        {"中", 0x4E2D},
        {"文", 0x6587},
        {"，", 0xFF0C},      /* 全角逗号 */
        /* 日文 */
        {"ひ", 0x3072},      /* 平假名 */
        {"カ", 0x30AB},      /* 片假名 */
        {"漢", 0x6F22},      /* 日文汉字 */
        /* 基本长字符 */
        {"𠮷", 0x20BB7},     /* 中日韩扩展 B */
        /* Emoji */
        {"😀", 0x1F600},     /* 基础笑脸 */
        {"👨‍💻", 0x1F468},    /* 👨 为序列首码点，完整序列 1F468 200D 1F4BB */
        {"👨‍👩‍👧‍👦", 0x1F468}, /* 家庭 ZWJ 序列首码点 */
        /* 边界 */
        {"\x7F", 0x7F},
        {"\xC2\x80", 0x80},  /* U+0080 最小 2 字节 */
        {"\xDF\xBF", 0x7FF}, /* U+07FF 最大 2 字节 */
        {"\xE0\xA0\x80", 0x800},   /* U+0800 最小 3 字节 */
        {"\xEF\xBF\xBF", 0xFFFF},  /* U+FFFF 最大 3 字节 */
        {"\xF0\x90\x80\x80", 0x10000}, /* U+10000 最小 4 字节 */
        {"\xF4\x8F\xBF\xBF", 0x10FFFF} /* U+10FFFF 最大 4 字节 */
    };

    for (size_t i = 0; i < sizeof(tbl)/sizeof(tbl[0]); ++i) {
        const char *p = tbl[i].bytes;
        uint32_t got = utf8_decode(&p);
        ASSERT_EQ(got, tbl[i].cp);
    }
}

/* ---------- 2. 编码正确性 ---------- */
static void test_encode(void)
{
    const struct {
        uint32_t cp;
        const char *bytes;
        int      bytes_len;
    } tbl[] = {
        {0x41,    "A",          1},
        {0x4E2D,  "\xE4\xB8\xAD", 3},
        {0x1F600, "\xF0\x9F\x98\x80", 4},
        {0x20BB7, "\xF0\xA0\xAE\xB7", 4}
    };
    char buf[4];
    for (size_t i = 0; i < sizeof(tbl)/sizeof(tbl[0]); ++i) {
        int n = utf8_encode(tbl[i].cp, buf);
        ASSERT_EQ(n, tbl[i].bytes_len);
        ASSERT_EQ(memcmp(buf, tbl[i].bytes, n), 0);
    }
}

/* ---------- 3. 宽度计算 ---------- */
static void test_width(void)
{
    const struct {
        uint32_t cp;
        int      exp_w;
    } tbl[] = {
        {'A', 1},
        {0x4E2D, 2},   /* 中文 */
        {0x3072, 2},   /* 日文 */
        {0x1F600, 2},  /* Emoji */
        {0x20BB7, 2},  /* 扩展汉字 */
        {0x0020, 1},   /* 空格 */
        {0x007F, 0},   /* DEL 控制字符 */
        {0x0000, 0}    /* NUL */
    };
    for (size_t i = 0; i < sizeof(tbl)/sizeof(tbl[0]); ++i) {
        int w = utf8_width(tbl[i].cp);
        ASSERT_EQ(w, tbl[i].exp_w);
    }
}

/* ---------- 4. 字符串长度 ---------- */
static void test_len(void)
{
    ASSERT_EQ(utf8_len(""), 0);
    ASSERT_EQ(utf8_len("abc"), 3);
    ASSERT_EQ(utf8_len("中文"), 2);
    ASSERT_EQ(utf8_len("ひらがな"), 4);
    ASSERT_EQ(utf8_len("🙂😀"), 2);
}

/* ---------- 5. 字符串显示宽度 ---------- */
static void test_swidth(void)
{
    ASSERT_EQ(utf8_swidth("abc"), 3);
    ASSERT_EQ(utf8_swidth("中文"), 4);
    ASSERT_EQ(utf8_swidth("ひら"), 4);
    ASSERT_EQ(utf8_swidth("👨‍💻"), 2);   /* 虽然底层有多个码点，但首码点宽 2 */
}

/* ---------- 6. 非法/边界序列 ---------- */
static void test_validate(void)
{
    /* 合法 */
    ASSERT_EQ(utf8_valid("abc", 3), 0);
    ASSERT_EQ(utf8_valid("中文", 6), 0);
    /* 非法：半吊子 2 字节 */
    ASSERT_NE(utf8_valid("\xC2", 1), 0);
    /* 非法：尾随字节错误 */
    ASSERT_NE(utf8_valid("\xC2\x00", 2), 0);
    /* 非法：超长编码 (5 字节) */
    ASSERT_NE(utf8_valid("\xF8\x80\x80\x80\x80", 5), 0);
    /* 非法：代理区 */
    ASSERT_NE(utf8_valid("\xED\xA0\x80", 3), 0); /* U+D800 */
    /* 非法：超出 U+10FFFF */
    ASSERT_NE(utf8_valid("\xF4\x90\x80\x80", 4), 0);
}

/* ---------- 7. 组合注册 ---------- */
TEST(utf8, test)
{
    test_decode();
    test_encode();
    test_width();
    test_len();
    test_swidth();
    test_validate();
    puts("Extended UTF-8 tests passed.");
}