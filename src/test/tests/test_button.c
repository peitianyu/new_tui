#include "core/widgets.h"
#include "core/c_test.h"
#include "core/log.h"

static void click(ButtonData *b, void *event) {
    b->st.bg = 1;
    b->label = "world";
}

TEST(button, test) {
    setlocale(LC_ALL, "");
    TuiNode *root = creat_root_node();
    ButtonData data = { "hello", (style_t){ .fg = 2, .bg = 5, .text = 1, .rect = 1, .align_horz = 1, .align_vert = 1 }};
    TuiNode  *btn  = button_new((TuiRect){5, 5, 9, 1}, "button", &data);
    tui_node_add(root, btn);

    tui_calc(root);
    tui_draw_init(root);
    canvas_flush();
    while (1) {           
        event_t e = read_event();
        if (e.type == EVENT_KEY && e.key.num == 1 && e.key.key[0] == K_ESCAPE)
            break;

        if (e.type == EVENT_MOUSE) 
            tui_hit_node(root, e.mouse.x - 1, e.mouse.y - 1, e.mouse.type == MOUSE_PRESS ? 1 : 0);

        tui_calc(root);
        tui_draw(root, &e);
        canvas_flush();
    }

    canvas_free();
    term_restore();
}