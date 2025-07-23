#include "core/tui.h"
#include "core/term.h"
#include "core/c_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool btn_clicked(TuiNode *btn, TuiEvent *ev) {
    (void)ev;
    free(btn->data);
    btn->data = strdup("Pressed");
    return true;
}

static TuiNode *build_ui(void) {
    int w, h;
    term_size(&w, &h);

    /* ---------- 根窗口（整屏） ---------- */
    TuiNode *root = tui_node_new(0, 0, w, h);
    root->style.bg = (TuiColor){235, 235, 235};   /* 浅灰背景 */
    root->bits.style_bg = 1;
    tui_node_set_layout(root, TUI_LAYOUT_VERT);

    /* ---------- 标题栏 ---------- */
    TuiNode *title = tui_node_new(0, 0, 0, 2);
    title->bits.expand = 0;                    /* 固定高度 */
    title->style.bg    = (TuiColor){ 40,  40,  40};
    title->style.fg    = (TuiColor){255, 255, 255};
    title->bits.style_bg = title->bits.style_fg = 1;
    title->data = strdup("Demo Window");
    title->style.align_horz = TUI_ALIGN_CENTER;
    title->style.align_vert = TUI_ALIGN_MIDDLE;
    tui_node_add(root, title);

    /* ---------- 按钮容器 ---------- */
    TuiNode *hbox = tui_node_new(0, 0, 0, 0);
    hbox->bits.expand = 1;                     /* 占满剩余空间 */
    hbox->style.padding = 2;
    hbox->style.spacing = 3;
    tui_node_set_layout(hbox, TUI_LAYOUT_HORZ);
    tui_node_add(root, hbox);

    /* ---------- 三个按钮 ---------- */
    const char *labels[] = {"Btn A", "Btn B", "Btn C"};
    for (int i = 0; i < 3; ++i) {
        TuiNode *btn = tui_node_new(0, 0, 0, 3);
        btn->bits.focusable  = 1;
        btn->bits.expand     = 1;              /* 自动拉伸宽度 */
        btn->bits.style_border = btn->bits.style_bg = btn->bits.style_fg = 1;
        btn->style.bg = (TuiColor){70, 70, 70};
        btn->style.fg = (TuiColor){255, 255, 255};
        btn->style.align_horz = TUI_ALIGN_CENTER;
        btn->style.align_vert = TUI_ALIGN_MIDDLE;
        btn->data = strdup(labels[i]);
        btn->handler = btn_clicked;
        tui_node_add(hbox, btn);
    }

    tui_calc(root);
    tui_focus_set(root, tui_focus_get(root));   /* 初始焦点 */
    return root;
}

TEST(tui_resize, test)
{
    setlocale(LC_ALL, "");
    term_init();
    draw_scene_focus_init(80, 24);

    TuiNode *ui = build_ui();

    bool run = true;
    while (run) {
        /* 如果窗口尺寸变化，重新构建 UI */
        if (term_sigwinch_received()) {
            tui_node_free(ui);
            ui = build_ui();
        }

        draw_scene_focus(ui);

        TuiEvent ev = read_event();
        switch (ev.type) {
        case EVENT_KEY:
            for (int i = 0; i < ev.key.num; ++i) {
                int k = ev.key.key[i];
                if (k == K_ESCAPE || k == CTRL_KEY('c'))
                    run = false;
                else if (k == K_TAB)
                    tui_focus_next(ui);
                else if (k == K_ENTER) {
                    TuiNode *f = tui_focus_get(ui);
                    if (f && f->handler) f->handler(f, &ev);
                }
            }
            break;
        case EVENT_MOUSE:
            if (ev.mouse.type == MOUSE_PRESS) {
                TuiNode *hit = tui_hit_test(ui, ev.mouse.x - 1, ev.mouse.y - 1);
                if (hit) tui_focus_set(ui, hit);
                if (hit && hit->handler) hit->handler(hit, &ev);
            }
            break;
        default: break;
        }
    }

    tui_node_free(ui);
    draw_scene_focus_cleanup();
    term_restore();
}