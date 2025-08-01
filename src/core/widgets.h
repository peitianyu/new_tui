#ifndef __WIDGETS_H__
#define __WIDGETS_H__

#include "render.h"
#include "tui.h"
#include "term.h"

typedef struct ButtonData ButtonData;
struct ButtonData {
    char *label;
    style_t st;
    void (*func) (ButtonData *b, void *event);
};

TuiNode *button_new(TuiRect r, const char *label, ButtonData *data);


#endif // __WIDGETS_H__