// widgets_input.c
#include "widgets.h"
#include <string.h>
#include <stdlib.h>

/* ---------- 工具 ---------- */
static int utf8_char_len(const char *s) {
    unsigned char c = (unsigned char)*s;
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1; 
}

static inline int wcwidth_fast(uint32_t cp) {
    return ((cp >= 0x4E00 && cp <= 0x9FFF) ||
            (cp >= 0x3400 && cp <= 0x4DBF) ||
            (cp >= 0xF900 && cp <= 0xFAFF) ||
            (cp >= 0xFF01 && cp <= 0xFF5E)) ? 2 : 1;
}

/* ---------- 构造 ---------- */
TuiNode *input_new(TuiRect r, const char *id, struct InputData *data,
                   void (*draw)(TuiNode *, void *)) {
    TuiNode *in = tui_node_new(r.x, r.y, r.w, r.h);
    in->id       = id;
    in->bits.focusable = 1;
    in->data     = data;
    in->draw     = draw ? draw : input_draw;

    if (!data->text) data->text = strdup("");
    data->cursor_pos = strlen(data->text);
    return in;
}

/* ---------- 绘制+编辑 ---------- */
void input_draw(TuiNode *in, void *evt) {
    if (!in) return;
    struct InputData *data = (struct InputData *)in->data;

    /* ---------- 1. 仅 focus 时处理键盘 ---------- */
    if (evt && in->bits.focus == 1) {
        event_t *e = (event_t *)evt;
        if (e->type == EVENT_KEY) {
            for (int i = 0; i < e->key.num; ++i) {
                if (e->key.type[i] == KEY_NORMAL) {
                    int key = e->key.key[i];
                    if (key < 256 && strlen(data->text) < data->max_length) {
                        size_t old = strlen(data->text);
                        char *tmp  = malloc(old + 2);
                        strncpy(tmp, data->text, data->cursor_pos);
                        tmp[data->cursor_pos] = (char)key;
                        strcpy(tmp + data->cursor_pos + 1,
                               data->text + data->cursor_pos);
                        free(data->text);
                        data->text = tmp;
                        data->cursor_pos += 1;
                    }
                } else if (e->key.type[i] == KEY_SPECIAL) {
                    switch (e->key.key[i]) {
                    case K_BACKSPACE:
                        if (data->cursor_pos > 0) {
                            int len = utf8_char_len(data->text +
                                                    data->cursor_pos - 1);
                            memmove(data->text + data->cursor_pos - len,
                                    data->text + data->cursor_pos,
                                    strlen(data->text) - data->cursor_pos + 1);
                            data->cursor_pos -= len;
                        }
                        break;
                    case K_DEL:
                        if (data->text[data->cursor_pos]) {
                            int len = utf8_char_len(data->text +
                                                    data->cursor_pos);
                            memmove(data->text + data->cursor_pos,
                                    data->text + data->cursor_pos + len,
                                    strlen(data->text) -
                                    data->cursor_pos - len + 1);
                        }
                        break;
                    case K_LEFT:
                        if (data->cursor_pos > 0) {
                            data->cursor_pos -=
                                utf8_char_len(data->text +
                                              data->cursor_pos - 1);
                        }
                        break;
                    case K_RIGHT:
                        if (data->text[data->cursor_pos]) {
                            data->cursor_pos +=
                                utf8_char_len(data->text +
                                              data->cursor_pos);
                        }
                        break;
                    case K_ENTER: /* 可根据需要回调 */ break;
                    case K_TAB:   /* 由外层统一处理 */   break;
                    }
                }
            }
        }
    }

    /* ---------- 2. 渲染 ---------- */
    style_t st = data->st;
    if (in->bits.focus == 1) {
        st.bg = 4;
        st.cursor = 1;
        term_cursor_show(1);
    } else if (in->bits.hover == 1) {
        st.bg = 3;
        st.cursor = 0;          /* hover 不显示光标 */
        term_cursor_show(0);
    } else {
        st.bg = 5;
        st.cursor = 0;
        term_cursor_show(0);
    }
    st.rect   = 1;
    st.border = 1;
    st.text   = 1;

    rect_t r = { in->abs_x, in->abs_y, in->bounds.w, in->bounds.h };
    canvas_draw(r, data->text, st);

    /* ---------- 3. 光标定位（仅 focus） ---------- */
    if (in->bits.focus == 1) {
        int cursor_x = 0;
        const char *p = data->text;
        for (int i = 0; i < data->cursor_pos && *p;) {
            int len = utf8_char_len(p);
            uint32_t cp = 0;
            for (int j = 0; j < len; ++j)
                cp = (cp << 8) | (unsigned char)p[j];
            p += len;
            cursor_x += wcwidth_fast(cp);
        }
        canvas_cursor_move(in->abs_x + cursor_x, in->abs_y + 1);
    }
}