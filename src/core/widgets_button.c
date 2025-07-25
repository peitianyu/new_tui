#include "widgets.h"

TuiNode *button_new(TuiRect r, const char *label, struct ButtonData *data, void (*draw)(TuiNode *, void *)) {
    TuiNode *b = tui_node_new(r.x, r.y, r.w, r.h);
    b->id = label;
    b->bits.focusable = 1;   
    b->data = data;
    b->draw = draw;
    return b;
}

void button_draw(TuiNode *btn, void *event) {
    if (!btn) return;

    struct ButtonData data = *(struct ButtonData *)btn->data;
    style_t st = data.st;
    if(btn->bits.hover == 1)      st.bg = 3;
    else if(btn->bits.focus == 1) st.bg = 4;
    else                          st.bg = 5;
    rect_t r = { btn->abs_x, btn->abs_y, btn->bounds.w, btn->bounds.h };
    canvas_draw(r, btn->bits.focus ? "world" : data.label, st);
    
    btn->bits.focus = 0;
}