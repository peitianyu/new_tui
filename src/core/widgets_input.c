/*
 * widgets_input.c  ——  精简、无重复、易维护版本
 */
#include "widgets.h"
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

/* ---------- 纯工具 ---------- */
typedef struct {
    size_t byte;   /* 字节下标 */
    int    col;    /* 逻辑列坐标 */
} Cursor;

static inline int utf8_len(char c) {
    unsigned char u = (unsigned char)c;
    if (u < 0x80)        return 1;
    if ((u & 0xE0) == 0xC0) return 2;
    if ((u & 0xF0) == 0xE0) return 3;
    if ((u & 0xF8) == 0xF0) return 4;
    return 1;
}

static inline int wcwidth_fast(uint32_t cp) {
    return ((cp >= 0x4E00 && cp <= 0x9FFF) ||
            (cp >= 0x3400 && cp <= 0x4DBF) ||
            (cp >= 0xF900 && cp <= 0xFAFF) ||
            (cp >= 0xFF01 && cp <= 0xFF5E)) ? 2 : 1;
}

static inline uint32_t utf8_to_cp(const char *s, int *len_out) {
    *len_out = utf8_len(*s);
    uint32_t cp = 0;
    for (int i = 0; i < *len_out; ++i)
        cp = (cp << 8) | (unsigned char)s[i];
    return cp;
}

/* 把字节位置转换成逻辑列坐标；返回 0 表示起点 */
static int byte_to_col(const char *text, size_t byte_pos) {
    int col = 0;
    const char *p = text;
    while (*p && (size_t)(p - text) < byte_pos) {
        int len;
        uint32_t cp = utf8_to_cp(p, &len);
        col += wcwidth_fast(cp);
        p += len;
    }
    return col;
}

/* 把逻辑列坐标转成最近字节位置；返回字节下标 */
static size_t col_to_byte(const char *text, int target_col) {
    int col = 0;
    const char *p = text;
    size_t byte = 0;
    while (*p) {
        int len;
        uint32_t cp = utf8_to_cp(p, &len);
        int w = wcwidth_fast(cp);
        if (col + w > target_col) break;
        col += w;
        byte += len;
        p += len;
    }
    return byte;
}

/* ---------- 构造 ---------- */
TuiNode *input_new(TuiRect r, const char *id, struct InputData *data,
                   void (*draw)(TuiNode *, void *)) {
    TuiNode *in = tui_node_new(r.x, r.y, r.w, r.h);
    in->id             = id;
    in->bits.focusable = 1;
    in->data           = data;
    in->draw           = draw ? draw : input_draw;

    if (!data->text) data->text = strdup("");
    data->cursor_pos = strlen(data->text);
    data->vis_start = 0;

    /* 初始化 vis_start */
    int visible_width = r.w - 2;
    int cursor_col = byte_to_col(data->text, data->cursor_pos);
    if (cursor_col >= visible_width)
        data->vis_start = cursor_col - visible_width + 1;

    return in;
}

/* ---------- 事件处理 ---------- */
void input_handle_event(TuiNode *in, event_t *e) {
    if (!in || !in->bits.focus) return;
    struct InputData *d = in->data;
    int visible_width   = in->bounds.w - 2;

    /* 1. 鼠标点击 */
    if (e->type == EVENT_MOUSE && e->mouse.type == MOUSE_PRESS) {
        int click_x = e->mouse.x - in->abs_x - 1;
        int click_y = e->mouse.y - in->abs_y - 2;
        if (click_x >= 0 && click_x < in->bounds.w - 2 &&
            click_y >= 0 && click_y < in->bounds.h - 2) {
            int target_col = click_x + d->vis_start;
            d->cursor_pos = col_to_byte(d->text, target_col);
        }
    }

    /* 2. 键盘 */
    if (e->type == EVENT_KEY) {
        for (int i = 0; i < e->key.num; ++i) {
            if (e->key.type[i] == KEY_NORMAL) {
                int ch = e->key.key[i];
                if (ch < 256 && strlen(d->text) < d->max_length) {
                    size_t old = strlen(d->text), new_len = old + 1;
                    char *tmp = malloc(new_len + 1);
                    strncpy(tmp, d->text, d->cursor_pos);
                    tmp[d->cursor_pos] = (char)ch;
                    strcpy(tmp + d->cursor_pos + 1,
                           d->text + d->cursor_pos);
                    free(d->text);
                    d->text = tmp;
                    d->cursor_pos += 1;
                }
            } else if (e->key.type[i] == KEY_SPECIAL) {
                switch (e->key.key[i]) {
                case K_BACKSPACE:
                    if (d->cursor_pos > 0) {
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
                    if (d->cursor_pos > 0)
                        d->cursor_pos -= utf8_len(d->text[d->cursor_pos - 1]);
                    break;
                case K_RIGHT:
                    if (d->text[d->cursor_pos])
                        d->cursor_pos += utf8_len(d->text[d->cursor_pos]);
                    break;
                }
            }
        }
    }

    /* 3. 自动滚动 */
    int cursor_col = byte_to_col(d->text, d->cursor_pos);
    if (cursor_col < d->vis_start)
        d->vis_start = cursor_col;
    else if (cursor_col >= d->vis_start + visible_width)
        d->vis_start = cursor_col - visible_width + 1;
}

/* ---------- 纯绘制 ---------- */
void input_draw(TuiNode *in, void *evt) {
    if (!in) return;
    struct InputData *d = in->data;
    int visible_width   = in->bounds.w - 2;

    /* 1. 先处理事件 */
    if (evt) input_handle_event(in, (event_t *)evt);

    /* 2. 计算可见字符串起点 */
    int current_col = 0;
    const char *p = d->text;
    size_t start_byte = 0;
    while (*p) {
        int len;
        uint32_t cp = utf8_to_cp(p, &len);
        int w = wcwidth_fast(cp);
        if (current_col + w > d->vis_start) break;
        current_col += w;
        start_byte += len;
        p += len;
    }

    /* 3. 渲染 */
    style_t st = d->st;
    int cursor_able = 0;
    if (in->bits.focus) { st.bg = 4; cursor_able = 1;
    } else if (in->bits.hover) { st.bg = 3; cursor_able = 0;
    } else { st.bg = 5; cursor_able = 0;}
    st.rect = st.border = st.text = 1;

    rect_t r_rect = { in->abs_x, in->abs_y, in->bounds.w, in->bounds.h };
    canvas_draw(r_rect, d->text + start_byte, st);

    /* 4. 光标 */
    int cursor_col = byte_to_col(d->text, d->cursor_pos);
    int cursor_x = cursor_col - d->vis_start;
    cursor_x = (cursor_x < 0) ? 0 :
               (cursor_x >= visible_width) ? visible_width - 1 : cursor_x;
    canvas_cursor_move(cursor_able, in->abs_x + 1 + cursor_x, in->abs_y + 1, 1);
}