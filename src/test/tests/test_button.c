#include "core/widgets.h"
#include "core/c_test.h"
#include "core/log.h"

static void click(ButtonData *b, void *event) {
    event_t e = *(event_t*)event;
    if(e.mouse.button == MOUSE_RIGHT)       b->st.bg = 6;
    else if(e.mouse.button == MOUSE_MIDDLE) b->st.bg = 2;
    else if(e.mouse.button == MOUSE_LEFT)   b->st.bg = 7;
}

static inline TuiNode *creat_root_node(void) {
    int w, h; term_size(&w, &h);
    term_init();
    term_clear();
    canvas_init(w, h);
    return tui_node_new(0, 0, w, h);
}

static void updata_draw(TuiNode *root, void *event) {
    tui_calc(root);
    tui_draw(root, event);
    canvas_flush();
}

TEST(button, test) {
    setlocale(LC_ALL, "");
    TuiNode *root = creat_root_node();

    ButtonData data = { "hello", (style_t){ .fg = 2, .bg = 5, .text = 1, .rect = 1, .border = 1, 
                        .align_horz = 1, .align_vert = 1, .border_st = 3 }, .focus_func = click};
    TuiNode  *btn  = button_new((TuiRect){5, 5, 9, 3}, "button", &data);
    tui_node_add(root, btn);

    updata_draw(root, NULL);
    while (1) {           
        event_t e = read_event();
        if (e.type == EVENT_KEY && e.key.key[0] == K_ESCAPE)
            break;

        if (e.type == EVENT_MOUSE) 
            tui_hit_node(root, e.mouse.x - 1, e.mouse.y - 1, e.mouse.type == MOUSE_PRESS ? 1 : 0);

        updata_draw(root, &e);
    }
    canvas_free();
    term_restore();
}