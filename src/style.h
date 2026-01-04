#ifndef __STYLE_H__
#define __STYLE_H__

#include <stdio.h>
#include <string.h>

typedef struct {
    int fg, bg;                 /* 24-bit color: 0xRRGGBB, -1 表示默认 */
    union {
        int raw;
        struct {
            unsigned bold      : 1;
            unsigned underline : 1;
            unsigned blink     : 1;
            unsigned reverse   : 1;
            unsigned strike    : 1;
            unsigned italic    : 1;
        };
    };
} style_t;

static inline int style_color(char *dst, int code, int is_bg) {
    return (code < 0) ? 0 : sprintf(dst, "%d;2;%d;%d;%d", is_bg ? 48 : 38,
                            (code>>16)&0xFF, (code>>8)&0xFF, code&0xFF);
}

static inline int style_cmp(style_t a, style_t b) {
    return (a.fg != b.fg) || (a.bg != b.bg) || (a.raw != b.raw); 
}

static inline char* style_to_string(style_t s) {
    if (!style_cmp(s, (style_t){-1,-1,0})) return "\x1b[0m\0";
    static char seq[256];
    char *p = seq;
    
    *p++ = '\x1b';
    *p++ = '[';
    p += style_color(p, s.fg, 0);
    if (s.fg >= 0 && s.bg >= 0) *p++ = ';';
    p += style_color(p, s.bg, 1);
    const struct { int bit; int code; } attr[] = {
        {s.bold,      1}, {s.italic,    3}, {s.underline, 4},
        {s.blink,     5}, {s.reverse,   7}, {s.strike,    9}, };
    for (size_t i = 0; i < sizeof(attr)/sizeof(attr[0]); ++i) {
        if (!attr[i].bit) continue;
        if (p != seq + 2) *p++ = ';';   
        p += sprintf(p, "%d", attr[i].code);
    }
    *p++ = 'm';
    *p   = '\0';
    return seq;
}

#endif /* __STYLE_H__ */