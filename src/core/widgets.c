#include "widgets.h"
#include <string.h>
#include <stdlib.h>

/* ---------- 面板 ---------- */
TuiNode *tui_panel(const char *title, TuiRect r, bool show_title,
                   TuiColor bg, bool border) {
    TuiNode *pn = tui_node_new(r.x, r.y, r.w, r.h);
    tui_node_set_layout(pn, TUI_LAYOUT_VERT);
    pn->style.padding = border ? 1 : 0;
    tui_node_set_color(pn, (TuiColor){255, 255, 255}, bg);
    tui_node_set_state(pn, TUI_NODE_STATE_STYLE_BORDER, border);
    tui_node_set_state(pn, TUI_NODE_STATE_FOCUSABLE, false);
    pn->data = strdup(title);
    return pn;
}

/* ---------- 标签 ---------- */
TuiNode *tui_label(const char *text, TuiRect r,
                   TuiColor fg, TuiColor bg) {
    TuiNode *lbl = tui_node_new(r.x, r.y, strlen(text), 1);
    tui_node_set_state(lbl, TUI_NODE_STATE_FOCUSABLE, false);
    lbl->data = strdup(text);
    tui_node_set_color(lbl, fg, bg);
    lbl->style.align_horz = TUI_ALIGN_LEFT;
    return lbl;
}

/* ---------- 按钮 ---------- */
TuiNode *tui_button(const char *label, TuiRect r,
                    TuiColor bg, TuiColor fg, bool border) {
    TuiNode *btn = tui_node_new(r.x, r.y, strlen(label) + 2, r.h);
    tui_node_set_state(btn, TUI_NODE_STATE_FOCUSABLE, true);
    btn->data = strdup(label);
    tui_node_set_color(btn, fg, bg);
    tui_node_set_state(btn, TUI_NODE_STATE_STYLE_BORDER, border);
    btn->style.align_horz = TUI_ALIGN_CENTER;
    btn->style.align_vert = TUI_ALIGN_MIDDLE;
    return btn;
}

/* ---------- 输入框 ---------- */
TuiNode *tui_input(const char *placeholder, TuiRect r,
                   int width, TuiColor bg, TuiColor fg, bool border) {
    TuiNode *in = tui_node_new(r.x, r.y, width + 2, 1);
    tui_node_set_state(in, TUI_NODE_STATE_FOCUSABLE, true);
    in->data = calloc(width + 1, 1);
    if (placeholder) strcpy(in->data, placeholder);
    in->constraints.max_w = width + 2;
    in->constraints.min_w = width + 2;
    tui_node_set_color(in, fg, bg);
    tui_node_set_state(in, TUI_NODE_STATE_STYLE_BORDER, border);
    in->style.align_horz = TUI_ALIGN_LEFT;
    in->style.align_vert = TUI_ALIGN_MIDDLE;
    return in;
}