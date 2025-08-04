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
int utf8_width(uint32_t cp)
{
    /* 0x00–0x7F：直接查表 */
    static const uint8_t w0[128] = {
        /* 00–1F：控制字符宽 0 */
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,
        /* 20–7F：其余 ASCII 宽 1 */
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,
        1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,0
    };
    if (cp < 0x80) return w0[cp];

    /* 零宽格式控制字符 */
    if (cp == 0x200B || cp == 0x200C || cp == 0x200D || cp == 0xFE0F)
        return 0;

    /* 0x80–0x2FF：统一按 1 处理 */
    if (cp < 0x300) return 1;

    static const struct { uint32_t lo, hi; } wide[] = {
        {0x1100,0x115F}, {0x2329,0x232A}, {0x2E80,0x303E},
        {0x3040,0xA4CF}, {0xAC00,0xD7A3}, {0xF900,0xFAFF},
        {0xFE10,0xFE19}, {0xFE30,0xFE6F}, {0xFF00,0xFF60},
        {0xFFE0,0xFFE6}, {0x1F600,0x1F64F},
        {0x20000,0x2FFFD}, {0x30000,0x3FFFD}
    };
    size_t l = 0, r = sizeof(wide)/sizeof(wide[0]);
    while (l < r) {
        size_t m = (l + r) >> 1;
        if (cp > wide[m].hi)      l = m + 1;
        else if (cp < wide[m].lo) r = m;
        else                      return 2;
    }
    return 1;
}

size_t utf8_len(const char *s)
{
    size_t n = 0;
    while (*s) {
        utf8_decode(&s);
        n++;
    }
    return n;
}

int utf8_swidth_len(const char *s, size_t byte_len) {
    int w = 0;
    const char *end = s + byte_len;
    while (s < end) {
        const char *old = s;
        uint32_t cp = utf8_decode(&s);
        w += utf8_width(cp);
    }
    return w;
}

int utf8_swidth(const char *s)
{
    int w = 0;
    while (*s)
        w += utf8_width(utf8_decode(&s));
    return w;
}

int utf8_valid(const char *s, size_t n)
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

size_t utf8_chr_len(const char *s)
{
    const unsigned char *p = (const unsigned char *)s;
    if (p[0] < 0x80) return 1;
    if ((p[0] & 0xE0) == 0xC0) return 2;
    if ((p[0] & 0xF0) == 0xE0) return 3;
    if ((p[0] & 0xF8) == 0xF0) return 4;
    return 1; /* invalid -> 1 byte skip */
}

size_t utf8_prev(const char *s, size_t cursor)
{
    if (cursor == 0) return 0;
    size_t pos = cursor;
    do { --pos; } while (pos > 0 && (s[pos] & 0xC0) == 0x80);
    return pos;
}

size_t utf8_advance(const char *s, size_t cursor, int target_width) {
    const char *p = s + cursor;
    int width = 0;

    while (*p && width < target_width) {
        uint32_t cp = utf8_decode(&p);
        width += utf8_width(cp);
    }

    return p - s;
}

size_t utf8_trunc_width(const char *s, int max_w)
{
    int w = 0;
    const char *p = s;
    while (*p) {
        const char *old = p;
        uint32_t cp = utf8_decode(&p);
        int cw = utf8_width(cp);
        if (w + cw > max_w) return old - s;
        w += cw;
    }
    return p - s;
}
