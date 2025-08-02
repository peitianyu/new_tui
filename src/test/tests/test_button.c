#include "core/widgets.h"
#include "core/c_test.h"

TEST(button, test) {
    setlocale(LC_ALL, "");
    int w, h; term_size(&w, &h);
    term_init();
    term_clear();
    canvas_init(w, h);

    TuiNode *root = tui_node_new(0, 0, w, h);
    ButtonData data = { "hello", (style_t){ .fg = 2, .text = 1, .rect = 1, .border = 0, .align_horz = 1, .align_vert = 1 } };
    TuiNode  *btn  = button_new((TuiRect){5, 5, 7, 1}, "button", &data);
    tui_node_add(root, btn);

    tui_calc(root);
    tui_draw_init(root);
    canvas_flush();
    while (1) {           
        event_t e = read_event();
        if (e.type == EVENT_MOUSE) {
            tui_hit_node(root, e.mouse.x - 1, e.mouse.y - 1, e.mouse.type == MOUSE_PRESS ? 1 : 0);
        }

        tui_calc(root);
        tui_draw(root, &e);
        canvas_flush();
        if (e.type == EVENT_KEY && e.key.num == 1 && e.key.key[0] == K_ESCAPE)
            break;
    }

    canvas_free();
    term_restore();
}