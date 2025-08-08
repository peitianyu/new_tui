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
    rt.default_style = (style_t){ .fg = 1, .bg = 1, .text = 1, .rect = 1, .border = 1, .border_fg = 4, .border_st = 1 };
    rt.info_style    = (style_t){ .fg = 2, .bg = 2, .text = 1, .rect = 1 };
    rt.scroll_style  = (style_t){ .fg = 4, .bg = 3, .text = 1, .rect = 1 };
    rt.line_no_style = (style_t){ .fg = 4, .bg = 4, .text = 1, .rect = 1 };
    rt.text = strdup("Hello 世界！\nThis is a RichText widget demo.\n支持中文、Emoji 😊 与样式。");
    rt.len = strlen(rt.text);
    rt.cap = rt.len + 1;
    rt.show_line_no = 1;
    rt.show_scroll  = 1;
    rt.show_info    = 1;
    TuiNode *richtext = richtext_new((TuiRect){5, 5, 50, 10}, &rt);
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

    richtext_free(&rt);
    canvas_free();
    term_restore();
    term_clear();
}