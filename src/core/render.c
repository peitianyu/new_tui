#include "render.h"

static canvas_t g_canvas = {0};
static canvas_t g_last_canvas = {0};

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
void canvas_draw(rect_t r_orig, const char *utf8, style_t st) {
    rect_t r = clip(r_orig);
    if (r.w <= 0 || r.h <= 0) return;

    /* 背景填充 */
    if (st.rect) {
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
        if (x0 < 0 || x1 >= g_canvas.w || y0 < 0 || y1 >= g_canvas.h) goto skip_border; 
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
#define SEQ_LEN(x)  (sizeof(x)-1)

static const char *FG[16] = {
    "\033[30m","\033[34m","\033[35m","\033[32m","\033[31m","\033[90m","\033[37m","\033[97m",
    "\033[91m","\033[33m","\033[93m","\033[92m","\033[36m","\033[95m","\033[96m","\033[94m"
};
static const char *BG[16] = {
    "\033[40m","\033[44m","\033[45m","\033[42m","\033[41m","\033[100m","\033[47m","\033[107m",
    "\033[101m","\033[43m","\033[103m","\033[102m","\033[106m","\033[105m","\033[106m","\033[104m"
};

static const uint8_t FG_LEN[16] = {       // 预存长度，省一次 sizeof
    SEQ_LEN("\033[30m"),SEQ_LEN("\033[34m"),SEQ_LEN("\033[35m"),SEQ_LEN("\033[32m"),
    SEQ_LEN("\033[31m"),SEQ_LEN("\033[90m"),SEQ_LEN("\033[37m"),SEQ_LEN("\033[97m"),
    SEQ_LEN("\033[91m"),SEQ_LEN("\033[33m"),SEQ_LEN("\033[93m"),SEQ_LEN("\033[92m"),
    SEQ_LEN("\033[36m"),SEQ_LEN("\033[95m"),SEQ_LEN("\033[96m"),SEQ_LEN("\033[94m")
};
static const uint8_t BG_LEN[16] = {
    SEQ_LEN("\033[40m"),SEQ_LEN("\033[44m"),SEQ_LEN("\033[45m"),SEQ_LEN("\033[42m"),
    SEQ_LEN("\033[41m"),SEQ_LEN("\033[100m"),SEQ_LEN("\033[47m"),SEQ_LEN("\033[107m"),
    SEQ_LEN("\033[101m"),SEQ_LEN("\033[43m"),SEQ_LEN("\033[103m"),SEQ_LEN("\033[102m"),
    SEQ_LEN("\033[106m"),SEQ_LEN("\033[105m"),SEQ_LEN("\033[106m"),SEQ_LEN("\033[104m")
};

#undef SEQ_LEN

/* ---------------------------------------- */
static inline int pos_seq_len(int y, int x) {
    char tmp[32];
    return sprintf(tmp, "\033[%d;%dH", y, x);
}

// 统一字符编码处理
static inline int encode_utf8(uint32_t cp, char buf[4]) {
    if (cp == 0) return 0;
    if (cp < 0x80) {
        buf[0] = (char)cp;
        return 1;
    } else if (cp < 0x800) {
        buf[0] = (char)(0xC0 | (cp >> 6));
        buf[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    } else if (cp < 0x10000) {
        buf[0] = (char)(0xE0 | (cp >> 12));
        buf[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        buf[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    }
    return 0;
}

/* ---------- 统一渲染单元 ---------- */
static inline void render_cell_fast(int idx, char **p, int with_pos) {
    uint32_t cp = g_canvas.buf[idx];
    if (!cp) return;

    style_t st = g_canvas.sty[idx];
    char  ch[4];
    int   ch_len = encode_utf8(cp, ch);
    if (!ch_len) return;

    char *dst = *p;
    if (with_pos) {
        int y = idx / g_canvas.w + 1;
        int x = idx % g_canvas.w + 1;
        dst += sprintf(dst, "\033[%d;%dH", y, x);
    }

    memcpy(dst, BG[st.bg], BG_LEN[st.bg]); dst += BG_LEN[st.bg];
    memcpy(dst, FG[st.fg], FG_LEN[st.fg]); dst += FG_LEN[st.fg];
    memcpy(dst, ch, ch_len);               dst += ch_len;
    memcpy(dst, "\033[0m", 4);             dst += 4;

    *p = dst;
}

/* ---------- 增量刷新 ---------- */
void canvas_flush(void) {
    const int N = g_canvas.w * g_canvas.h;

    /* 先粗估变化单元数，预留空间：平均 40 字节/格 */
    int changes = 0;
    for (int i = 0; i < N; ++i) {
        if (g_canvas.buf[i] != g_last_canvas.buf[i] ||
            g_canvas.sty[i].v != g_last_canvas.sty[i].v)
            ++changes;
    }

    printf(g_canvas.cursor.cursor_able ? "\033[?25h" : "\033[?25l");
    printf("\033[%d q\033[%d;%dH", g_canvas.cursor.st, g_canvas.cursor.y + 1, g_canvas.cursor.x + 1);
    fflush(stdout);

    if (!changes) return;

    size_t cap = changes * 64;         
    char *buf  = malloc(cap);
    if (!buf) return;

    char *ptr = buf;
    for (int i = 0; i < N; ++i) {
        if (g_canvas.buf[i] == g_last_canvas.buf[i] &&
            g_canvas.sty[i].v == g_last_canvas.sty[i].v) continue;

        if ((size_t)(ptr - buf) + 64 > cap) {
            cap *= 2;
            char *tmp = realloc(buf, cap);
            if (!tmp) { free(buf); return; }
            ptr = tmp + (ptr - buf);
            buf = tmp;
        }

        render_cell_fast(i, &ptr, 1);
    }
    fwrite(buf, 1, ptr - buf, stdout);
    printf("\033[%d q\033[%d;%dH", g_canvas.cursor.st, g_canvas.cursor.y + 1, g_canvas.cursor.x + 1);
    fflush(stdout);
    free(buf);

    /* 更新历史 */
    memcpy(g_last_canvas.buf, g_canvas.buf, N * sizeof(uint32_t));
    memcpy(g_last_canvas.sty, g_canvas.sty, N * sizeof(style_t));
}

/* ---------- 全量刷新 ---------- */
void canvas_flush_all(void) {
    const int N = g_canvas.w * g_canvas.h;
    size_t cap = N * 40 + g_canvas.h;
    char  *buf = malloc(cap);
    if (!buf) return;

    char *ptr = buf;
    for (int y = 0; y < g_canvas.h; ++y) {
        for (int x = 0; x < g_canvas.w; ++x) {
            int idx = y * g_canvas.w + x;
            render_cell_fast(idx, &ptr, 0);
        }
        *ptr++ = '\n';
    }

    fwrite(buf, 1, ptr - buf, stdout);
    fflush(stdout);
    free(buf);

    memcpy(g_last_canvas.buf, g_canvas.buf, N * sizeof(uint32_t));
    memcpy(g_last_canvas.sty, g_canvas.sty, N * sizeof(style_t));
}

void canvas_rest_cursor(void) {
    g_canvas.cursor.cursor_able = 0;
}
void canvas_cursor_move(int cusr_able, int x, int y, int style) {
    g_canvas.cursor.cursor_able = cusr_able;
    g_canvas.cursor.x = x;
    g_canvas.cursor.y = y;
    g_canvas.cursor.st = style;
}