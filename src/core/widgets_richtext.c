#include "widgets.h"
#include <stdlib.h>
#include <string.h>

/* ---------- 基础宏 ---------- */
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))

/* =========================================================
* 行信息相关工具（原 build_lines / pos_to_line_col 等）
* =========================================================*/
static void rt_count_lines(const char *text, size_t len, size_t *out_cnt)
{
    *out_cnt = 1;
    for (size_t i = 0; i < len; ++i)
        if (text[i] == '\n') (*out_cnt)++;
}

static void rt_build_lines(RichTextData *d)
{
    size_t new_cnt;
    rt_count_lines(d->text, d->len, &new_cnt);

    /* 按需扩容 */
    if (new_cnt > d->line_cap) {
        d->line_cap = new_cnt + 16;
        d->lines    = realloc(d->lines, d->line_cap * sizeof(RichLine));
    }

    /* 重新填行表 */
    size_t line = 0, start = 0;
    int    lineno = 1;
    for (size_t i = 0; i < d->len; ++i) {
        if (d->text[i] == '\n') {
            d->lines[line++] = (RichLine){ start, i - start, lineno++ };
            start = i + 1;
        }
    }
    d->lines[line++] = (RichLine){ start, d->len - start, lineno };
    d->line_cnt      = line;
}

/* 把字节下标换成行列 */
static void rt_pos_to_line_col(const RichTextData *d, size_t pos,
                            size_t *line, size_t *col)
{
    size_t lo = 0, hi = d->line_cnt;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (d->lines[mid].off > pos) hi = mid;
        else                         lo = mid + 1;
    }
    *line = lo ? lo - 1 : 0;

    *col = 0;
    const char *p   = d->text + d->lines[*line].off;
    const char *end = d->text + pos;
    while (p < end) {
        unsigned cp = utf8_decode(&p);
        if (cp == '\n') { *col += 1; break; }
        *col += utf8_width(cp);
    }
}

/* 把行列换成字节下标 */
static size_t rt_line_col_to_pos(const RichTextData *d, size_t line, size_t col)
{
    if (line >= d->line_cnt) return d->len;

    const char *p   = d->text + d->lines[line].off;
    const char *end = p + d->lines[line].len;

    size_t w = 0;
    while (p < end && *p != '\n' && w < col) {
        unsigned cp = utf8_decode(&p);
        w += utf8_width(cp);
    }
    return p - d->text;
}

/* =========================================================
* 缓冲区容量管理
* =========================================================*/
static void rt_ensure_text_capacity(RichTextData *d, size_t needed)
{
    if (needed >= d->cap) {
        d->cap = MAX(d->cap * 2, needed + 64);
        d->text = realloc(d->text, d->cap);
    }
}

/* =========================================================
* 滚动逻辑
* =========================================================*/
static void rt_scroll_to_cursor(RichTextData *d, const TuiNode *rt)
{
    if (!rt || !d) return;

    const int bw    = d->default_style.border ? 1 : 0;
    const int scroll_w = d->show_scroll ? 2 : 0;
    const int vis_w = rt->bounds.w - 2 * bw - scroll_w;
    const int vis_h = rt->bounds.h - 2 * bw;
    if (vis_w <= 0 || vis_h <= 0) return;

    size_t cur_line, cur_col;
    rt_pos_to_line_col(d, d->cursor, &cur_line, &cur_col);

    /* 纵向 */
    d->scroll_y = CLAMP(d->scroll_y, 0, (int)d->line_cnt - 1);
    if ((int)cur_line < d->scroll_y)
        d->scroll_y = (int)cur_line;
    else if ((int)cur_line >= d->scroll_y + vis_h)
        d->scroll_y = (int)cur_line - vis_h + 1;

    /* 横向 */
    d->scroll_x = CLAMP(d->scroll_x, 0, (int)cur_col);
    if ((int)cur_col < d->scroll_x)
        d->scroll_x = (int)cur_col;
    else if ((int)cur_col >= d->scroll_x + vis_w)
        d->scroll_x = (int)cur_col - vis_w + 1;
}

/* =========================================================
* 输入事件拆分
* =========================================================*/
static void rt_handle_key(RichTextData *d, const TuiNode *rt, const key_event_t *k)
{
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
                size_t line, col;
                rt_pos_to_line_col(d, d->cursor, &line, &col);
                line = (ch == K_UP) ? (line ? line - 1 : 0)
                                    : MIN(line + 1, d->line_cnt ? d->line_cnt - 1 : 0);
                d->cursor = rt_line_col_to_pos(d, line, col);
                break;
            }
            case K_HOME:
            case K_END: {
                size_t line, col;
                rt_pos_to_line_col(d, d->cursor, &line, &col);
                d->cursor = rt_line_col_to_pos(d, line,
                                            (ch == K_HOME) ? 0 : d->lines[line].len);
                break;
            }
            case K_PGUP:
            case K_PGDN: {
                const int bw    = d->default_style.border ? 1 : 0;
                const int vis_h = rt->bounds.h - 2 * bw;
                size_t line, col;
                rt_pos_to_line_col(d, d->cursor, &line, &col);
                if (ch == K_PGUP)
                    line = (line > (size_t)vis_h) ? line - vis_h : 0;
                else {
                    size_t max_line = d->line_cnt ? d->line_cnt - 1 : 0;
                    line = MIN(line + vis_h, max_line);
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
                    d->len   -= step;
                }
                break;
            case K_DEL:
                if (d->cursor < d->len) {
                    size_t next = utf8_advance(d->text, d->cursor, 1);
                    size_t step = next - d->cursor;
                    memmove(d->text + d->cursor, d->text + next, d->len - next + 1);
                    d->len -= step;
                }
                break;
            case K_ENTER:
                rt_ensure_text_capacity(d, d->len + 1);
                memmove(d->text + d->cursor + 1, d->text + d->cursor,
                        d->len - d->cursor + 1);
                d->text[d->cursor] = '\n';
                d->cursor++;
                d->len++;
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
                d->len    += bytes;
            }
        }
        rt_build_lines(d);
    }
}

static void rt_handle_mouse(RichTextData *d, const TuiNode *rt, const mouse_event_t *m)
{
    const int bw    = d->default_style.border ? 1 : 0;
    const int scroll_w = d->show_scroll ? 2 : 0;
    const int click_x = m->x - rt->abs_x - bw;
    const int click_y = m->y - rt->abs_y - bw - 1;
    const int vis_w   = rt->bounds.w - 2 * bw - scroll_w;
    const int vis_h   = rt->bounds.h - 2 * bw;

    if (click_x >= 0 && click_x < vis_w &&
        click_y >= 0 && click_y < vis_h) {
        size_t line = d->scroll_y + click_y;
        if (line < d->line_cnt) {
            size_t col = click_x + d->scroll_x;
            d->cursor = rt_line_col_to_pos(d, line, col);
        }
    }
}

/* =========================================================
* 主流程（保持不变，只调用上面的小函数）
* =========================================================*/
static void richtext_draw(TuiNode *n, void *event);
static void richtext_focus(RichTextData *d, TuiNode *rt, void *event);

TuiNode *richtext_new(TuiRect r, RichTextData *d)
{
    TuiNode *rt = tui_node_new(r.x, r.y, r.w, r.h);
    rt->bits.focusable = 1;
    rt->draw           = richtext_draw;
    rt->data           = d;

    /* 初始化文本缓冲区 */
    if (!d->cap) d->cap = 256;
    if (!d->text) {
        d->text = malloc(d->cap);
        d->text[0] = '\0';
        d->len  = 0;
    }

    /* 状态初始化 */
    d->state    = -1;
    d->cursor   = 0;
    d->scroll_x = d->scroll_y = 0;

    /* 行表初始化 */
    d->lines     = NULL;
    d->line_cnt  = d->line_cap = 0;
    rt_build_lines(d);

    return rt;
}

/* ---------- draw & focus ---------- */
static void richtext_draw(TuiNode *n, void *event)
{
    RichTextData *d = n->data;
    if (!n->bits.focus && d->state == 0) return;
    if (n->bits.focus) richtext_focus(d, n, event);

    rt_scroll_to_cursor(d, n);

    const int bw    = d->default_style.border ? 1 : 0;
    const int scroll_w = d->show_scroll ? 2 : 0;
    const int vis_w = n->bounds.w - 2 * bw - scroll_w;
    const int vis_h = n->bounds.h - 2 * bw;
    if (vis_w <= 0 || vis_h <= 0) return;

    /* 背景 & 边框 */
    canvas_draw((rect_t){ n->bounds.x, n->bounds.y, n->bounds.w, n->bounds.h },
                "", d->default_style);

    /* 绘制可见行 */
    for (int row = 0; row < vis_h; ++row) {
        size_t line_idx = d->scroll_y + row;
        if (line_idx >= d->line_cnt) break;

        RichLine *ln = &d->lines[line_idx];

        size_t byte_off = utf8_advance(d->text + ln->off, 0, d->scroll_x);
        if (byte_off >= ln->len) continue;

        int char_width   = utf8_swidth_len(d->text + ln->off + byte_off,
                                        ln->len - byte_off);
        int visible_w    = MIN(char_width, vis_w);
        size_t byte_take = utf8_trunc_width(d->text + ln->off + byte_off,
                                            visible_w);

        char tmp[byte_take + 1];
        memcpy(tmp, d->text + ln->off + byte_off, byte_take);
        tmp[byte_take] = '\0';

        canvas_draw((rect_t){ n->abs_x + bw, n->abs_y + bw + row, vis_w, 1 },
                    tmp,
                    (style_t){ .fg = d->default_style.fg,
                            .text = 1,
                            .bg  = d->default_style.bg });
    }

    if (d->show_scroll) {
        const int sb_x = n->abs_x + n->bounds.w - bw - 1;               // 最右列
        const int sb_h = vis_h;                                         // 指示器总高度
        const int slot_h = MAX(1, sb_h * vis_h / (int)d->line_cnt);     // 滑块高
        const int slot_y = sb_h * d->scroll_y / (int)d->line_cnt;       // 滑块偏移

        canvas_draw((rect_t){ sb_x, n->abs_y + bw, 1, sb_h }, "", (style_t){ .bg = 11, .rect = 1 });
        canvas_draw((rect_t){ sb_x, n->abs_y + bw + slot_y, 1, 1 }, " ", (style_t){ .fg = 5, .text = 1 });
    }

    /* 光标 */
    if (n->bits.focus) {
        size_t cur_line, cur_col;
        rt_pos_to_line_col(d, d->cursor, &cur_line, &cur_col);

        int cur_row     = (int)cur_line - d->scroll_y;
        int cur_col_vis = (int)cur_col  - d->scroll_x;

        if (cur_row >= 0 && cur_row < vis_h &&
            cur_col_vis >= 0 && cur_col_vis < vis_w) {
            canvas_cursor_move(n->abs_x + bw + cur_col_vis + 1,
                            n->abs_y + bw + cur_row + 1,
                            1);
        }
    }
}

static void richtext_focus(RichTextData *d, TuiNode *rt, void *event)
{
    if (!event) return;
    event_t *ev = (event_t *)event;

    if (ev->type == EVENT_KEY)
        rt_handle_key(d, rt, &ev->key);
    else if (ev->type == EVENT_MOUSE && ev->mouse.type == MOUSE_PRESS)
        rt_handle_mouse(d, rt, &ev->mouse);
}