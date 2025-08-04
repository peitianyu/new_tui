#include "render.h"

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
    int bs = (bs >= sizeof(g_border_tbl)/sizeof(g_border_tbl[0])) ? BORDER_LIGHT : st.border_st;
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
static void draw_text(rect_t r_orig, const char *utf8, style_t st) {
    if (!st.text || !utf8) return;

    /* 可用区域 */
    int bw = st.border ? 1 : 0;
    int usable_w = r_orig.w - 2 * bw;
    int usable_h = r_orig.h - 2 * bw;
    if (usable_w <= 0 || usable_h <= 0) return;

    int x0 = r_orig.x + bw;
    int y0 = r_orig.y + bw;

    /* 统计行 */
    typedef struct { const char *s; int w; } line_t;
    line_t lines[64];
    int n = 0;
    for (const char *p = utf8; *p && n < 64;) {
        lines[n].s = p;
        lines[n].w = 0;
        while (*p && *p != '\n') {
            uint32_t cp = utf8_decode(&p);
            lines[n].w += utf8_width(cp);
        }
        ++n;
        if (*p == '\n') ++p;
    }

    /* 垂直偏移 */
    int start_y = y0;
    if (st.align_vert == 1) start_y += (usable_h - n) / 2;
    if (st.align_vert == 2) start_y += (usable_h - n);

    /* 逐行绘制 */
    for (int ly = 0; ly < n && start_y + ly < y0 + usable_h; ++ly) {
        int start_x = x0;
        if (st.align_horz == 1) start_x += (usable_w - lines[ly].w) / 2;
        if (st.align_horz == 2) start_x += (usable_w - lines[ly].w);

        const char *p = lines[ly].s;
        int cx = start_x;
        while (*p && *p != '\n') {
            uint32_t cp = utf8_decode(&p);
            int cw = utf8_width(cp);
            if (cx >= x0 && cx < x0 + usable_w && start_y + ly < y0 + usable_h) {
                int i = (start_y + ly) * g_canvas.w + cx;
                g_canvas.buf[i] = cp; g_canvas.sty[i] = st;
                if (cw == 2 && cx + 1 < x0 + usable_w) {
                    g_canvas.buf[i + 1] = 0; g_canvas.sty[i + 1] = st;
                }
            }
            cx += cw;
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

/* ---------- 终端输出 ---------- */
#define SEQ_LEN(x)  (sizeof(x)-1)
static const char *FG[16] = {
    "\e[30m","\e[34m","\e[35m","\e[32m","\e[31m","\e[90m","\e[37m","\e[97m",
    "\e[91m","\e[33m","\e[93m","\e[92m","\e[36m","\e[95m","\e[96m","\e[94m" };
static const char *BG[16] = {
    "\e[40m","\e[44m","\e[45m","\e[42m","\e[41m","\e[100m","\e[47m","\e[107m",
    "\e[101m","\e[43m","\e[103m","\e[102m","\e[106m","\e[105m","\e[106m","\e[104m" };
static const uint8_t FG_LEN[16] = {       
    SEQ_LEN("\e[30m"),SEQ_LEN("\e[34m"),SEQ_LEN("\e[35m"),SEQ_LEN("\e[32m"),
    SEQ_LEN("\e[31m"),SEQ_LEN("\e[90m"),SEQ_LEN("\e[37m"),SEQ_LEN("\e[97m"),
    SEQ_LEN("\e[91m"),SEQ_LEN("\e[33m"),SEQ_LEN("\e[93m"),SEQ_LEN("\e[92m"),
    SEQ_LEN("\e[36m"),SEQ_LEN("\e[95m"),SEQ_LEN("\e[96m"),SEQ_LEN("\e[94m") };
static const uint8_t BG_LEN[16] = {
    SEQ_LEN("\e[40m"),SEQ_LEN("\e[44m"),SEQ_LEN("\e[45m"),SEQ_LEN("\e[42m"),
    SEQ_LEN("\e[41m"),SEQ_LEN("\e[100m"),SEQ_LEN("\e[47m"),SEQ_LEN("\e[107m"),
    SEQ_LEN("\e[101m"),SEQ_LEN("\e[43m"),SEQ_LEN("\e[103m"),SEQ_LEN("\e[102m"),
    SEQ_LEN("\e[106m"),SEQ_LEN("\e[105m"),SEQ_LEN("\e[106m"),SEQ_LEN("\e[104m") };
#undef SEQ_LEN

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
    
    int fg = (st.border) ? 15-st.border_fg : st.fg;
    memcpy(dst, FG[fg], FG_LEN[fg]);       dst += FG_LEN[fg];
    memcpy(dst, BG[st.bg], BG_LEN[st.bg]); dst += BG_LEN[st.bg];
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
            if(g_canvas.buf[id] == 0) g_canvas.cursor.x++;
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
