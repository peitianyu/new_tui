/* test_label.c */
#include "core/widgets.h"
#include "core/c_test.h"
#include "core/log.h"

static inline TuiNode *create_root(void)
{
    int w, h;
    term_size(&w, &h);
    term_init();
    term_clear();
    canvas_init(w, h);
    return tui_node_new(0, 0, w, h);
}

static void update_and_draw(TuiNode *root, void *event)
{
    tui_calc(root);
    tui_draw(root, event);
    canvas_flush();
}

TEST(label, basic)
{
    setlocale(LC_ALL, "");

    TuiNode *root = create_root();

    /* -------- 1. 单行左对齐 -------- */
    LabelData left = {
        .text       = "Left label",
        .st         = { .fg = 7, .bg = 1, .text = 1, .rect = 1, .align_horz = 0, .align_vert = 1},
    };
    tui_node_add(root, label_new((TuiRect){2, 2, 15, 1}, NULL, &left));

    /* -------- 2. 单行右对齐 -------- */
    LabelData right = {
        .text       = "Right label",
        .st         = { .fg = 0, .bg = 2, .text = 1, .rect = 1, .align_horz = 2, .align_vert = 1 },
    };
    tui_node_add(root, label_new((TuiRect){20, 2, 15, 1}, NULL, &right));

    /* -------- 3. 自动换行长文本 -------- */
    LabelData wrap = {
        .text       = "This is a very long sentence which should wrap automatically when it reaches the boundary of the label rectangle. 支持中文、Emoji 😊 与样式。",
        .st         = { .fg = 3, .bg = 4, .text = 1, .rect = 1, .border = 1, .align_horz = 1, .align_vert = 1, .border_st = 1, .wrap = 1 },
    };
    tui_node_add(root, label_new((TuiRect){2, 5, 35, 7}, NULL, &wrap));

    /* -------- 主循环 -------- */
    update_and_draw(root, NULL);
    while (1) {
        event_t e = read_event();
        if (e.type == EVENT_KEY && e.key.key[0] == K_ESCAPE)
            break;

        update_and_draw(root, &e);
    }

    canvas_free();
    term_restore();
}