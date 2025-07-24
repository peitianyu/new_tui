#ifndef __WIDGETS_H__
#define __WIDGETS_H__

#include "render.h"
#include "tui.h"
#include "term.h"

struct ButtonData{
    char *label;
    style_t st;
};
TuiNode *button_new(TuiRect r, const char *label, struct ButtonData *data, void (*draw)(TuiNode *, void *));
void button_draw(TuiNode *btn, void *event);

struct InputData {
    char *text;             // 当前文本
    int   cursor_pos;       // 光标位置（以字节为单位）
    int   max_length;       // 最大长度
    style_t st;             // 样式       
};
TuiNode *input_new(TuiRect r, const char *id, struct InputData *data, void (*draw)(TuiNode *, void *));
void input_draw(TuiNode *in, void *event);

#endif // __WIDGETS_H__