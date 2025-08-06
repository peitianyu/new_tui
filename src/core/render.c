#include "render.h"
#include "color_palette.h"

/* ---------- 画布 API ---------- */
static canvas_t g_canvas = {0};
static canvas_t g_last_canvas = {0};
void canvas_init(int w, int h) {
    g_canvas.w = w; g_canvas.h = h;
    g_canvas.buf = calloc(w * h, sizeof(uint32_t));
    g_canvas.sty = calloc(w * h, sizeof(style_t));
    memset(g_canvas.buf, ' ', w * h * sizeof(uint32_t));

    g_last_canvas.w = w; g_last_canvas.h = h;
    g_last_canvas.buf = calloc(w * h, sizeof(uint32_t));
    g_last_canvas.sty = calloc(w * h, sizeof(style_t));
    for (int i = 0; i < w * h; ++i) g_last_canvas.buf[i] = 0;
}

void canvas_free(void) {
    free(g_canvas.buf); free(g_canvas.sty); g_canvas = (canvas_t){0};
}

void canvas_clear(void) {
    for (int i = 0; i < g_canvas.w * g_canvas.h; ++i) g_canvas.buf[i] = ' ';
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
/* 1. 背景填充 ------------------------------------------------------------- */
static void draw_rect(rect_t r_clip, style_t st) {
    if (!st.rect) return;

    for (int y = r_clip.y; y < r_clip.y + r_clip.h; ++y)
    for (int x = r_clip.x; x < r_clip.x + r_clip.w; ++x) {
        int i = y * g_canvas.w + x;
        g_canvas.sty[i] = st;
        g_canvas.buf[i] = ' ';
    }
}

/* 2. 边框 ----------------------------------------------------------------- */
typedef enum {
    BORDER_LIGHT=0, /* ─ │ ┌ ┐ └ ┘ */
    BORDER_HEAVY,   /* ═ ║ ╔ ╗ ╚ ╝ */
    BORDER_DASHED,  /* ┄ ┆ ┌ ┐ └ ┘ */
    BORDER_ROUND,   /* ─ │ ╭ ╮ ╰ ╯ */
} border_style_t;

typedef struct {
    uint32_t horz, vert, tl, tr, bl, br;
} border_chars_t;

static const border_chars_t g_border_tbl[] = {
    [BORDER_LIGHT]  = {0x2500, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518},
    [BORDER_HEAVY]  = {0x2550, 0x2551, 0x2554, 0x2557, 0x255A, 0x255D},
    [BORDER_DASHED] = {0x2504, 0x2506, 0x250C, 0x2510, 0x2514, 0x2518},
    [BORDER_ROUND]  = {0x2500, 0x2502, 0x256D, 0x256E, 0x2570, 0x256F},
};

static void draw_border(rect_t r, style_t st) {
    if (!st.border || r.w < 2 || r.h < 2) return;

    int x0 = r.x, y0 = r.y;
    int x1 = x0 + r.w - 1, y1 = y0 + r.h - 1;
    if (x0 < 0 || x1 >= g_canvas.w || y0 < 0 || y1 >= g_canvas.h) return;

    const int stride = g_canvas.w;
    uint32_t *restrict buf = g_canvas.buf;
    style_t  *restrict sty = g_canvas.sty;

    /* 取出当前样式字符 */
    int bs = st.border_st % (sizeof(g_border_tbl)/sizeof(g_border_tbl[0]));
    const border_chars_t b = g_border_tbl[bs];

    /* 四个角 */
    buf[y0 * stride + x0] = b.tl;
    buf[y0 * stride + x1] = b.tr;
    buf[y1 * stride + x0] = b.bl;
    buf[y1 * stride + x1] = b.br;

    /* 上边和下边 */
    for (int x = x0 + 1; x < x1; ++x) {
        const int top = y0 * stride + x;
        const int bot = y1 * stride + x;
        buf[top] = b.horz; sty[top] = st;
        buf[bot] = b.horz; sty[bot] = st;
    }

    /* 左边和右边 */
    for (int y = y0 + 1; y < y1; ++y) {
        const int lef = y * stride + x0;
        const int rig = y * stride + x1;
        buf[lef] = b.vert; sty[lef] = st;
        buf[rig] = b.vert; sty[rig] = st;
    }
}

/* 3. 文本 ----------------------------------------------------------------- */
static void draw_text(rect_t r_orig, const char *utf8, style_t st)
{
    if (!st.text || !utf8) return;

    const int bw = st.border ? 1 : 0;
    const int ux = r_orig.x + bw;
    const int uy = r_orig.y + bw;
    const int uw = r_orig.w - 2 * bw;
    const int uh = r_orig.h - 2 * bw;
    if (uw <= 0 || uh <= 0) return;

    /* ---------- 1. 把 UTF-8 拆成 codepoint + 宽度 ---------- */
    uint32_t cps[1024];
    int      wds[1024];
    int      cnt = 0;
    for (const char *p = utf8; *p && cnt < 1024;) {
        uint32_t cp = utf8_decode(&p);
        cps[cnt] = cp;
        wds[cnt] = utf8_width(cp);
        ++cnt;
    }

    /* ---------- 2. 排版成行 ---------- */
    typedef struct { int start, len, vis; } line_t;
    line_t lines[64];
    int    lc = 0;

    int i = 0;
    while (i < cnt && lc < 64) {
        int s = i, vis = 0, lastsp = -1;

        /* 不换行模式：硬截断到可用宽度 */
        if (!st.wrap) {
            while (i < cnt && cps[i] != '\n' && vis + wds[i] <= uw) {
                vis += wds[i];
                ++i;
            }
            lines[lc++] = (line_t){s, i - s, vis};
            if (i < cnt && cps[i] == '\n') ++i;   /* 吃掉显式换行 */
            continue;
        }

        /* 自动换行模式：按单词边界软换行 */
        while (i < cnt) {
            if (cps[i] == '\n') { ++i; break; }
            if (cps[i] == ' ') lastsp = i;
            if (vis + wds[i] > uw) {
                if (lastsp >= 0 && lastsp > s) {   /* 退格到空格 */
                    i = lastsp + 1;
                    vis = 0;
                    for (int k = s; k < lastsp; ++k) vis += wds[k];
                }
                break;
            }
            vis += wds[i++];
        }
        lines[lc++] = (line_t){s, i - s, vis};
        while (i < cnt && (cps[i] == ' ' || cps[i] == '\n')) ++i; /* 吃掉空格/换行 */
    }

    /* ---------- 3. 垂直对齐 ---------- */
    int y0 = uy;
    if (st.align_vert == 1) y0 += (uh - lc) / 2;
    if (st.align_vert == 2) y0 += (uh - lc);

    /* ---------- 4. 逐行绘制 ---------- */
    for (int ln = 0; ln < lc && y0 + ln < uy + uh; ++ln) {
        int x0 = ux;
        if (st.align_horz == 1) x0 += (uw - lines[ln].vis) / 2;
        if (st.align_horz == 2) x0 += (uw - lines[ln].vis);

        for (int k = 0; k < lines[ln].len; ++k) {
            int idx = lines[ln].start + k;
            uint32_t cp = cps[idx];
            int w = wds[idx];
            if (x0 + w > ux + uw) break;
            int pos = (y0 + ln) * g_canvas.w + x0;
            g_canvas.buf[pos] = cp;
            g_canvas.sty[pos] = st;
            if (w == 2 && x0 + 1 < ux + uw) {
                g_canvas.buf[pos + 1] = 0;
                g_canvas.sty[pos + 1] = st;
            }
            x0 += w;
        }
    }
}

void canvas_draw(rect_t r_orig, const char *utf8, style_t st) {
    rect_t r = clip(r_orig);
    if (r.w <= 0 || r.h <= 0) return;

    draw_rect(r, st);           /* 1. 背景 */
    draw_border(r_orig, st);    /* 2. 边框 */
    draw_text(r_orig, utf8, st);/* 3. 文本 */
}

static inline void render_cell_fast(int idx, char **p, int with_pos) {
    uint32_t cp = g_canvas.buf[idx];
    if (!cp) return;

    style_t st = g_canvas.sty[idx];
    char  ch[4];
    int   ch_len = utf8_encode(cp, ch);
    if (!ch_len) return;

    char *dst = *p;
    if (with_pos) {
        int y = idx / g_canvas.w + 1;
        int x = idx % g_canvas.w + 1;
        dst += sprintf(dst, "\e[%d;%dH", y, x);
    }

    if (st.italic)    { memcpy(dst, "\e[3m", 4);  dst += 4; }
    if (st.underline) { memcpy(dst, "\e[4m", 4);  dst += 4; }
    if (st.bold)      { memcpy(dst, "\e[1m", 4);  dst += 4; }
    if (st.strike)    { memcpy(dst, "\e[9m", 4);  dst += 4; }
    
    int fg = (st.border) ? st.border_fg : st.fg;
    memcpy(dst, RENDER_FG[fg], RENDER_FG_LEN[fg]);       dst += RENDER_FG_LEN[fg];
    memcpy(dst, RENDER_BG[st.bg], RENDER_BG_LEN[st.bg]); dst += RENDER_BG_LEN[st.bg];
    memcpy(dst, ch, ch_len);               dst += ch_len;
    memcpy(dst, "\e[0m", 4);               dst += 4;

    *p = dst;
}

/* ---------- 增量刷新 ---------- */
#define MAX_CELL_SIZE (64) // !注意: 渲染应该不超过这个数值
void canvas_flush(void) {
    const int N = g_canvas.w * g_canvas.h;
    
    int changes = 0;
    for (int i = 0; i < N; ++i) 
        changes += (g_canvas.buf[i] != g_last_canvas.buf[i] || g_canvas.sty[i].v != g_last_canvas.sty[i].v);
    if (!changes) goto flush_cursor;

    size_t cap = changes * MAX_CELL_SIZE;         
    char *buf  = malloc(cap);
    if (!buf) goto flush_cursor;

    char *ptr = buf;
    for (int i = 0; i < N; ++i) {
        if (g_canvas.buf[i] == g_last_canvas.buf[i] && g_canvas.sty[i].v == g_last_canvas.sty[i].v) continue;
        render_cell_fast(i, &ptr, 1);
    }

    printf("\e[?25l");
    fwrite(buf, 1, ptr - buf, stdout);
    fflush(stdout);
    free(buf);

    memcpy(g_last_canvas.buf, g_canvas.buf, N * sizeof(uint32_t));
    memcpy(g_last_canvas.sty, g_canvas.sty, N * sizeof(style_t));

    flush_cursor:
        if(g_canvas.cursor.cursor_able) {
            int id = g_canvas.cursor.y * g_canvas.w + g_canvas.cursor.x;
            if(g_canvas.buf[id] == 0) g_canvas.cursor.x--;
            printf("\e[?25h\e[%d q\e[%d;%dH", g_canvas.cursor.st, g_canvas.cursor.y, g_canvas.cursor.x);
        }
            
        fflush(stdout);
}

/* ---------- 全量刷新 ---------- */
void canvas_flush_all(void) {
    const int N = g_canvas.w * g_canvas.h;
    size_t cap = N * MAX_CELL_SIZE + g_canvas.h;
    char  *buf = malloc(cap);
    if (!buf) return;

    char *ptr = buf;
    for (int y = 0; y < g_canvas.h; ++y) {
    for (int x = 0; x < g_canvas.w; ++x) {
        render_cell_fast(y * g_canvas.w + x, &ptr, 0);
    }
    *ptr++ = '\n';
    }

    fwrite(buf, 1, ptr - buf, stdout);
    fflush(stdout);
    free(buf);

    memcpy(g_last_canvas.buf, g_canvas.buf, N * sizeof(uint32_t));
    memcpy(g_last_canvas.sty, g_canvas.sty, N * sizeof(style_t));
}

void canvas_cursor_move(int x, int y, int style) {
    g_canvas.cursor.cursor_able = 1;
    g_canvas.cursor.x = x;
    g_canvas.cursor.y = y;
    g_canvas.cursor.st = style;
}

void canvas_cursor_clear(void) {
    g_canvas.cursor.cursor_able = 0;
}
