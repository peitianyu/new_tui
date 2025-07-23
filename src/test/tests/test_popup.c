#include "core/c_test.h"
#include "core/tui.h"
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---------- 颜色主题 ---------- */
typedef struct { uint8_t bg, fg, hilite; } Theme;
static const Theme THEME_ROOT = {232, 250, 255};
static const Theme THEME_POPUP = {235, 255, 255}; // 弹窗主题

static TuiColor pico_8_colors[16] = {
    {0, 0, 0},          // black
    {29, 43, 83},       // dark-blue
    {126, 37, 83},      // dark-purple
    {0, 135, 81},       // dark-green
    {171, 82, 54},      // brown
    {95, 87, 79},       // dark-grey
    {194, 195, 199},    // light-grey
    {255, 245, 220},    // white
    {136, 34, 21},      // red
    {231, 76, 60},      // orange
    {255, 163, 0},      // yellow
    {113, 198, 113},    // green
    {0, 104, 135},      // blue
    {206, 162, 209},    // pink
    {255, 204, 170},    // light-pink
    {179, 157, 219}     // light-purple
};

/* ---------- 节点工具函数 ---------- */
static TuiNode* make_button(const char* label) {
    TuiNode* btn = tui_node_new(1, 1, 10, 5);
    btn->bits.focusable = 1;
    btn->bits.style_bg = 1;
    btn->bits.style_fg = 1;
    btn->bits.style_border = 1;

    btn->style.bg = pico_8_colors[10];
    btn->style.fg = pico_8_colors[7];
    btn->style.align_horz = TUI_ALIGN_CENTER;
    btn->style.align_vert = TUI_ALIGN_MIDDLE;
    btn->data = strdup(label);
    return btn;
}

/* ---------- 弹窗相关 ---------- */
static TuiNode* popup = NULL;

static bool btn_toggle_popup(TuiNode* b, TuiEvent* e) {
    (void)e;
    if (!popup) {
        // 创建弹窗
        int scr_w, scr_h;
        term_size(&scr_w, &scr_h);
        popup = tui_node_new(0, 0, 20, 10);
        popup->style.bg = pico_8_colors[THEME_POPUP.bg];
        popup->style.fg = pico_8_colors[THEME_POPUP.fg];
        popup->bits.style_bg = 1;
        popup->bits.style_fg = 1;
        popup->bits.style_border = 1;
        popup->style.align_horz = TUI_ALIGN_CENTER;
        popup->style.align_vert = TUI_ALIGN_MIDDLE;
        popup->data = strdup("This is a popup");
        tui_node_add(b->parent, popup);
    } else {
        tui_node_remove(b->parent, popup);
        popup = NULL;
    }
    return true;
}

static void hide_popup() {
    if (popup) {
        tui_node_remove(popup->parent, popup);
        popup = NULL;
    }
}

TEST(layout, popup_test) {
    setlocale(LC_ALL, "");
    term_init();
    int scr_w, scr_h;
    term_size(&scr_w, &scr_h);
    draw_scene_focus_init(scr_w, scr_h);

    /* ---------- 根节点 ---------- */
    TuiNode* root = tui_node_new(0, 0, scr_w, scr_h);
    root->style.bg = pico_8_colors[9];
    root->bits.focusable = 0;
    root->bits.style_bg = 1;
    tui_node_set_layout(root, TUI_LAYOUT_NONE);

    /* ---------- 按钮 ---------- */
    TuiNode* btn_toggle = make_button("Toggle Popup");
    btn_toggle->handler = btn_toggle_popup;
    tui_node_add(root, btn_toggle);

    /* ---------- 事件循环 ---------- */
    bool run = true;
    while (run) {
        draw_scene_focus(root);

        TuiEvent ev = read_event();
        switch (ev.type) {
            case EVENT_KEY:
                for (int i = 0; i < ev.key.num; i++) {
                    int k = ev.key.key[i];
                    if (k == K_ESCAPE || k == CTRL_KEY('c')) run = false;
                }
                break;
            case EVENT_MOUSE:
                if (ev.mouse.type == MOUSE_PRESS) {
                    TuiNode* hit = tui_hit_test(root, ev.mouse.x - 1, ev.mouse.y - 1);
                    if (hit && hit->handler) {
                        hit->handler(hit, &ev); 
                    } else {
                        hide_popup(); 
                    }
                }
                break;
            default:
                break;
        }
    }

    tui_node_free(root);
    draw_scene_focus_cleanup();
    term_restore();
}