#ifndef WIDGETS_H
#define WIDGETS_H

#include "tui.h"

/* 面板容器 */
TuiNode *tui_panel(const char *title, TuiRect r, bool show_title,
                   TuiColor bg, bool border);

/* 标签 */
TuiNode *tui_label(const char *text, TuiRect r,
                   TuiColor fg, TuiColor bg);

/* 按钮 */
TuiNode *tui_button(const char *label, TuiRect r,
                    TuiColor bg, TuiColor fg, bool border);

/* 文本输入框 */
TuiNode *tui_input(const char *placeholder, TuiRect r,
                   int width, TuiColor bg, TuiColor fg, bool border);

#endif