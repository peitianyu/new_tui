#ifndef __WIDGETS_H__
#define __WIDGETS_H__

#include "render.h"
#include "tui.h"
#include "term.h"
#include "log.h"

static inline TuiNode *creat_root_node(void) {
    int w, h; term_size(&w, &h);
    term_init();
    term_clear();
    canvas_init(w, h);

    return tui_node_new(0, 0, w, h);
}

typedef struct ButtonData ButtonData;
struct ButtonData {
    char *label;
    style_t st;
    void (*hover_func) (ButtonData *b);
    void (*focus_func) (ButtonData *b, void *event);
};

TuiNode *button_new(TuiRect r, const char *label, ButtonData *data);

#endif // __WIDGETS_H__