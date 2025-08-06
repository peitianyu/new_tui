#include "widgets.h"
#include <stdlib.h>
#include <string.h>

static void pos_to_line_col(const RichTextData *d, size_t pos, size_t *line, size_t *col) {
    size_t lo = 0, hi = d->line_cnt;
    while (lo < hi) {
        size_t mid = (lo + hi) >> 1;
        if (d->lines[mid].off > pos) hi = mid;
        else                         lo = mid + 1;
    }
    *line = lo ? lo - 1 : 0;

    *col = 0;
    const char *p   = d->text + d->lines[*line].off;
    const char *end = d->text + pos;

    while (p < end) {
        unsigned cp = utf8_decode((const char **)&p);
        if (cp == '\n') { *col += 1; break; }
        *col += utf8_width(cp);
    }
}

static size_t line_col_to_pos(const RichTextData *d, size_t line, size_t col) {
    if (line >= d->line_cnt) return d->len;

    size_t off = d->lines[line].off;
    size_t lim = d->lines[line].len;
    const char *p   = d->text + off;
    const char *end = p + lim;

    size_t w = 0;
    while (p < end && *p != '\n' && w < col) {
        unsigned cp = utf8_decode((const char **)&p);
        w += utf8_width(cp);
    }
    return p - d->text;
}
static void build_lines(RichTextData *d) {
    d->line_cnt = 1;
    for (size_t i = 0; i < d->len; ++i)
        if (d->text[i] == '\n') ++d->line_cnt;

    if (d->line_cnt > d->line_cap) {
        d->line_cap = d->line_cnt + 8;
        d->lines = realloc(d->lines, d->line_cap * sizeof *d->lines);
    }

    size_t line = 0, start = 0;
    for (size_t i = 0; i < d->len; ++i) {
        if (d->text[i] == '\n') {
            d->lines[line++] = (RichLine){ start, i - start };
            start = i + 1;
        }
    }
    d->lines[line++] = (RichLine){ start, d->len - start };
    d->line_cnt = line;
}

/* -------------------- TuiNode 回调 -------------------- */
static void richtext_draw(TuiNode *n, void *event);
static void richtext_focus(RichTextData *d, TuiNode *rt, void *event);

TuiNode *richtext_new(TuiRect r, RichTextData *d)
{
    TuiNode *rt = tui_node_new(r.x, r.y, r.w, r.h);
    rt->bits.focusable = 1;
    rt->draw           = richtext_draw;
    rt->data           = d;

    if (!d->cap) d->cap = 256;
    if (!d->text) {
        d->text = malloc(d->cap);
        d->text[0] = '\0';
        d->len = 0;
    }
    d->state = -1;
    d->cursor = 0;
    d->scroll_x = d->scroll_y = 0;

    d->lines     = NULL;
    d->line_cnt  = d->line_cap = 0;
    build_lines(d);
    return rt;
}

static void richtext_draw(TuiNode *n, void *event)
{
    RichTextData *d = n->data;
    if (!n->bits.focus && d->state == 0) return;
    if (n->bits.focus) richtext_focus(d, n, event);

    const int bw = d->default_style.border ? 1 : 0;
    const int vis_w = n->bounds.w - 2 * bw;
    const int vis_h = n->bounds.h - 2 * bw;
    if (vis_w <= 0 || vis_h <= 0) return;

    canvas_draw((rect_t){n->bounds.x, n->bounds.y, n->bounds.w, n->bounds.h}, "", d->default_style);
    for (int row = 0; row < vis_h; ++row) {
        size_t line_idx = d->scroll_y + row;
        if (line_idx >= d->line_cnt) break;

        RichLine *ln = &d->lines[line_idx];

        size_t byte_off = utf8_advance(d->text + ln->off, 0, d->scroll_x);
        if (byte_off >= ln->len) continue;  
        int char_width = utf8_swidth_len(d->text + ln->off + byte_off, ln->len - byte_off);
        int visible_width = char_width > vis_w ? vis_w : char_width;
        size_t byte_take = utf8_trunc_width(d->text + ln->off + byte_off, visible_width);

        char tmp[vis_w * 4 + 1];  
        memcpy(tmp, d->text + ln->off + byte_off, byte_take);
        tmp[byte_take] = '\0';

        canvas_draw((rect_t){ n->abs_x + bw, n->abs_y + bw + row, vis_w, 1 }, 
                    tmp, (style_t){ .fg = d->default_style.fg, .text = 1, .bg = d->default_style.bg });
    }

    if (n->bits.focus) {
        size_t cur_line, cur_col;
        pos_to_line_col(d, d->cursor, &cur_line, &cur_col);

        int cur_row = (int)cur_line - d->scroll_y;
        int cur_col_vis = (int)cur_col - d->scroll_x; 

        if (cur_row >= 0 && cur_row < vis_h &&
            cur_col_vis >= 0 && cur_col_vis < vis_w) {
            canvas_cursor_move(n->abs_x + bw + cur_col_vis+1, n->abs_y + bw + cur_row + 1, 1);
        }
    }
}

static void richtext_focus(RichTextData *d, TuiNode *rt, void *event)
{
    if (!event) return;
    event_t *ev = (event_t *)event;

    if (ev->type == EVENT_KEY) {
        key_event_t *k = &ev->key;
        for (int i = 0; i < k->num; ++i) {
            int ch = k->key[i];
            if (k->type[i] == KEY_SPECIAL) {
                switch (ch) {
                    case K_LEFT:
                        if (d->cursor > 0) {
                            d->cursor = utf8_prev(d->text, d->cursor);
                        }
                        break;
                    case K_RIGHT:
                        if (d->cursor < d->len) {
                            d->cursor = utf8_advance(d->text, d->cursor, 1);
                        }
                        break;
                    case K_UP:
                        {
                            size_t line, col;
                            pos_to_line_col(d, d->cursor, &line, &col);
                            if (line > 0) {
                                d->cursor = line_col_to_pos(d, line - 1, col);
                            }
                        }
                        break;
                    case K_DOWN:
                        {
                            size_t line, col;
                            pos_to_line_col(d, d->cursor, &line, &col);
                            if (line < d->line_cnt - 1) {
                                d->cursor = line_col_to_pos(d, line + 1, col);
                            }
                        }
                        break;
                    case K_HOME:
                        {
                            size_t line, col;
                            pos_to_line_col(d, d->cursor, &line, &col);
                            d->cursor = line_col_to_pos(d, line, 0);
                        }
                        break;
                    case K_END:
                        {
                            size_t line, col;
                            pos_to_line_col(d, d->cursor, &line, &col);
                            d->cursor = line_col_to_pos(d, line, d->lines[line].len);
                        }
                        break;
                    case K_BACKSPACE:
                        if (d->cursor > 0) {
                            size_t prev = utf8_prev(d->text, d->cursor);
                            size_t step = d->cursor - prev;
                            memmove(d->text + prev, d->text + d->cursor, d->len - d->cursor + 1);
                            d->cursor = prev;
                            d->len -= step;
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
                        if (d->len + 1 >= d->cap) {
                            d->cap *= 2;
                            d->text = realloc(d->text, d->cap);
                        }
                        memmove(d->text + d->cursor + 1, d->text + d->cursor, d->len - d->cursor + 1);
                        d->text[d->cursor] = '\n';
                        d->cursor += 1;
                        d->len += 1;
                        break;
                }
            } else {
                // Insert character
                if (d->len + 4 >= d->cap) {
                    d->cap *= 2;
                    d->text = realloc(d->text, d->cap);
                }
                char tmp[4];
                int bytes = utf8_encode(ch, tmp);
                if (!bytes) return;

                memmove(d->text + d->cursor + bytes, d->text + d->cursor, d->len - d->cursor + 1);
                memcpy(d->text + d->cursor, tmp, bytes);
                d->cursor += bytes;
                d->len += bytes;            }
            build_lines(d);
        }
    } else if (ev->type == EVENT_MOUSE && ev->mouse.type == MOUSE_PRESS) {
        int bw = d->default_style.border ? 1 : 0;
        int vis_w = rt->bounds.w - 2 * bw;
        int vis_h = rt->bounds.h - 2 * bw;

        int click_x = ev->mouse.x - rt->abs_x - bw;
        int click_y = ev->mouse.y - rt->abs_y - bw - 1;

        if (click_x < 0 || click_x >= vis_w || click_y < 0 || click_y >= vis_h) return;

        size_t line = d->scroll_y + click_y;
        if (line >= d->line_cnt) return;

        size_t col = click_x + d->scroll_x;
        d->cursor = line_col_to_pos(d, line, col);
    }
}