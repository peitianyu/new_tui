#include "widgets.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static void inputbox_draw(TuiNode *ib, void *event);
static void inputbox_focus(InputBoxData *d, TuiNode* ib, void *event);
static void inputbox_func(InputBoxData *d) {}
/* ----------------------------- 构造 -------------------------------- */
TuiNode *inputbox_new(TuiRect r, InputBoxData *d) {
    TuiNode *n = tui_node_new(r.x, r.y, r.w, r.h);
    n->bits.focusable = 1;
    n->data = d;
    n->draw = inputbox_draw;

    if (!d->cap) d->cap = 128;
    if (!d->text) {
        d->text = malloc(d->cap);
        d->text[0] = '\0';
        d->len = 0;
        d->cursor = 0;
    }
    return n;
}

/* ----------------------------- 绘制 -------------------------------- */
static void inputbox_draw(TuiNode *ib, void *event) {
    InputBoxData *d = (InputBoxData *)ib->data;
    style_t st = d->st;
    if(d->no_changed && ib->bits.focus == 0) { return; }
    d->no_changed = 1;

    if (ib->bits.focus) { inputbox_focus(d, ib, event); if(d->func) d->func(d); st = d->st_focus; }

    /* 可视宽度（减去边框） */
    int bw = st.border ? 1 : 0;
    int vis_w = ib->bounds.w - 2 * bw;
    if (vis_w <= 0) { return; }

    /* 计算字符级别的光标偏移量（以字符宽度为单位） */
    int cursor_col = utf8_swidth_len(d->text, d->cursor);
    int scroll = d->scroll_x;
    if (cursor_col < scroll)               { scroll = cursor_col; }
    else if (cursor_col >= scroll + vis_w) { scroll = cursor_col - vis_w + 1; }
    d->scroll_x = scroll;
    
    /* 跳过滚动位置 */
    const char *p = d->text;
    size_t skip_bytes = utf8_advance(p, 0, scroll);
    p += skip_bytes;

    /* 截取可见文本 */
    char vis_buf[vis_w * 4 + 1];
    size_t vis_bytes = utf8_trunc_width(p, vis_w);
    strncpy(vis_buf, p, vis_bytes);
    vis_buf[vis_bytes] = '\0';
    if(vis_w - utf8_swidth_len(vis_buf, vis_bytes) == 2) {
        memmove(vis_buf + 1, vis_buf, vis_bytes);
        vis_buf[0] = ' ';
        vis_buf[vis_bytes+1] = '\0';
    }

    /* 绘制文本 */
    canvas_draw((rect_t){ ib->abs_x, ib->abs_y, ib->bounds.w, ib->bounds.h }, vis_buf, st);

    /* 光标位置（屏幕坐标） */
    if(!ib->bits.focus) { return; }
    int cursor_scr_x = ib->abs_x + bw + (cursor_col - d->scroll_x);
    int cursor_scr_y = ib->abs_y + bw;
    canvas_cursor_move(cursor_scr_x+1, cursor_scr_y+1, 1);
}

/* ----------------------------- 编辑操作 ---------------------------- */
static void inputbox_insert_char(InputBoxData *d, uint32_t ch) {
    if (d->len + 4 >= d->cap) {
        d->cap *= 2;
        d->text = realloc(d->text, d->cap);
    }
    char tmp[4];
    int bytes = utf8_encode(ch, tmp);
    if (!bytes) { return; }

    memmove(d->text + d->cursor + bytes, d->text + d->cursor, d->len - d->cursor + 1);
    memcpy(d->text + d->cursor, tmp, bytes);
    d->cursor += bytes;
    d->len += bytes;
}

static void inputbox_backspace(InputBoxData *d) {
    if (d->cursor == 0) { return; }

    const char *p = d->text;
    const char *prev = p;
    while (p < d->text + d->cursor) {
        prev = p;
        utf8_decode(&p);
    }

    size_t step = (d->text + d->cursor) - prev;
    if (step > d->cursor) { return; }

    memmove(d->text + d->cursor - step, d->text + d->cursor, d->len - d->cursor + 1);
    d->cursor -= step;
    d->len -= step;
}

static void inputbox_delete(InputBoxData *d) {
    if (d->cursor >= d->len) { return; }
    const char *next = d->text + d->cursor;
    utf8_decode(&next);
    size_t step = next - (d->text + d->cursor);
    memmove(d->text + d->cursor, d->text + d->cursor + step, d->len - d->cursor - step + 1);
    d->len -= step;
}

static void inputbox_move_cursor(InputBoxData *d, int dir) {
    if (dir < 0 && d->cursor > 0) {
        const char *p = d->text;
        size_t prev_pos = 0;
        while (p < d->text + d->cursor) {
            prev_pos = p - d->text;
            utf8_decode(&p);
        }
        d->cursor = prev_pos;
    } else if (dir > 0 && d->cursor < d->len) {
        const char *next = d->text + d->cursor;
        utf8_decode(&next);
        d->cursor = next - d->text;
    }
}

static void inputbox_clear_line(InputBoxData *d) {
    d->text[0] = '\0';
    d->len = d->cursor = 0;
}

/* ----------------------------- 事件处理 ---------------------------- */
static void inputbox_focus(InputBoxData *d, TuiNode* ib, void *event) {
    if (!event) { return; }
    event_t *ev = (event_t *)event;

    if (ev->type == EVENT_KEY) {
        key_event_t *k = &ev->key;
        for (int i = 0; i < k->num; ++i) {
            int ch = k->key[i];
            if (k->type[i] == KEY_SPECIAL) {
                switch (ch) {
                    case K_BACKSPACE: inputbox_backspace(d); break;
                    case K_DEL:       inputbox_delete(d);    break;
                    case K_LEFT:      inputbox_move_cursor(d, -1); break;
                    case K_RIGHT:     inputbox_move_cursor(d,  1); break;
                    case K_HOME:      d->cursor = 0; break;
                    case K_END:       d->cursor = d->len; break;
                    case CTRL_KEY('U'): inputbox_clear_line(d); break;
                }
            } else {
                inputbox_insert_char(d, k->key[i]);
            }
        }
    } else if (ev->type == EVENT_MOUSE && ev->mouse.type == MOUSE_PRESS) {
        int bw = d->st.border ? 1 : 0;
        int vis_w = ib->bounds.w - 2 * bw;
        int click_col = ev->mouse.x - ib->abs_x - bw + d->scroll_x;
        if (click_col < 0) click_col = 0;
    
        const char *p = d->text;
        int col = 0;
        size_t pos = 0;
        while (*p) {
            uint32_t cp = utf8_decode(&p);
            int w = utf8_width(cp);
            if (col + w > click_col) break;
            col += w;
            pos = p - d->text;
        }
        d->cursor = pos;
    }
}