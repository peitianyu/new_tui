#include "tui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ---------- 节点生命周期 ---------- */
TuiNode *tui_node_new(int16_t x, int16_t y, int16_t w, int16_t h) {
    TuiNode *n = calloc(1, sizeof *n);
    if (!n) { perror("calloc"); exit(EXIT_FAILURE); }
    n->bounds = (TuiRect){x, y, w, h};
    n->clip   = n->bounds;
    n->abs_x  = x;
    n->abs_y  = y;
    n->bits.focusable = 1;
    n->bits.dirty = 1;
    n->kid_cap = 0;
    return n;
}

void tui_node_add(TuiNode *p, TuiNode *c) {
    if (p->kid_cnt >= p->kid_cap) {
        p->kid_cap = p->kid_cap ? p->kid_cap * 2 : 8;
        p->kids = realloc(p->kids, p->kid_cap * sizeof *p->kids);
        if (!p->kids) { perror("realloc"); exit(EXIT_FAILURE); }
    }
    p->kids[p->kid_cnt++] = c;
    c->parent = p;
    p->bits.dirty = 1;
}

void tui_node_remove(TuiNode *parent, TuiNode *child) {
    if (!parent || !child) return;
    for (int i = 0; i < parent->kid_cnt; ++i) {
        if (parent->kids[i] == child) {
            tui_node_free(child);
            parent->kids[i] = parent->kids[--parent->kid_cnt];
            parent->kids[parent->kid_cnt] = NULL;
            parent->bits.dirty = 1;
            return;
        }
    }
}

void tui_node_free(TuiNode *n) {
    if (!n) return;
    for (int i = 0; i < n->kid_cnt; ++i) tui_node_free(n->kids[i]);
    free(n->kids);
    free(n->data);
    free(n);
}

/* ---------- 约束与布局 ---------- */
static inline void apply_constraints(TuiNode *n) {
    if (n->constraints.min_w && n->bounds.w < n->constraints.min_w)
        n->bounds.w = n->constraints.min_w;
    if (n->constraints.max_w && n->bounds.w > n->constraints.max_w)
        n->bounds.w = n->constraints.max_w;
    if (n->constraints.min_h && n->bounds.h < n->constraints.min_h)
        n->bounds.h = n->constraints.min_h;
    if (n->constraints.max_h && n->bounds.h > n->constraints.max_h)
        n->bounds.h = n->constraints.max_h;
}

static void layout_vert(TuiNode *p) {
    int16_t y = p->style.padding;
    int16_t avail_w = p->bounds.w - 2 * p->style.padding;
    int16_t fix_h = 0, exp = 0;

    for (int i = 0; i < p->kid_cnt; ++i) {
        apply_constraints(p->kids[i]);
        if (p->kids[i]->bits.expand) exp++;
        else fix_h += p->kids[i]->bounds.h;
    }
    int16_t avail_h = p->bounds.h - 2 * p->style.padding
                      - (p->kid_cnt - 1) * p->style.spacing - fix_h;
    int16_t exp_h   = exp ? avail_h / exp : 0;

    for (int i = 0; i < p->kid_cnt; ++i) {
        TuiNode *c = p->kids[i];
        c->bounds.w = avail_w; apply_constraints(c);
        if (c->bits.expand) { c->bounds.h = exp_h; apply_constraints(c); }
        c->bounds.x = p->style.padding;
        c->bounds.y = y;
        y += c->bounds.h + p->style.spacing;
        c->bits.dirty = 1;
    }
}

static void layout_horz(TuiNode *p) {
    int16_t x = p->style.padding;
    int16_t avail_h = p->bounds.h - 2 * p->style.padding;
    int16_t fix_w = 0, exp = 0;

    for (int i = 0; i < p->kid_cnt; ++i) {
        apply_constraints(p->kids[i]);
        if (p->kids[i]->bits.expand) exp++;
        else fix_w += p->kids[i]->bounds.w;
    }
    int16_t avail_w = p->bounds.w - 2 * p->style.padding
                      - (p->kid_cnt - 1) * p->style.spacing - fix_w;
    int16_t exp_w   = exp ? avail_w / exp : 0;

    for (int i = 0; i < p->kid_cnt; ++i) {
        TuiNode *c = p->kids[i];
        c->bounds.h = avail_h; apply_constraints(c);
        if (c->bits.expand) { c->bounds.w = exp_w; apply_constraints(c); }
        c->bounds.x = x;
        c->bounds.y = p->style.padding;
        x += c->bounds.w + p->style.spacing;
        c->bits.dirty = 1;
    }
}

static void layout_flow(TuiNode *p) {
    int16_t x = p->style.padding, y = p->style.padding, line_h = 0;
    int16_t max_w = p->bounds.w - 2 * p->style.padding;
    for (int i = 0; i < p->kid_cnt; ++i) {
        TuiNode *c = p->kids[i];
        apply_constraints(c);
        if (x + c->bounds.w > max_w && x > p->style.padding) {
            x = p->style.padding;
            y += line_h + p->style.spacing;
            line_h = 0;
        }
        c->bounds.x = x;
        c->bounds.y = y;
        x += c->bounds.w + p->style.spacing;
        if (c->bounds.h > line_h) line_h = c->bounds.h;
        c->bits.dirty = 1;
    }
}

static inline void apply_layout(TuiNode *n) {
    if (!n || !n->bits.dirty) return;
    switch (n->bits.layout) {
        case TUI_LAYOUT_VERT: layout_vert(n); break;
        case TUI_LAYOUT_HORZ: layout_horz(n); break;
        case TUI_LAYOUT_FLOW: layout_flow(n); break;
        default: break;
    }
}

static void calc_recursive(TuiNode *n) {
    if (!n || !n->bits.dirty) return;

    if (n->parent) {
        n->abs_x = n->parent->abs_x + n->bounds.x;
        n->abs_y = n->parent->abs_y + n->bounds.y - n->scroll.y;

        n->abs_x = n->parent->abs_x + n->bounds.x;
        n->abs_y = n->parent->abs_y + n->bounds.y - n->parent->scroll.y;
    } else {
        n->abs_x = n->bounds.x;
        n->abs_y = n->bounds.y;
    }

    TuiRect pvis = n->parent ? n->parent->clip
                             : (TuiRect){0, 0, n->bounds.w, n->bounds.h};
    TuiRect self = {n->abs_x, n->abs_y, n->bounds.w, n->bounds.h};
    int16_t x1 = (self.x > pvis.x) ? self.x : pvis.x;
    int16_t y1 = (self.y > pvis.y) ? self.y : pvis.y;
    int16_t x2 = (self.x + self.w < pvis.x + pvis.w)
                 ? self.x + self.w : pvis.x + pvis.w;
    int16_t y2 = (self.y + self.h < pvis.y + pvis.h)
                 ? self.y + self.h : pvis.y + pvis.h;
    n->clip = (TuiRect){x1, y1, (x2 > x1 ? x2 - x1 : 0), (y2 > y1 ? y2 - y1 : 0)};

    apply_layout(n);
    for (int i = 0; i < n->kid_cnt; ++i) calc_recursive(n->kids[i]);
    n->bits.dirty = 0;
}

void tui_calc(TuiNode *root) { 
    if (root) calc_recursive(root); 
}

void tui_scroll(TuiNode *n, int16_t d) {
    if (!n) return;
    n->scroll.y += d;
    if (n->scroll.y < 0) n->scroll.y = 0;
    n->bits.dirty = 1;
    tui_calc(n);
}

/* ---------- 焦点 / Hover ---------- */
static TuiNode *g_focus = NULL;
static bool inside(const TuiNode *n, int16_t x, int16_t y) {
    return x >= n->clip.x && x < n->clip.x + n->clip.w &&
           y >= n->clip.y && y < n->clip.y + n->clip.h;
}

static void clear_hover_recursive(TuiNode *n) {
    if (!n) return;
    n->bits.hover = 0;
    for (int i = 0; i < n->kid_cnt; ++i)
        clear_hover_recursive(n->kids[i]);
}

static TuiNode *hit_recursive(TuiNode *n, int16_t x, int16_t y) {
    if (!n || !inside(n, x, y)) return NULL;
    for (int i = n->kid_cnt - 1; i >= 0; --i) {
        TuiNode *c = hit_recursive(n->kids[i], x, y);
        if (c && c->bits.focusable) return c;
    }
    return n->bits.focusable ? n : NULL;
}

TuiNode *tui_hit_test(TuiNode *root, int16_t mx, int16_t my) {
    if (!root) return NULL;
    clear_hover_recursive(root);
    TuiNode *h = hit_recursive(root, mx, my);
    if (h && h->bits.focusable) h->bits.hover = 1;
    return h;
}

bool tui_focus_set(TuiNode *root, TuiNode *target) {
    if (!root || !target || !target->bits.focusable) return false;
    if (g_focus) g_focus->bits.focus = 0;
    g_focus = target;
    g_focus->bits.focus = 1;
    return true;
}

TuiNode *tui_focus_get(TuiNode *root) {
    if (!root) return NULL;
    if (g_focus && g_focus->bits.focusable) return g_focus;
    TuiNode *n = hit_recursive(root, root->abs_x + root->bounds.w / 2,
                               root->abs_y + root->bounds.h / 2);
    return n && n->bits.focusable ? n : NULL;
}

void tui_focus_next(TuiNode *root) {
    if (!root) return;
    TuiNode *cur = g_focus;
    TuiNode *next = NULL;
    bool found = (cur == NULL);

    TuiNode *stack[128];
    int top = 0;
    stack[top++] = root;
    while (top) {
        TuiNode *n = stack[--top];
        if (n->bits.focusable) {
            if (found) { next = n; break; }
            if (n == cur) found = 1;
        }
        for (int i = n->kid_cnt - 1; i >= 0; --i) stack[top++] = n->kids[i];
    }
    if (next) tui_focus_set(root, next);
    else if (cur) tui_focus_set(root, root);
}

/* ---------- 内部工具 ---------- */
static void out_append(Canvas *c, const char *s) {
    size_t len = strlen(s);
    if (c->out_len + len >= c->out_cap) {
        size_t new_cap = c->out_cap ? c->out_cap * 2 : 4096;
        while (new_cap <= c->out_len + len) new_cap *= 2;
        char *new_out = realloc(c->out, new_cap);
        if (!new_out) { perror("realloc out"); exit(EXIT_FAILURE); }
        c->out = new_out;
        c->out_cap = new_cap;
    }
    memcpy(c->out + c->out_len, s, len);
    c->out_len += len;
}

static void cv_color(Canvas *c, bool fg, TuiColor col) {
    char tmp[32];
    snprintf(tmp, sizeof tmp, "\x1b[%d8;2;%d;%d;%dm",
             fg ? 3 : 4, col.r, col.g, col.b);
    out_append(c, tmp);
}

static void cv_color_reset(Canvas *c) {
    out_append(c, "\x1b[0m");
}
/* ---------- 画布 ---------- */
Canvas *cv_new(int w, int h) {
    Canvas *c = malloc(sizeof *c);
    if (!c) { perror("malloc"); exit(EXIT_FAILURE); }
    c->w = w; c->h = h;
    c->buf = calloc(w * h, 1);
    if (!c->buf) { perror("calloc"); exit(EXIT_FAILURE); }
    memset(c->buf, ' ', w * h);

    c->out_cap = 4096;
    c->out = malloc(c->out_cap);
    if (!c->out) { perror("malloc out"); exit(EXIT_FAILURE); }
    c->out_len = 0;
    return c;
}

void cv_free(Canvas *c) {
    if (!c) return;
    free(c->buf);
    free(c->out);
    free(c);
}

void cv_flush(Canvas *c) {
    fputs("\x1b[2J\x1b[H", stdout);
    fwrite(c->out, 1, c->out_len, stdout);
    fflush(stdout);
    c->out_len = 0;
}

/* ---------- 画布重用 ---------- */
static Canvas *g_shared_canvas = NULL;
void draw_scene_focus_init(int w, int h) {
    if (!g_shared_canvas) g_shared_canvas = cv_new(w, h);
}

void draw_scene_focus_cleanup(void) {
    if (g_shared_canvas) {
        cv_free(g_shared_canvas);
        g_shared_canvas = NULL;
    }

    printf("\x1b[0m\x1b[H\x1b[2J");  
    fflush(stdout);
}

/* ---------- 裁剪 ---------- */
static inline bool cv_clip(const TuiRect *clip, const TuiRect *in, TuiRect *out) {
    int x0 = in->x > clip->x ? in->x : clip->x;
    int y0 = in->y > clip->y ? in->y : clip->y;
    int x1 = (in->x + in->w) < (clip->x + clip->w) ? (in->x + in->w) : (clip->x + clip->w);
    int y1 = (in->y + in->h) < (clip->y + clip->h) ? (in->y + in->h) : (clip->y + clip->h);

    if (x0 >= x1 || y0 >= y1) {
        out->w = out->h = 0;
        return false;
    }
    *out = (TuiRect){x0, y0, x1 - x0, y1 - y0};
    return true;
}

/* ---------- 画框 ---------- */
static void cv_border_fast(Canvas *c, int x, int y, int w, int h) {
    if (w <= 2 || h <= 2) return;
    char line[128];
    snprintf(line, sizeof line, "\x1b[%d;%dH┌", y + 1, x + 1);
    out_append(c, line);
    for (int i = 1; i < w - 1; ++i) out_append(c, "─");
    out_append(c, "┐");

    for (int j = 1; j < h - 1; ++j) {
        snprintf(line, sizeof line, "\x1b[%d;%dH│", y + 1 + j, x + 1);
        out_append(c, line);
        snprintf(line, sizeof line, "\x1b[%d;%dH│", y + 1 + j, x + w);
        out_append(c, line);
    }

    snprintf(line, sizeof line, "\x1b[%d;%dH└", y + h, x + 1);
    out_append(c, line);
    for (int i = 1; i < w - 1; ++i) out_append(c, "─");
    out_append(c, "┘");
}

/* ---------- 画背景 ---------- */
static void cv_rect_fast(Canvas *c, int x, int y, int w, int h,
                         const TuiNode *n) {
    if (w <= 0 || h <= 0) return;

    TuiColor bg = n->style.bg;
    if (tui_node_has_state(n, TUI_NODE_STATE_FOCUS))
        bg = (TuiColor){255, 255, 0};   /* yellow */
    else if (tui_node_has_state(n, TUI_NODE_STATE_HOVER))
        bg = (TuiColor){0, 255, 255};   /* cyan  */

    cv_color(c, false, bg);
    char line[1024];
    for (int j = y; j < y + h; ++j) {
        if (j < 0 || j >= c->h) continue;
        snprintf(line, sizeof line, "\x1b[%d;%dH%*s", j + 1, x + 1, w, "");
        out_append(c, line);
    }
}

/* ---------- 画文字 ---------- */
static void cv_text_fast(Canvas *c, int x, int y, const char *s,
                         const TuiNode *n) {
    if (!tui_node_has_state(n, TUI_NODE_STATE_STYLE_FG)) return;

    TuiRect clip = n->clip;
    int len = strlen(s);
    if (len <= 0) return;

    /* 水平对齐 */
    int render_x = x;
    int text_w   = len;
    int box_w    = n->clip.w;
    switch (n->style.align_horz) {
        case TUI_ALIGN_CENTER: render_x = x + (box_w - text_w) / 2; break;
        case TUI_ALIGN_RIGHT:  render_x = x + box_w - text_w; break;
        default: break;
    }

    /* 垂直对齐（单行）*/
    int render_y = y;
    switch (n->style.align_vert) {
        case TUI_ALIGN_MIDDLE: render_y = y + (n->clip.h - 1) / 2; break;
        case TUI_ALIGN_BOTTOM: render_y = y + n->clip.h - 1; break;
        default: break;
    }

    /* 裁剪 */
    if (render_x < clip.x) {
        int skip = clip.x - render_x;
        if (skip >= len) return;
        s += skip;
        render_x += skip;
        len -= skip;
    }
    int max_w = clip.x + clip.w - render_x;
    if (len > max_w) len = max_w;
    if (len <= 0) return;

    char buf[256];
    memcpy(buf, s, len); buf[len] = '\0';

    cv_color(c, true, n->style.fg);
    char prefix[32];
    snprintf(prefix, sizeof(prefix), "\x1b[%d;%dH", render_y + 1, render_x + 1);
    out_append(c, prefix);
    out_append(c, buf);
}

/* ---------- 递归绘制 ---------- */
void tui_draw_node(Canvas *c, TuiNode *n) {
    if (!n || n->clip.w <= 0 || n->clip.h <= 0) return;

    cv_rect_fast(c, n->clip.x, n->clip.y, n->clip.w, n->clip.h, n);

    if (tui_node_has_state(n, TUI_NODE_STATE_STYLE_BORDER))
        cv_border_fast(c, n->clip.x, n->clip.y, n->clip.w, n->clip.h);

    if (n->data && *(char *)n->data) {
        cv_text_fast(c, n->abs_x + 1, n->abs_y,
                     (const char *)n->data, n);
    }

    for (int i = 0; i < n->kid_cnt; ++i)
        tui_draw_node(c, n->kids[i]);
}

void draw_scene_focus(TuiNode *root) {
    tui_calc(root);
    g_shared_canvas->out_len = 0;
    cv_color_reset(g_shared_canvas);
    tui_draw_node(g_shared_canvas, root);
    cv_flush(g_shared_canvas);
}