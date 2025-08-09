/* widgets_richtext.c - 优化版本 */
#include "widgets.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// 脏标记定义
#define DIRTY_NONE    0x00
#define DIRTY_LINES   0x01
#define DIRTY_CURSOR  0x02
#define DIRTY_ALL     0xFF

/* ---------- 基础宏 ---------- */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

/* =========================================================
* 统一布局
* =========================================================*/
typedef struct {
    int bw, scroll_w, gutter_w, info_h;
    int inner_x, inner_y;
    int vis_w, vis_h;
} rt_layout_t;

static inline rt_layout_t rt_calc_layout(const RichTextData *d, const TuiNode *rt) {
    rt_layout_t L = {0};
    
    L.bw       = d->default_style.border ? 1 : 0;
    L.scroll_w = d->show_scroll ? 2 : 0;
    L.gutter_w = d->show_line_no ? 6 : 0;
    L.info_h   = d->show_info ? 1 : 0;
    
    L.inner_x = rt->abs_x + L.bw + 1 + L.gutter_w;
    L.inner_y = rt->abs_y + L.bw + 1;
    
    L.vis_w = rt->bounds.w - 2 * L.bw - L.scroll_w - L.gutter_w;
    L.vis_h = rt->bounds.h - 2 * L.bw - L.info_h;
    return L;
}

/* =========================================================
* 行管理
* =========================================================*/
static void rt_count_lines(const char *text, size_t len, size_t *out_cnt) {
    *out_cnt = 1;
    for (size_t i = 0; i < len; ++i)
        if (text[i] == '\n') (*out_cnt)++;
}

static void rt_build_lines(RichTextData *d) {
    size_t new_cnt;
    rt_count_lines(d->text, d->len, &new_cnt);

    if (new_cnt > d->line_cap) {
        d->line_cap = new_cnt + 16;
        d->lines = realloc(d->lines, d->line_cap * sizeof(RichLine));
    }

    size_t line = 0, start = 0;
    int lineno = 1;
    for (size_t i = 0; i < d->len; ++i) {
        if (d->text[i] == '\n') {
            d->lines[line++] = (RichLine){ start, i - start, lineno++ };
            start = i + 1;
        }
    }
    d->lines[line++] = (RichLine){ start, d->len - start, lineno };
    d->line_cnt = line;
}

static void rt_update_line_info(RichTextData *d) {
    if (d->dirty_flags & DIRTY_LINES) {
        rt_build_lines(d);
        d->dirty_flags &= ~DIRTY_LINES;
    }
}

static void rt_pos_to_line_col(const RichTextData *d, size_t pos,
                               size_t *line, size_t *col) {
    size_t lo = 0, hi = d->line_cnt;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (d->lines[mid].off > pos) hi = mid;
        else lo = mid + 1;
    }
    *line = lo ? lo - 1 : 0;

    *col = 0;
    const char *p = d->text + d->lines[*line].off;
    const char *end = d->text + pos;
    while (p < end) {
        unsigned cp = utf8_decode(&p);
        if (cp == '\n') { *col += 1; break; }
        *col += utf8_width(cp);
    }
}

static size_t rt_line_col_to_pos(const RichTextData *d, size_t line, size_t col) {
    if (line >= d->line_cnt) return d->len;

    const char *p = d->text + d->lines[line].off;
    const char *end = p + d->lines[line].len;

    size_t w = 0;
    while (p < end && *p != '\n' && w < col) {
        unsigned cp = utf8_decode(&p);
        w += utf8_width(cp);
    }
    return p - d->text;
}

static void rt_update_cursor_pos(RichTextData *d) {
    if (d->dirty_flags & DIRTY_CURSOR) {
        rt_pos_to_line_col(d, d->cursor, &d->cursor_line, &d->cursor_col);
        d->dirty_flags &= ~DIRTY_CURSOR;
    }
}

/* =========================================================
* 缓冲区管理
* =========================================================*/
static void rt_ensure_text_capacity(RichTextData *d, size_t needed) {
    if (needed >= d->cap) {
        d->cap = MAX(d->cap * 2, needed + 64);
        d->text = realloc(d->text, d->cap);
    }
}

/* =========================================================
* 滚动逻辑
* =========================================================*/
static void rt_scroll_to_cursor(RichTextData *d, const TuiNode *rt) {
    const rt_layout_t L = rt_calc_layout(d, rt);
    if (L.vis_w <= 0 || L.vis_h <= 0) return;

    rt_update_cursor_pos(d);
    size_t cur_line = d->cursor_line;
    size_t cur_col = d->cursor_col;

    /* 纵向滚动 */
    d->scroll_y = CLAMP(d->scroll_y, 0, (int)d->line_cnt - 1);
    if ((int)cur_line < d->scroll_y)
        d->scroll_y = (int)cur_line;
    else if ((int)cur_line >= d->scroll_y + L.vis_h)
        d->scroll_y = (int)cur_line - L.vis_h + 1;

    /* 横向滚动 */
    d->scroll_x = CLAMP(d->scroll_x, 0, (int)cur_col);
    if ((int)cur_col < d->scroll_x)
        d->scroll_x = (int)cur_col;
    else if ((int)cur_col >= d->scroll_x + L.vis_w)
        d->scroll_x = (int)cur_col - L.vis_w + 1;
}

/* =========================================================
* 事件处理
* =========================================================*/
static void rt_handle_key(RichTextData *d, const TuiNode *rt, const key_event_t *k) {
    for (int i = 0; i < k->num; ++i) {
        int ch = k->key[i];
        if (k->type[i] == KEY_SPECIAL) {
            switch (ch) {
            case K_LEFT:
                if (d->cursor) d->cursor = utf8_prev(d->text, d->cursor);
                break;
            case K_RIGHT:
                if (d->cursor < d->len)
                    d->cursor = utf8_advance(d->text, d->cursor, 1);
                break;
            case K_UP:
            case K_DOWN: {
                size_t line = d->cursor_line;
                size_t col = d->cursor_col;
                line = (ch == K_UP) ? (line ? line - 1 : 0)
                                    : MIN(line + 1, d->line_cnt ? d->line_cnt - 1 : 0);
                d->cursor = rt_line_col_to_pos(d, line, col);
                break;
            }
            case K_HOME:
            case K_END: {
                size_t line = d->cursor_line;
                d->cursor = rt_line_col_to_pos(d, line,
                                               (ch == K_HOME) ? 0 : d->lines[line].len);
                break;
            }
            case K_PGUP:
            case K_PGDN: {
                const rt_layout_t L = rt_calc_layout(d, rt);
                size_t line = d->cursor_line;
                size_t col = d->cursor_col;
                if (ch == K_PGUP)
                    line = (line > (size_t)L.vis_h) ? line - L.vis_h : 0;
                else {
                    size_t max_line = d->line_cnt ? d->line_cnt - 1 : 0;
                    line = MIN(line + L.vis_h, max_line);
                }
                d->cursor = rt_line_col_to_pos(d, line, col);
                break;
            }
            case K_BACKSPACE:
                if (d->cursor > 0) {
                    size_t prev = utf8_prev(d->text, d->cursor);
                    size_t step = d->cursor - prev;
                    memmove(d->text + prev, d->text + d->cursor, d->len - d->cursor + 1);
                    d->cursor = prev;
                    d->len -= step;
                    d->dirty_flags |= DIRTY_LINES;
                }
                break;
            case K_DEL:
                if (d->cursor < d->len) {
                    size_t next = utf8_advance(d->text, d->cursor, 1);
                    size_t step = next - d->cursor;
                    memmove(d->text + d->cursor, d->text + next, d->len - next + 1);
                    d->len -= step;
                    d->dirty_flags |= DIRTY_LINES;
                }
                break;
            case K_ENTER:
                rt_ensure_text_capacity(d, d->len + 1);
                memmove(d->text + d->cursor + 1, d->text + d->cursor,
                        d->len - d->cursor + 1);
                d->text[d->cursor] = '\n';
                d->cursor++;
                d->len++;
                d->dirty_flags |= DIRTY_LINES;
                break;
            }
        } else {
            /* 普通字符 */
            rt_ensure_text_capacity(d, d->len + 4);
            char tmp[4];
            int bytes = utf8_encode(ch, tmp);
            if (bytes > 0) {
                memmove(d->text + d->cursor + bytes, d->text + d->cursor,
                        d->len - d->cursor + 1);
                memcpy(d->text + d->cursor, tmp, bytes);
                d->cursor += bytes;
                d->len += bytes;
                d->dirty_flags |= DIRTY_LINES;
            }
        }
        d->dirty_flags |= DIRTY_CURSOR;
    }
}

static void rt_handle_mouse(RichTextData *d, const TuiNode *rt, const mouse_event_t *m) {
    const rt_layout_t L = rt_calc_layout(d, rt);
    if (L.vis_w <= 0 || L.vis_h <= 0) return;

    const int click_x = m->x - L.inner_x;
    const int click_y = m->y - L.inner_y;

    if (m->type == MOUSE_PRESS && m->button == MOUSE_LEFT) {
        if (click_x >= 0 && click_x < L.vis_w &&
            click_y >= 0 && click_y < L.vis_h) {
            size_t line = d->scroll_y + click_y;
            if (line < d->line_cnt) {
                size_t col = click_x + d->scroll_x;
                d->cursor = rt_line_col_to_pos(d, line, col);
                d->dirty_flags |= DIRTY_CURSOR;
            }
        }
    }

    if (m->type == MOUSE_WHEEL) {
        size_t line = d->cursor_line;
        size_t col = d->cursor_col;
        line = (m->scroll > 0) ? (line ? line - 1 : 0)
                               : MIN(line + 1, d->line_cnt ? d->line_cnt - 1 : 0);
        d->cursor = rt_line_col_to_pos(d, line, col);
        d->dirty_flags |= DIRTY_CURSOR;
    }
}

/* =========================================================
* 绘制组件
* =========================================================*/
static void draw_gutter(TuiNode *n, RichTextData *d, const rt_layout_t *L) {
    // 行号栏背景
    canvas_draw((rect_t){ n->abs_x + L->bw, L->inner_y - 1, 
                         L->gutter_w, L->vis_h }, "", d->line_no_style);
    
    // 绘制行号
    for (int row = 0; row < L->vis_h; ++row) {
        size_t line_idx = d->scroll_y + row;
        if (line_idx >= d->line_cnt) break;

        char gutter[8];
        snprintf(gutter, sizeof(gutter), "%4d", d->lines[line_idx].lineno);
        canvas_draw((rect_t){ n->abs_x + L->bw + 1, L->inner_y + row - 1, 
                             L->gutter_w - 1, 1 }, gutter, d->line_no_style);
    }
}

static void draw_text_lines(TuiNode *n, RichTextData *d, const rt_layout_t *L) {
    for (int row = 0; row < L->vis_h; ++row) {
        size_t line_idx = d->scroll_y + row;
        if (line_idx >= d->line_cnt) break;

        RichLine *ln = &d->lines[line_idx];
        size_t byte_off = utf8_advance(d->text + ln->off, 0, d->scroll_x);
        if (byte_off >= ln->len) continue;

        int char_width = utf8_swidth_len(d->text + ln->off + byte_off, ln->len - byte_off);
        int visible_w = MIN(char_width, L->vis_w);
        size_t byte_take = utf8_trunc_width(d->text + ln->off + byte_off, visible_w);

        char tmp[byte_take + 1];
        memcpy(tmp, d->text + ln->off + byte_off, byte_take);
        tmp[byte_take] = '\0';

        canvas_draw((rect_t){ L->inner_x - 1, L->inner_y + row - 1, L->vis_w, 1 }, tmp, (style_t){.text = 1});
    }
}

static void draw_scrollbar(TuiNode *n, RichTextData *d, const rt_layout_t *L) {
    const int sb_x = n->abs_x + n->bounds.w - L->bw - 1;
    const int sb_h = L->vis_h;
    const int slot_h = MAX(1, sb_h * L->vis_h / (int)d->line_cnt);
    const int slot_y = sb_h * d->scroll_y / (int)d->line_cnt;

    canvas_draw((rect_t){ sb_x, L->inner_y - 1, 1, sb_h }, "", d->scroll_style);
    canvas_draw((rect_t){ sb_x, L->inner_y - 1 + slot_y, 1, 1 }, "█", d->scroll_style);
}

static void draw_info_bar(TuiNode *n, RichTextData *d, const rt_layout_t *L) {
    char info[128];
    rt_update_cursor_pos(d);
    snprintf(info, sizeof(info), "  Ln %zu, Col %zu  |  %zu lines  |  UTF-8  ",
            d->cursor_line + 1, d->cursor_col + 1, d->line_cnt);
    
    canvas_draw((rect_t){ n->abs_x + L->bw,
                         n->abs_y + n->bounds.h - L->bw - 1,
                         n->bounds.w - 2 * L->bw, 1 }, "", d->info_style);
    canvas_draw((rect_t){ n->abs_x + L->bw,
                         n->abs_y + n->bounds.h - L->bw - 1,
                         n->bounds.w - 2 * L->bw, 1 }, info, d->info_style);
}

static void draw_cursor(TuiNode *n, RichTextData *d, const rt_layout_t *L) {
    rt_update_cursor_pos(d);
    int cur_row_vis = (int)d->cursor_line - d->scroll_y;
    int cur_col_vis = (int)d->cursor_col - d->scroll_x;

    if (cur_row_vis >= 0 && cur_row_vis < L->vis_h &&
        cur_col_vis >= 0 && cur_col_vis < L->vis_w) {
        canvas_cursor_move(L->inner_x + cur_col_vis, L->inner_y + cur_row_vis, 1);
    }
}

/* =========================================================
* 主流程
* =========================================================*/
static void richtext_draw(TuiNode *n, void *event);
static void richtext_focus(RichTextData *d, TuiNode *rt, void *event);

TuiNode *richtext_new(TuiRect r, RichTextData *d) {
    TuiNode *rt = tui_node_new(r.x, r.y, r.w, r.h);
    rt->bits.focusable = 1;
    rt->draw = richtext_draw;
    rt->data = d;

    if (!d->cap) d->cap = 256;
    if (!d->text) {
        d->text = malloc(d->cap);
        d->text[0] = '\0';
        d->len = 0;
    }

    d->state = -1;
    d->cursor = 0;
    d->cursor_line = d->cursor_col = 0;
    d->scroll_x = d->scroll_y = 0;
    d->lines = NULL;
    d->line_cnt = d->line_cap = 0;
    d->dirty_flags = DIRTY_ALL;
    rt_build_lines(d);
    return rt;
}

void richtext_free(RichTextData *d) {
    free(d->text);
    free(d->lines);
    free(d->styles);
    free(d->clipboard);
    memset(d, 0, sizeof(*d));
}

static void richtext_draw(TuiNode *n, void *event) {
    RichTextData *d = n->data;
    if (!n->bits.focus && d->state == 0) return;
    if (n->bits.focus) richtext_focus(d, n, event);

    rt_update_line_info(d);
    rt_scroll_to_cursor(d, n);

    const rt_layout_t L = rt_calc_layout(d, n);
    if (L.vis_w <= 0 || L.vis_h <= 0) return;

    // 背景和边框
    canvas_draw((rect_t){n->bounds.x, n->bounds.y, n->bounds.w, n->bounds.h},
                "", d->default_style);

    // 模块化绘制
    if (d->show_line_no) draw_gutter(n, d, &L);
    draw_text_lines(n, d, &L);
    if (d->show_scroll) draw_scrollbar(n, d, &L);
    if (d->show_info) draw_info_bar(n, d, &L);
    if (n->bits.focus) draw_cursor(n, d, &L);
}

static void richtext_focus(RichTextData *d, TuiNode *rt, void *event) {
    if (!event) return;
    event_t *ev = (event_t *)event;

    if (ev->type == EVENT_KEY)
        rt_handle_key(d, rt, &ev->key);
    else if (ev->type == EVENT_MOUSE)
        rt_handle_mouse(d, rt, &ev->mouse);
}