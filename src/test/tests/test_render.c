#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "core/c_test.h"

/* ---------- 画布 ---------- */
typedef struct { int x, y, w, h; } rect_t;

typedef union {
    struct {
        uint32_t fg        : 4;
        uint32_t bg        : 4;
        uint32_t text      : 1;
        uint32_t rect      : 1;
        uint32_t border    : 1;
        uint32_t focus     : 2; // 0: none, 1: hover, 2: focus, 3: active
        uint32_t align_horz: 2;
        uint32_t align_vert: 2;
        uint32_t italic    : 1;
        uint32_t underline : 1;
        uint32_t bold      : 1;
        uint32_t strike    : 1;
    };
    uint32_t v;
} style_t;

typedef struct {
    uint32_t *buf;   
    style_t  *sty;
    int w, h;
} canvas_t;
static canvas_t g_canvas = {0};

/* ---------- UTF-8 <-> UTF-32 ---------- */
static inline uint32_t utf8_decode(const char **s) {
    const uint8_t *p = (uint8_t *)(*s);
    uint32_t c;
    if (*p < 0x80)          { c = *p;          (*s)++; }
    else if ((*p & 0xE0)==0xC0){ c=((*p&0x1F)<<6)|(p[1]&0x3F); (*s)+=2; }
    else if ((*p & 0xF0)==0xE0){ c=((*p&0x0F)<<12)|((p[1]&0x3F)<<6)|(p[2]&0x3F); (*s)+=3; }
    else if ((*p & 0xF8)==0xF0){ c=((*p&0x07)<<18)|((p[1]&0x3F)<<12)|((p[2]&0x3F)<<6)|(p[3]&0x3F); (*s)+=4; }
    else { (*s)++; return 0xFFFD; }
    return c;
}

/* ---------- 字宽 ---------- */
static inline int wcwidth_fast(uint32_t cp) {
    return ((cp >= 0x4E00 && cp <= 0x9FFF) ||
            (cp >= 0x3400 && cp <= 0x4DBF) ||
            (cp >= 0xF900 && cp <= 0xFAFF) ||
            (cp >= 0xFF01 && cp <= 0xFF5E)) ? 2 : 1;
}

/* ---------- 画布 API ---------- */
void canvas_init(int w, int h) {
    g_canvas.w = w; g_canvas.h = h;
    g_canvas.buf = calloc(w * h, sizeof(uint32_t));
    g_canvas.sty = calloc(w * h, sizeof(style_t));
    for (int i = 0; i < w * h; ++i) g_canvas.buf[i] = ' ';
}

void canvas_free(void) {
    free(g_canvas.buf); free(g_canvas.sty); g_canvas = (canvas_t){0};
}

/* ---------- 裁剪 ---------- */
static inline rect_t clip(rect_t r) {
    int x0 = r.x, y0 = r.y, x1 = x0 + r.w, y1 = y0 + r.h;
    if (x0 < 0) x0 = 0; if (x0 > g_canvas.w) x0 = g_canvas.w;
    if (x1 < 0) x1 = 0; if (x1 > g_canvas.w) x1 = g_canvas.w;
    if (y0 < 0) y0 = 0; if (y0 > g_canvas.h) y0 = g_canvas.h;
    if (y1 < 0) y1 = 0; if (y1 > g_canvas.h) y1 = g_canvas.h;
    return (rect_t){x0, y0, x1 - x0, y1 - y0};
}

/* ---------- 绘制 ---------- */
void canvas_draw(rect_t r_orig, const char *utf8, style_t st) {
    rect_t r = clip(r_orig);
    if (r.w <= 0 || r.h <= 0) return;

    /* 背景填充 */
    if (st.rect) {
        if (st.focus == 1) st.bg = 3;
        else if (st.focus == 2) st.bg = 4;

        for (int y = r.y; y < r.y + r.h; ++y)
        for (int x = r.x; x < r.x + r.w; ++x) {
            int i = y * g_canvas.w + x;
            g_canvas.sty[i] = st;
            g_canvas.buf[i] = ' ';
        }
    }

    /* 边框 */
    if (st.border && r_orig.w >= 2 && r_orig.h >= 2) {
        int x0 = r_orig.x, y0 = r_orig.y, x1 = x0 + r_orig.w - 1, y1 = y0 + r_orig.h - 1;
        if (x0 < 0 || x1 >= g_canvas.w || y0 < 0 || y1 >= g_canvas.h) goto skip_border; /* 超出不画 */
        for (int x = x0; x <= x1; ++x) { g_canvas.buf[y0*g_canvas.w+x] = 0x2500; g_canvas.sty[y0*g_canvas.w+x]=st; 
                                         g_canvas.buf[y1*g_canvas.w+x]=0x2500; g_canvas.sty[y1*g_canvas.w+x]=st; }
        for (int y = y0; y <= y1; ++y) { g_canvas.buf[y*g_canvas.w+x0]=0x2502; g_canvas.sty[y*g_canvas.w+x0]=st; 
                                         g_canvas.buf[y*g_canvas.w+x1]=0x2502; g_canvas.sty[y*g_canvas.w+x1]=st; }
        g_canvas.buf[y0*g_canvas.w+x0]=0x250C; g_canvas.buf[y0*g_canvas.w+x1]=0x2510;
        g_canvas.buf[y1*g_canvas.w+x0]=0x2514; g_canvas.buf[y1*g_canvas.w+x1]=0x2518;
    skip_border:;
    }

    if (!st.text || !utf8) return;

    /* 可用区域 */
    int bw = st.border ? 1 : 0;
    int x0 = r.x + bw, y0 = r.y + bw;
    int usable_w = r_orig.w - 2 * bw, usable_h = r_orig.h - 2 * bw;
    if (usable_w <= 0 || usable_h <= 0) return;

    /* 统计行 */
    typedef struct { const char *s; int w; } line_t;
    line_t lines[64]; int n = 0;
    for (const char *p = utf8; *p && n < 64;) {
        lines[n].s = p;
        lines[n].w = 0;
        while (*p && *p != '\n') {
            uint32_t cp = utf8_decode(&p);
            lines[n].w += wcwidth_fast(cp);
        }
        ++n;
        if (*p == '\n') ++p;
    }

    /* 垂直偏移 */
    int start_y = y0;
    if (st.align_vert == 1) start_y += (usable_h - n) / 2;
    if (st.align_vert == 2) start_y +=  usable_h - n;

    /* 逐行绘制 */
    for (int ly = 0; ly < n && start_y + ly < y0 + usable_h; ++ly) {
        int start_x = x0;
        if (st.align_horz == 1) start_x += (usable_w - lines[ly].w) / 2;
        if (st.align_horz == 2) start_x +=  usable_w - lines[ly].w;

        const char *p = lines[ly].s;
        int cx = start_x;
        while (*p && *p != '\n') {
            uint32_t cp = utf8_decode(&p);
            int cw = wcwidth_fast(cp);
            if (cx >= x0 && cx < x0 + usable_w && start_y + ly < y0 + usable_h) {
                int i = (start_y + ly) * g_canvas.w + cx;
                g_canvas.buf[i] = cp;
                g_canvas.sty[i] = st;
                if (cw == 2 && cx + 1 < x0 + usable_w) {
                    g_canvas.buf[i + 1] = 0;
                    g_canvas.sty[i + 1] = st;
                }
            }
            cx += cw;
        }
    }
}

/* ---------- 终端输出 ---------- */
static const char *FG[16] = {
    "\033[30m","\033[34m","\033[35m","\033[32m",
    "\033[31m","\033[90m","\033[37m","\033[97m",
    "\033[91m","\033[33m","\033[93m","\033[92m",
    "\033[36m","\033[95m","\033[96m","\033[94m"
};
static const char *BG[16] = {
    "\033[40m","\033[44m","\033[45m","\033[42m",
    "\033[41m","\033[100m","\033[47m","\033[107m",
    "\033[101m","\033[43m","\033[103m","\033[102m",
    "\033[106m","\033[105m","\033[106m","\033[104m"
};

static inline void print_cell(int i) {
    uint32_t cp = g_canvas.buf[i];
    style_t  st = g_canvas.sty[i];
    char buf[5];
    if (cp == 0) return;         
    if (cp < 0x80) { buf[0] = cp; buf[1] = 0; }
    else { /* 只处理 BMP，简单一点 */
        buf[0] = 0xE0 | (cp >> 12);
        buf[1] = 0x80 | ((cp >> 6) & 0x3F);
        buf[2] = 0x80 | (cp & 0x3F);
        buf[3] = 0;
    }
    printf("%s%s%s\033[0m", BG[st.bg], FG[st.fg], buf);
}

void canvas_flush(void) {
    for (int y = 0; y < g_canvas.h; ++y) {
        for (int x = 0; x < g_canvas.w; ++x) print_cell(y * g_canvas.w + x);
        putchar('\n');
    }
}


// ☑ ☐
/* ---------- 测试代码不变 ---------- */
TEST(render, test)
{
    canvas_init(50, 12);

    style_t st1 = {
        .fg = 7, .bg = 3, .rect = 1, .border = 1, .text = 1,
        .align_horz = 1, .align_vert = 1
    };
    canvas_draw((rect_t){10, 2, 30, 4},
                "Hello 世界!☑ ☐ ▶ ◀ \nCentered", st1);

    style_t st2 = {
        .fg = 8, .bg = 2, .rect = 1, .text = 1, .focus = 0,
        .align_horz = 0, .align_vert = 1
    };
    canvas_draw((rect_t){5, 7, 15, 3},
                "This text is too long", st2);

    canvas_flush();
    canvas_free();
}