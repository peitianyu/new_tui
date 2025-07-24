#include "tui.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

/* ---------- 节点生命周期 ---------- */
TuiNode *tui_node_new(int16_t x, int16_t y, int16_t w, int16_t h) {
    TuiNode *n = calloc(1, sizeof *n);
    if (!n) { perror("calloc"); exit(EXIT_FAILURE); }
    n->bounds = (TuiRect){x, y, w, h};
    n->clip   = n->bounds;
    n->abs_x  = x;
    n->abs_y  = y;
    return n;
}

void tui_node_add(TuiNode *p, TuiNode *c) {
    if (!p || !c) return;
    if (p->kid_cnt >= p->kid_cap) {
        p->kid_cap = p->kid_cap ? p->kid_cap * 2 : 8;
        p->kids = realloc(p->kids, p->kid_cap * sizeof *p->kids);
        if (!p->kids) { perror("realloc"); exit(EXIT_FAILURE); }
    }
    p->kids[p->kid_cnt++] = c;
    c->parent = p;
}

void tui_node_remove(TuiNode *p, TuiNode *c) {
    if (!p || !c) return;
    for (int i = 0; i < p->kid_cnt; ++i) {
        if (p->kids[i] == c) {
            p->kids[i] = p->kids[--p->kid_cnt];
            tui_node_free(c);
            return;
        }
    }
}

void tui_node_free(TuiNode *n) {
    if (!n) return;
    for (int i = 0; i < n->kid_cnt; ++i) tui_node_free(n->kids[i]);
    free(n->kids);
    free(n->data);
    free(n);
}

/* ---------- 约束 ---------- */
static inline void apply_constraints(TuiNode *n) {
    if (n->constraints.min_w && n->bounds.w < n->constraints.min_w)
        n->bounds.w = n->constraints.min_w;
    if (n->constraints.max_w && n->bounds.w > n->constraints.max_w)
        n->bounds.w = n->constraints.max_w;
    if (n->constraints.min_h && n->bounds.h < n->constraints.min_h)
        n->bounds.h = n->constraints.min_h;
    if (n->constraints.max_h && n->bounds.h > n->constraints.max_h)
        n->bounds.h = n->constraints.max_h;
}

/* ---------- 布局实现 ---------- */
static void layout_vert(TuiNode *p) {
    int16_t y = 0;
    int16_t avail_w = p->bounds.w;
    int16_t fix_h = 0, exp = 0;

    for (int i = 0; i < p->kid_cnt; ++i) {
        apply_constraints(p->kids[i]);
        if (p->kids[i]->bits.expand) exp++;
        else fix_h += p->kids[i]->bounds.h;
    }
    int16_t avail_h = p->bounds.h - fix_h;
    int16_t exp_h   = exp ? avail_h / exp : 0;

    for (int i = 0; i < p->kid_cnt; ++i) {
        TuiNode *c = p->kids[i];
        c->bounds.w = avail_w; apply_constraints(c);
        if (c->bits.expand) { c->bounds.h = exp_h; apply_constraints(c); }
        c->bounds.x = 0;
        c->bounds.y = y;
        y += c->bounds.h;
    }
}

static void layout_horz(TuiNode *p) {
    int16_t x = 0;
    int16_t avail_h = p->bounds.h;
    int16_t fix_w = 0, exp = 0;

    for (int i = 0; i < p->kid_cnt; ++i) {
        apply_constraints(p->kids[i]);
        if (p->kids[i]->bits.expand) exp++;
        else fix_w += p->kids[i]->bounds.w;
    }
    int16_t avail_w = p->bounds.w - fix_w;
    int16_t exp_w   = exp ? avail_w / exp : 0;

    for (int i = 0; i < p->kid_cnt; ++i) {
        TuiNode *c = p->kids[i];
        c->bounds.h = avail_h; apply_constraints(c);
        if (c->bits.expand) { c->bounds.w = exp_w; apply_constraints(c); }
        c->bounds.x = x;
        c->bounds.y = 0;
        x += c->bounds.w;
    }
}

static void layout_flow(TuiNode *p) {
    int16_t x = 0, y = 0, line_h = 0;
    int16_t max_w = p->bounds.w;
    for (int i = 0; i < p->kid_cnt; ++i) {
        TuiNode *c = p->kids[i];
        apply_constraints(c);
        if (x + c->bounds.w > max_w && x > 0) {
            x = 0;
            y += line_h;
            line_h = 0;
        }
        c->bounds.x = x;
        c->bounds.y = y;
        x += c->bounds.w;
        if (c->bounds.h > line_h) line_h = c->bounds.h;
    }
}

static inline void apply_layout(TuiNode *n) {
    if (!n) return;
    switch (n->bits.layout) {
        case TUI_LAYOUT_VERT: layout_vert(n); break;
        case TUI_LAYOUT_HORZ: layout_horz(n); break;
        case TUI_LAYOUT_FLOW: layout_flow(n); break;
        default: break;
    }
}

/* ---------- 递归计算绝对坐标 + 裁剪 ---------- */
static void calc_recursive(TuiNode *n) {
    if (!n) return;

    if (n->parent) {
        n->abs_x = n->parent->abs_x + n->bounds.x - n->parent->scroll.x;
        n->abs_y = n->parent->abs_y + n->bounds.y - n->parent->scroll.y;
    } else {
        n->abs_x = n->bounds.x;
        n->abs_y = n->bounds.y;
    }

    TuiRect pvis = n->parent ? n->parent->clip : (TuiRect){0, 0, n->bounds.w, n->bounds.h};
    TuiRect self = {n->abs_x, n->abs_y, n->bounds.w, n->bounds.h};
    int16_t x0 = (self.x > pvis.x) ? self.x : pvis.x;
    int16_t y0 = (self.y > pvis.y) ? self.y : pvis.y;
    int16_t x1 = (self.x + self.w  < pvis.x + pvis.w ) ? self.x + self.w  : pvis.x + pvis.w;
    int16_t y1 = (self.y + self.h < pvis.y + pvis.h) ? self.y + self.h : pvis.y + pvis.h;
    n->clip = (TuiRect){x0, y0, (x1 > x0 ? x1 - x0 : 0), (y1 > y0 ? y1 - y0 : 0)};

    apply_layout(n);
    for (int i = 0; i < n->kid_cnt; ++i) calc_recursive(n->kids[i]);
}

void tui_calc(TuiNode *root) { if (root) calc_recursive(root); }

void tui_scroll(TuiNode *n, int16_t d) {
    if (!n) return;

    if(n->bits.scroll_x) {
        n->scroll.x += d;
        if(n->scroll.x < 0) n->scroll.x = 0;
        tui_calc(n);
    }else if(n->bits.scroll_y){
        n->scroll.y += d;
        if (n->scroll.y < 0) n->scroll.y = 0;
        tui_calc(n);
    }
}
/* ---------- 焦点 / 命中测试 ---------- */
static TuiNode *g_focus = NULL;

static inline bool inside(const TuiNode *n, int16_t x, int16_t y) {
    return x >= n->clip.x && x < n->clip.x + n->clip.w &&
           y >= n->clip.y && y < n->clip.y + n->clip.h;
}

static void clear_hover_recursive(TuiNode *n) {
    if (!n) return;
    n->bits.hover = 0;
    n->bits.focus = 0;
    for (int i = 0; i < n->kid_cnt; ++i)
        clear_hover_recursive(n->kids[i]);
}

static TuiNode *hit_recursive(TuiNode *n, int16_t x, int16_t y) {
    if (!n || !inside(n, x, y)) return NULL;
    for (int i = n->kid_cnt - 1; i >= 0; --i) {
        TuiNode *c = hit_recursive(n->kids[i], x, y);
        if (c && c->bits.focusable) return c;
    }
    return n->bits.focusable ? n : NULL;
}

TuiNode *tui_hit_node(TuiNode *root, int16_t mx, int16_t my, int mouse_down) {
    if (!root) return NULL;
    TuiNode *h = hit_recursive(root, mx, my);
    if(!h) return NULL;
    if(h->bits.focus == 1) return h;
    
    if (h && h->bits.focusable) {
        if(mouse_down) {clear_hover_recursive(root); h->bits.focus = 1;}
        else           h->bits.hover = 1;
    }
    return h;
}

bool tui_focus_set(TuiNode *root, TuiNode *target) {
    (void)root;
    if (!target || !target->bits.focusable) return false;
    if (g_focus) g_focus->bits.focus = 0;
    g_focus = target;
    g_focus->bits.focus = 1;
    return true;
}

TuiNode *tui_focus_get(TuiNode *root) {
    (void)root;
    return (g_focus && g_focus->bits.focusable) ? g_focus : NULL;
}

void tui_focus_next(TuiNode *root) {
    if (!root) return;
    TuiNode *cur = g_focus;
    TuiNode *next = NULL;
    bool found = (cur == NULL);

    TuiNode *stack[128];
    int top = 0;
    stack[top++] = root;
    while (top) {
        TuiNode *n = stack[--top];
        if (n->bits.focusable) {
            if (found) { next = n; break; }
            if (n == cur) found = 1;
        }
        for (int i = n->kid_cnt - 1; i >= 0; --i) stack[top++] = n->kids[i];
    }
    if (next) tui_focus_set(root, next);
    else if (cur) tui_focus_set(root, root);
}

void tui_draw(TuiNode *root, void *event) {
    if (!root) return;
    if(root->draw) root->draw(root, event);
    for (int i = 0; i < root->kid_cnt; ++i)
        tui_draw(root->kids[i], event);
}

void tui_draw_init(TuiNode *root) {
    tui_draw(root, NULL);
}

