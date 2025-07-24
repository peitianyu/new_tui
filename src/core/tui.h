#ifndef __TUI_H__
#define __TUI_H__

#include <stdint.h>
#include <stdbool.h>

/* ---------- 基础几何 ---------- */
typedef struct { int16_t x, y, w, h; } TuiRect;
typedef struct { int16_t x, y; } TuiPoint;
typedef struct { int16_t x, y; } TuiScrollOffset;

/* ---------- 状态位 ---------- */
typedef union {
    struct {
        uint16_t disabled   : 1;
        uint16_t hidden     : 1;
        uint16_t focusable  : 1;
        uint16_t hover      : 1;
        uint16_t focus      : 1;  
        uint16_t expand     : 1;
        uint16_t scroll_x   : 1;
        uint16_t scroll_y   : 1;
        uint16_t layout     : 2;   /* 0:none 1:vert 2:horz 3:flow */
    };
    uint16_t u32;
} TuiBits;

typedef enum {
    TUI_LAYOUT_NONE = 0,
    TUI_LAYOUT_VERT = 1,
    TUI_LAYOUT_HORZ = 2,
    TUI_LAYOUT_FLOW = 3
} TuiLayout;

/* ---------- 约束 ---------- */
typedef struct {
    int16_t min_w, max_w, min_h, max_h;
    bool    expand;
} TuiConstraints;

/* ---------- 节点 ---------- */
typedef struct TuiNode TuiNode;
struct TuiNode {
    TuiRect  bounds;     /* 相对父节点的矩形      */
    TuiRect  clip;       /* 绝对坐标下的可见矩形  */
    int16_t  abs_x, abs_y;

    TuiScrollOffset scroll;

    TuiConstraints constraints;
    TuiBits        bits;

    TuiNode  *parent;
    TuiNode **kids;
    uint16_t  kid_cnt;
    uint16_t  kid_cap;

    const char *id;
    void       *data;   
    void       (*draw)  (TuiNode *n, void *event);
};

/* ---------- 内联工具 ---------- */
static inline bool tui_node_has_state(const TuiNode *n, uint32_t mask) {
    return (n->bits.u32 & mask) != 0;
}
static inline void tui_node_set_state(TuiNode *n, uint32_t mask, bool on) {
    if (on) n->bits.u32 |=  mask;
    else    n->bits.u32 &= ~mask;
}
static inline void tui_node_set_layout(TuiNode *n, TuiLayout l) {
    n->bits.layout = l;
}

/* ---------- 生命周期 ---------- */
TuiNode *tui_node_new(int16_t x, int16_t y, int16_t w, int16_t h);
void     tui_node_add   (TuiNode *parent, TuiNode *child);
void     tui_node_remove(TuiNode *parent, TuiNode *child);
void     tui_node_free  (TuiNode *root);

/* ---------- 布局 ---------- */
void tui_calc(TuiNode *root);
void tui_scroll(TuiNode *n, int16_t d);

/* ---------- 焦点 / 命中 ---------- */
TuiNode *tui_hit_node(TuiNode *root, int16_t mx, int16_t my, int mouse_down);
bool     tui_focus_set(TuiNode *root, TuiNode *target);
TuiNode *tui_focus_get(TuiNode *root);
void     tui_focus_next(TuiNode *root);

/* ---------- 绘制 ---------- */
void tui_draw_init(TuiNode *root);
void tui_draw(TuiNode *root, void *event);


#endif /* __TUI_H__ */