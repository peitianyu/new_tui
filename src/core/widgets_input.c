#include "widgets.h"
#include <string.h>
#include <stdlib.h>

static inline int utf8_len(char c)
{
    unsigned char u = (unsigned char)c;
    if (u < 0x80)           return 1;
    if ((u & 0xE0) == 0xC0) return 2;
    if ((u & 0xF0) == 0xE0) return 3;
    return 4;
}

static inline int wcwidth_fast(uint32_t cp)
{
    return (cp >= 0x4E00 && cp <= 0x9FFF) ||
           (cp >= 0x3400 && cp <= 0x4DBF) ||
           (cp >= 0xF900 && cp <= 0xFAFF) ||
           (cp >= 0xFF01 && cp <= 0xFF5E) ? 2 : 1;
}

static int byte_to_col(const char *s, size_t byte)
{
    int col = 0;
    for (const char *p = s; *p && (size_t)(p - s) < byte; ) {
        int len = utf8_len(*p);
        uint32_t cp = 0;
        for (int i = 0; i < len; ++i) cp = (cp << 8) | (unsigned char)p[i];
        col += wcwidth_fast(cp);
        p += len;
    }
    return col;
}

static size_t col_to_byte(const char *s, int target)
{
    int col = 0;
    const char *p = s;
    while (*p) {
        int len = utf8_len(*p);
        uint32_t cp = 0;
        for (int i = 0; i < len; ++i) cp = (cp << 8) | (unsigned char)p[i];
        int w = wcwidth_fast(cp);
        if (col + w > target) break;
        col += w;
        p += len;
    }
    return p - s;
}

TuiNode *input_new(TuiRect r, const char *id, struct InputData *d, void (*draw)(TuiNode *, void *))
{
    TuiNode *n = tui_node_new(r.x, r.y, r.w, r.h);
    n->id = id; n->bits.focusable = 1;
    n->data = d; n->draw = draw ?: input_draw;

    if (!d->text) d->text = strdup("");
    d->cursor_pos = strlen(d->text);
    d->vis_start  = 0;

    int vis = r.w - 2;
    int col = byte_to_col(d->text, d->cursor_pos);
    if (col >= vis) d->vis_start = col - vis + 1;
    return n;
}

void input_handle_event(TuiNode *n, event_t *e)
{
    if (!n || !n->bits.focus) return;
    struct InputData *d = n->data;
    int vis = n->bounds.w - 2;

    if (e->type == EVENT_MOUSE && e->mouse.type == MOUSE_PRESS) {
        int x = e->mouse.x - n->abs_x - 1;
        int y = e->mouse.y - n->abs_y - 2;
        if (x >= 0 && x < vis && y >= 0 && y < n->bounds.h - 2)
            d->cursor_pos = col_to_byte(d->text, x + d->vis_start);
    }

    if (e->type == EVENT_KEY) {
        for (int i = 0; i < e->key.num; ++i) {
            if (e->key.type[i] == KEY_NORMAL) {
                int ch = e->key.key[i];
                size_t len = strlen(d->text);
                if (ch < 256 && len < d->max_length) {
                    char *tmp = malloc(len + 2);
                    memcpy(tmp, d->text, d->cursor_pos);
                    tmp[d->cursor_pos] = (char)ch;
                    strcpy(tmp + d->cursor_pos + 1, d->text + d->cursor_pos);
                    free(d->text); d->text = tmp;
                    ++d->cursor_pos;
                }
            } else if (e->key.type[i] == KEY_SPECIAL) {
                switch (e->key.key[i]) {
                case K_BACKSPACE:
                    if (d->cursor_pos) {
                        int len = utf8_len(d->text[d->cursor_pos - 1]);
                        memmove(d->text + d->cursor_pos - len,
                                d->text + d->cursor_pos,
                                strlen(d->text) - d->cursor_pos + 1);
                        d->cursor_pos -= len;
                    }
                    break;
                case K_DEL:
                    if (d->text[d->cursor_pos]) {
                        int len = utf8_len(d->text[d->cursor_pos]);
                        memmove(d->text + d->cursor_pos,
                                d->text + d->cursor_pos + len,
                                strlen(d->text) - d->cursor_pos - len + 1);
                    }
                    break;
                case K_LEFT:
                    if (d->cursor_pos)
                        d->cursor_pos -= utf8_len(d->text[d->cursor_pos - 1]);
                    break;
                case K_RIGHT:
                    if (d->text[d->cursor_pos])
                        d->cursor_pos += utf8_len(d->text[d->cursor_pos]);
                    break;
                case K_HOME:
                    d->cursor_pos = 0;
                    break;
                case K_END:
                    d->cursor_pos = 0;
                    for (const char *p = d->text; *p; ) {
                        int len = utf8_len(*p);
                        d->cursor_pos += len;
                        p += len;
                    }
                    break;
                }
            }
        }
    }

    int col = byte_to_col(d->text, d->cursor_pos);
    if (col < d->vis_start)             d->vis_start = col;
    else if (col >= d->vis_start + vis) d->vis_start = col - vis + 1;
}

/* ---------- 绘制 ---------- */
void input_draw(TuiNode *n, void *evt)
{
    if (!n) return;
    struct InputData *d = n->data;
    if (evt) input_handle_event(n, evt);

    int vis = n->bounds.w - 2;

    size_t start = 0, col = 0;
    const char *p = d->text;
    for (; *p; ) {
        int len = utf8_len(*p);
        uint32_t cp = 0;
        for (int i = 0; i < len; ++i) cp = (cp << 8) | (unsigned char)p[i];
        int w = wcwidth_fast(cp);
        if (col + w > d->vis_start) break;
        col += w;
        start += len;
        p += len;
    }

    style_t st = d->st;
    st.rect = st.border = st.text = 1;
    st.bg   = n->bits.focus ? 4 : n->bits.hover ? 3 : 5;

    rect_t r = { n->abs_x, n->abs_y, n->bounds.w, n->bounds.h };
    canvas_draw(r, d->text + start, st);

    int ccol = byte_to_col(d->text, d->cursor_pos);
    int cx   = ccol - d->vis_start;
    cx = cx < 0 ? 0 : cx >= vis ? vis - 1 : cx;
    canvas_cursor_move(n->bits.focus, n->abs_x + 1 + cx, n->abs_y + 1, 1);
}