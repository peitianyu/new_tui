#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "core/render.h"
#include "core/tui.h"
#include "core/term.h"
#include "core/widgets.h"
#include "core/c_test.h"

TuiNode *g_root;

TEST(button, test) {
    setlocale(LC_ALL, "");
    int w, h; term_size(&w, &h);
    term_init();
    term_clear();
    canvas_init(w, h);

    g_root = tui_node_new(0, 0, w, h);
    struct ButtonData data = { "hello", (style_t){ .fg = 2, .text = 1, .cursor = 1, .rect = 1, .border = 0, .align_horz = 1, .align_vert = 1 } };
    TuiNode  *btn  = button_new((TuiRect){5, 5, 10, 3}, "button", &data, button_draw);
    tui_node_add(g_root, btn);

    tui_calc(g_root);
    tui_draw_init(g_root);
    canvas_flush();
    while (1) {           
        event_t e = read_event();
        if (e.type == EVENT_MOUSE) {
            tui_hit_node(g_root, e.mouse.x - 1, e.mouse.y - 1, e.mouse.type == MOUSE_PRESS ? 1 : 0);
        }

        tui_calc(g_root);
        tui_draw(g_root, &e);
        canvas_flush();
        if (e.type == EVENT_KEY && e.key.num == 1 && e.key.key[0] == K_ESCAPE)
            break;
    }

    canvas_free();
    term_restore();
}