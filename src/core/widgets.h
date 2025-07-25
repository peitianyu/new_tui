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
    char *text;          
    size_t cursor_pos;     // 光标位置（字节偏移）
    int vis_start;         // 可见区域起始位置（屏幕列偏移）
    size_t max_length;      
    style_t st;                 
};
TuiNode *input_new(TuiRect r, const char *id, struct InputData *data, void (*draw)(TuiNode *, void *));
void input_draw(TuiNode *in, void *event);

#endif // __WIDGETS_H__