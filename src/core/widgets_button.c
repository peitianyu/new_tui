#include "widgets.h"

static void button_draw(TuiNode *btn, void *event);

TuiNode *button_new(TuiRect r, const char *label, ButtonData *data) {
    TuiNode *b = tui_node_new(r.x, r.y, r.w, r.h);
    b->id = label;
    b->bits.focusable = 1;   
    b->data = data;
    b->draw = button_draw;
    return b;
}

static void button_draw(TuiNode *btn, void *event) {
    if (!btn) return;

    ButtonData data = *(ButtonData *)btn->data;
    style_t st = data.st;
    if(btn->bits.hover == 1)      st.bg = 3;
    else if(btn->bits.focus == 1) st.bg = 4;
    else                          st.bg = 5;

    canvas_draw((rect_t){ btn->abs_x, btn->abs_y, btn->bounds.w, btn->bounds.h }, data.label, st);
    if(data.func) { data.func(&data, event); }
    
    btn->bits.focus = 0;
}