#include "core/widgets.h"
#include "core/c_test.h"
#include "core/log.h"

static inline TuiNode *create_root(void) {
    int w, h;
    term_size(&w, &h);
    term_init();
    term_clear();
    canvas_init(w, h);
    return tui_node_new(0, 0, w, h);
}

static void update(TuiNode *r, void *ev) {
    canvas_cursor_clear();
    tui_calc(r);
    tui_draw(r, ev);
    canvas_flush();
}

TEST(input, test) {
    setlocale(LC_ALL, "");
    TuiNode *root = create_root();

    InputBoxData ibox = {0};              
    ibox.st = (style_t){ .fg = 7, .bg = 0, .text = 1, .rect = 1, .border = 1, .border_st = 2 };
    ibox.st_focus = (style_t){ .fg = 7, .bg = 3, .text = 1, .rect = 1, .border = 1, .border_st = 2, .border_fg = 2 };
    TuiNode *ib = inputbox_new((TuiRect){ 5, 5, 30, 3 }, &ibox);
    tui_node_add(root, ib);

    update(root, NULL);
    while (1) {
        event_t ev = read_event();
        if (ev.type == EVENT_KEY && ev.key.key[0] == K_ESCAPE)
            break;

        if (ev.type == EVENT_MOUSE)
            tui_hit_node(root, ev.mouse.x - 1, ev.mouse.y - 1, ev.mouse.type == MOUSE_PRESS ? 1 : 0);

        update(root, &ev);
    }

    free(ibox.text);     
    canvas_free();
    term_restore();
}