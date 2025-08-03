#include "utf8.h"

/* ---------- 解码 ---------- */
uint32_t utf8_decode(const char **s)
{
    const unsigned char *p = (const unsigned char *)*s;
    uint32_t cp;

    if (p[0] < 0x80) {                    /* 0xxxxxxx */
        (*s)++;
        return p[0];
    } else if ((p[0] & 0xE0) == 0xC0) {   /* 110xxxxx */
        if ((p[1] & 0xC0) != 0x80) goto fail;
        cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
        if (cp < 0x80) goto fail;         /* overlong */
        (*s) += 2;
        return cp;
    } else if ((p[0] & 0xF0) == 0xE0) {   /* 1110xxxx */
        if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80) goto fail;
        cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
        if (cp < 0x800) goto fail;        /* overlong */
        if (cp >= 0xD800 && cp <= 0xDFFF) goto fail; /* surrogate */
        (*s) += 3;
        return cp;
    } else if ((p[0] & 0xF8) == 0xF0) {   /* 11110xxx */
        if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80 || (p[3] & 0xC0) != 0x80)
            goto fail;
        cp = ((p[0] & 0x07) << 18) | ((p[1] & 0x3F) << 12) |
             ((p[2] & 0x3F) << 6) | (p[3] & 0x3F);
        if (cp < 0x10000 || cp > 0x10FFFF) goto fail; /* overlong / out-of-range */
        (*s) += 4;
        return cp;
    }
fail:
    (*s)++;
    return 0xFFFD;
}

/* ---------- 编码 ---------- */
int utf8_encode(uint32_t cp, char dst[4])
{
    if (cp <= 0x7F) {
        dst[0] = (char)cp;
        return 1;
    } else if (cp <= 0x7FF) {
        dst[0] = (char)(0xC0 | (cp >> 6));
        dst[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    } else if (cp <= 0xFFFF) {
        dst[0] = (char)(0xE0 | (cp >> 12));
        dst[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        dst[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    } else if (cp <= 0x10FFFF) {
        dst[0] = (char)(0xF0 | (cp >> 18));
        dst[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        dst[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        dst[3] = (char)(0x80 | (cp & 0x3F));
        return 4;
    }
    return 0; /* invalid */
}

/* ---------- 宽度 ---------- */
/* 查表：0=不可见/控制，1=半角，2=全角 */
static const uint8_t width_table[256] = {
    /* 00-1F */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* 20-7F */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    /* 80-9F */ 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    /* A0-BF */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    /* C0-FF */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
};

int utf8_width(uint32_t cp)
{
    if (cp < 0x80)   return width_table[cp];
    if (cp < 0x300)  return 1;
    /* 东亚宽字符区 */
    if ((cp >= 0x1100 && cp <= 0x115F) ||
        (cp >= 0x2329 && cp <= 0x232A) ||
        (cp >= 0x2E80 && cp <= 0x303E) ||
        (cp >= 0x3040 && cp <= 0xA4CF) ||
        (cp >= 0xAC00 && cp <= 0xD7A3) ||
        (cp >= 0xF900 && cp <= 0xFAFF) ||
        (cp >= 0xFE10 && cp <= 0xFE19) ||
        (cp >= 0xFE30 && cp <= 0xFE6F) ||
        (cp >= 0xFF00 && cp <= 0xFF60) ||
        (cp >= 0xFFE0 && cp <= 0xFFE6) ||
        (cp >= 0x1F600 && cp <= 0x1F64F) || 
        (cp >= 0x20000 && cp <= 0x2FFFD) ||
        (cp >= 0x30000 && cp <= 0x3FFFD))
        return 2;
    return 1;
}

/* ---------- 辅助 ---------- */
size_t utf8_len(const char *s)
{
    size_t n = 0;
    while (*s) {
        utf8_decode(&s);
        n++;
    }
    return n;
}

int utf8_swidth(const char *s)
{
    int w = 0;
    while (*s)
        w += utf8_width(utf8_decode(&s));
    return w;
}

int utf8_validate(const char *s, size_t n)
{
    const char *end = s + n;
    while (s < end) {
        const char *p = s;
        uint32_t cp = utf8_decode(&p);
        if (cp == 0xFFFD && *s != 0xEF) /* 0xEF 可能是合法首字节 */
            return -1;
        s = p;
    }
    return 0;
}