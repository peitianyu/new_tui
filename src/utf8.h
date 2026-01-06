#ifndef __UTF8_H__
#define __UTF8_H__

#include <stdint.h>

typedef struct {
    uint8_t bytes[4];
    uint8_t len;   /* 1~4 */
    uint8_t width; /* 0/1/2  显示列数 */
} utf8_t;

typedef struct { uint8_t min[3], max[3]; } range3_t;
static const range3_t wide_tbl[] = {
    {{0xE4,0xB8,0x80}, {0xE9,0xBE,0xA3}},   /* CJK 统一汉字  U+4E00..U+9FA3  */
    // {{0xE3,0x80,0x80}, {0xE3,0xBF,0xBF}},   /* CJK 扩展 / 日文 / 韩文  U+3000..U+30FF */
    // {{0xEF,0xBC,0x80}, {0xEF,0xBD,0xA6}},   /* 全角符号  U+FF00..U+FFE6 */
    // {{0xE2,0x96,0x80}, {0xE2,0x9F,0xBF}},   /* 几何图形 + 杂项符号 U+2500..U+27FF */
    // {{0xE2,0xAC,0x80}, {0xE2,0xBF,0xBF}},   /* 箭头符号 + 杂项符号 U+2B00..U+2FFF */
    // {{0xF0,0x9F,0x8C}, {0xF0,0x9F,0x9B}},   /* Emoji 1F300..1F6FF  */
    // {{0xF0,0x9F,0xA4}, {0xF0,0x9F,0xA7}},   /* Emoji 1F900..1F9FF  */
};
#define WIDE_N  (sizeof(wide_tbl)/sizeof(wide_tbl[0]))

static inline int utf8_cmp(const utf8_t a, const utf8_t b) {
    if(a.len != b.len) return 1;
    for (uint8_t i = 0; i < a.len; ++i) {
        if (a.bytes[i] != b.bytes[i]) return 1;
    }
    return 0;
}

static inline uint8_t utf8_width(const uint8_t *s, uint8_t len) {
    if (len == 1) {
        uint8_t c = s[0];
        return (c < 0x20 || (c >= 0x7F && c <= 0x9F)) ? 0 : 1;
    }
    if (len == 3) {
        uint32_t key = (s[0]<<16)|(s[1]<<8)|s[2];
        for (uint32_t i = 0; i < WIDE_N; ++i) {
            uint32_t min = (wide_tbl[i].min[0]<<16)|(wide_tbl[i].min[1]<<8)|wide_tbl[i].min[2];
            uint32_t max = (wide_tbl[i].max[0]<<16)|(wide_tbl[i].max[1]<<8)|wide_tbl[i].max[2];
            if (key >= min && key <= max) return 2;
        }
    }
    if (len == 4 && s[0]==0xF0) {
        uint32_t key = (s[1]<<16)|(s[2]<<8)|s[3];
        if (key >= 0x9F8C80 && key <= 0x9F9BBF) return 2;   /* 1F300..1F6FF */
        if (key >= 0x9FA480 && key <= 0x9FA7BF) return 2;   /* 1F900..1F9FF */
    }
    return 1;
}

static inline int str_to_utf8(const char *str, utf8_t *out, int max) {
    const uint8_t *s = (const uint8_t *)str;
    int cnt = 0;
    while (*s && cnt < max) {
        uint8_t b = *s, len;
        if      ((b & 0x80) == 0x00) len = 1;
        else if ((b & 0xE0) == 0xC0) len = 2;
        else if ((b & 0xF0) == 0xE0) len = 3;
        else if ((b & 0xF8) == 0xF0) len = 4;
        else return -1;               

        for (uint8_t i = 1; i < len; ++i)
            if ((s[i] & 0xC0) != 0x80) return -1;

        uint8_t *dst = out[cnt].bytes;
        for (uint8_t i = 0; i < len; ++i) dst[i] = s[i];
        out[cnt].len    = len;
        out[cnt].width  = utf8_width(s, len);
        ++cnt;
        s += len;
    }
    return cnt;
}

#endif /* __UTF8_H__ */