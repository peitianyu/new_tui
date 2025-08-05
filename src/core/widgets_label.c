#include "widgets.h"
#include <string.h>

static void label_draw(TuiNode *lb, void *event);

TuiNode *label_new(TuiRect r, const char *text, LabelData *data)
{
    TuiNode *n = tui_node_new(r.x, r.y, r.w, r.h);
    n->bits.focusable = 0;
    n->draw           = label_draw;
    data->state       = -1;
    n->data           = data;

    if (text && !data->text) data->text = strdup(text);

    return n;
}

static void label_draw(TuiNode *lb, void *event)
{
    (void)event;
    if (!lb) return;

    LabelData *d = (LabelData *)lb->data;
    if (!d || !d->text) return;

    if (d->state == 0) return;
    d->state = 0;

    rect_t r = { lb->abs_x, lb->abs_y, lb->bounds.w, lb->bounds.h };
    if (r.w <= 0 || r.h <= 0) return;

    canvas_draw(r, d->text, d->st);
}