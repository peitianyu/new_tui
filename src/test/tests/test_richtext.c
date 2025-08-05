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

TEST(richtext, test) {
    setlocale(LC_ALL, "");
    TuiNode *root = create_root();

    RichTextData rt = {0};
    rt.default_style = (style_t){ .fg = 7, .bg = 0, .text = 1, .wrap = 1 };
    rt.text = strdup("Hello 世界！\nThis is a RichText widget demo.\n支持中文、Emoji 😊 与样式。");
    rt.len = strlen(rt.text);
    TuiNode *richtext = richtext_new((TuiRect){1, 1, 78, 20}, &rt);
    tui_node_add(root, richtext);

    update(root, NULL);
    while (1) {
        event_t ev = read_event();
        if (ev.type == EVENT_KEY && ev.key.key[0] == K_ESCAPE)
            break;

        if (ev.type == EVENT_MOUSE)
            tui_hit_node(root, ev.mouse.x - 1, ev.mouse.y - 1, ev.mouse.type == MOUSE_PRESS ? 1 : 0);

        update(root, &ev);
    }

    free(rt.text);     
    canvas_free();
    term_restore();
}