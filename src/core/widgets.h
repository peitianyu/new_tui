#ifndef __WIDGETS_H__
#define __WIDGETS_H__

#include "render.h"
#include "tui.h"
#include "term.h"
#include "utf8.h"
#include "log.h"
#include <stddef.h>  


typedef struct ButtonData ButtonData;
struct ButtonData {
    char *label;
    style_t st;
    bool no_changed;
    void (*hover_func) (ButtonData *b);
    void (*focus_func) (ButtonData *b, void *event);
};
TuiNode *button_new(TuiRect r, const char *label, ButtonData *data);

typedef struct InputBoxData InputBoxData;
struct InputBoxData {
    char       *text;          /* 当前内容（动态分配/修改） */
    size_t      len;           /* 已用长度 */
    size_t      cap;           /* 缓冲区容量 */
    size_t      cursor;        /* 光标字符位置（0~len） */
    style_t     st;            /* 样式 */
    style_t     st_focus;      /* 聚焦时的样式 */
    size_t      scroll_x;      /* 水平滚动偏移量（字符宽度单位） */
    bool        no_changed;    /* 内容变化 */
    void (*func) (InputBoxData *d);
};
TuiNode *inputbox_new(TuiRect r, InputBoxData *data);

#endif // __WIDGETS_H__