#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "core/render.h"
#include "core/tui.h"
#include "core/term.h"
#include "core/widgets.h"
#include "core/c_test.h"

static int utf8_char_len(const char *s) {
    unsigned char c = (unsigned char)*s;
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1; 
}

TuiNode *g_root;
TuiNode *g_input;

TEST(input, test) {
    setlocale(LC_ALL, "");
    int w, h; term_size(&w, &h);
    term_init();
    term_clear();
    canvas_init(w, h);

    g_root = tui_node_new(0, 0, w, h);
    
    // 创建输入框
    struct InputData input_data = {
        .text = 0, 
        .cursor_pos = 0,
        .max_length = 10,
        .st = (style_t){ .fg = 2, .text = 1, .cursor = 1, .rect = 1, .border = 1, .align_horz = 0, .align_vert = 1 }
    };
    g_input = input_new((TuiRect){10, 5, 30, 3}, "input", &input_data, input_draw);
    tui_node_add(g_root, g_input);
    
    struct ButtonData btn_data = { "Submit", (style_t){ .fg = 2, .text = 1, .rect = 1, .border = 0 } };
    TuiNode *btn = button_new((TuiRect){10, 9, 10, 3}, "button", &btn_data, NULL);
    tui_node_add(g_root, btn);

    tui_calc(g_root);
    tui_draw_init(g_root);
    canvas_flush();

    event_t e;
    
    while (1) {
        if (!input_ready(100)) continue;
        e = read_event();
        if (e.type == EVENT_KEY && e.key.num == 1 && e.key.key[0] == K_ESCAPE)
            break;

        if (e.type == EVENT_MOUSE) {
            tui_hit_node(g_root, e.mouse.x - 1, e.mouse.y - 1, e.mouse.type == MOUSE_PRESS ? 1 : 0);
        } else if (e.type == EVENT_KEY && e.key.key[0] == K_TAB) {
            tui_focus_next(g_root);
        }

        tui_calc(g_root);
        canvas_rest_cursor();
        tui_draw(g_root, &e);   
        canvas_flush();
    }

    // 清理
    free(input_data.text);
    canvas_free();
    term_restore();
}