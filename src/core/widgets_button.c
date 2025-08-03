#include "widgets.h"

static void button_draw(TuiNode *btn, void *event);
static void hover_func(ButtonData *b);
static void focus_func(ButtonData *b, void *event);

TuiNode *button_new(TuiRect r, const char *label, ButtonData *data) {
    TuiNode *b = tui_node_new(r.x, r.y, r.w, r.h);
    b->id = label;
    b->bits.focusable = 1;  
    if(!data->hover_func) { data->hover_func = hover_func; }
    if(!data->focus_func) { data->focus_func = focus_func; }
    b->data = data;
    b->draw = button_draw;
    return b;
}

static void hover_func(ButtonData *b) {
    b->st.bg = 3;
}

static void focus_func(ButtonData *b, void *event) {
    b->st.bg = 4;
}

static void button_draw(TuiNode *btn, void *event) {
    if (!btn) return;

    ButtonData *data = (ButtonData *)btn->data;
    style_t st = data->st;
    if(btn->bits.hover == 1)      { data->hover_func(data); }
    else if(btn->bits.focus == 1) { data->focus_func(data, event); }

    canvas_draw((rect_t){ btn->abs_x, btn->abs_y, btn->bounds.w, btn->bounds.h }, data->label, data->st);
    data->st = st;

    btn->bits.focus = 0;
}