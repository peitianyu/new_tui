#ifndef __TUI_H__
#define __TUI_H__

#include <stdbool.h>
#include <stdint.h>
#include "term.h"

/* ---------------- 基础几何 ---------------- */
typedef struct { int16_t x, y, w, h; } TuiRect;
typedef struct { int16_t x, y;        } TuiPoint;
typedef struct { int16_t x, y;        } TuiScrollOffset;

/* ---------------- 标志位 ---------------- */
typedef union {
    struct {
        uint32_t disabled   : 1;
        uint32_t hidden     : 1;
        uint32_t dirty      : 1;
        uint32_t focusable  : 1;
        uint32_t expand     : 1; 
        uint32_t scroll_x   : 1;
        uint32_t scroll_y   : 1;

        uint32_t hover      : 1;
        uint32_t focus      : 1;
        uint32_t active     : 1;

        uint32_t layout     : 2;   /* 0:none 1:vert 2:horz 3:flow */

        uint32_t style_border  : 1;
        uint32_t style_bg      : 1;
        uint32_t style_fg      : 1;
        uint32_t style_align   : 1;
        uint32_t style_padding : 1;
    };
    uint32_t u32;
} TuiBits;

#define TUI_NODE_STATE_DISABLED  (1 << 0)
#define TUI_NODE_STATE_HIDDEN    (1 << 1)
#define TUI_NODE_STATE_DIRTY     (1 << 2)
#define TUI_NODE_STATE_FOCUSABLE (1 << 3)
#define TUI_NODE_STATE_EXPAND    (1 << 4)
#define TUI_NODE_STATE_SCROLL_X  (1 << 5)
#define TUI_NODE_STATE_SCROLL_Y  (1 << 6)
#define TUI_NODE_STATE_HOVER     (1 << 7)
#define TUI_NODE_STATE_FOCUS     (1 << 8)
#define TUI_NODE_STATE_ACTIVE    (1 << 9)
#define TUI_NODE_STATE_LAYOUT    (1 << 10) /* 0:none 1:vert 2:horz 3:flow */
#define TUI_NODE_STATE_STYLE_BORDER  (1 << 11)
#define TUI_NODE_STATE_STYLE_BG      (1 << 12)
#define TUI_NODE_STATE_STYLE_FG      (1 << 13)
#define TUI_NODE_STATE_STYLE_ALIGN   (1 << 14)
#define TUI_NODE_STATE_STYLE_PADDING (1 << 15)

/* ---------------- 枚举 ---------------- */
typedef enum {
    TUI_LAYOUT_NONE = 0,
    TUI_LAYOUT_VERT = 1,
    TUI_LAYOUT_HORZ = 2,
    TUI_LAYOUT_FLOW = 3
} TuiLayout;

typedef enum {
    TUI_ALIGN_LEFT   = 0,
    TUI_ALIGN_CENTER = 1,
    TUI_ALIGN_RIGHT  = 2
} TuiAlignHorz;

typedef enum {
    TUI_ALIGN_TOP    = 0,
    TUI_ALIGN_MIDDLE = 1,
    TUI_ALIGN_BOTTOM = 2
} TuiAlignVert;

/* ---------------- 样式 ---------------- */
typedef struct { uint8_t r, g, b; } TuiColor;

typedef struct {
    TuiColor fg, bg;
    uint8_t  align_horz : 2;
    uint8_t  align_vert : 2;
    uint8_t  padding;
    uint8_t  spacing;
} TuiStyle;

/* ---------------- 约束 ---------------- */
typedef struct {
    int16_t min_w, max_w, min_h, max_h;
    bool    expand;
} TuiConstraints;

/* ---------------- 事件 ---------------- */
typedef event_t TuiEvent;

/* ---------------- 节点 ---------------- */
typedef struct TuiNode TuiNode;
struct TuiNode {
    TuiRect  bounds;
    TuiRect  clip;
    int16_t  abs_x, abs_y;
    TuiScrollOffset scroll;

    TuiStyle      style;
    TuiConstraints constraints;
    TuiBits       bits;

    TuiNode      *parent;
    TuiNode     **kids;
    uint16_t      kid_cnt;
    uint16_t      kid_cap;

    const char   *id;
    void         *data;

    bool (*handler)(TuiNode *, TuiEvent *);
};

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
static inline void tui_node_set_color(TuiNode *n, TuiColor fg, TuiColor bg) {
    n->style.fg = fg; n->style.bg = bg;
    n->bits.style_fg = n->bits.style_bg = 1;
}

/* ---- 生命周期 ---- */
TuiNode *tui_node_new(int16_t x, int16_t y, int16_t w, int16_t h);
void     tui_node_add(TuiNode *parent, TuiNode *child);
void     tui_node_remove(TuiNode *parent, TuiNode *child);
void     tui_node_free(TuiNode *root);

/* ---- 布局 ---- */
void tui_calc(TuiNode *root);
void tui_scroll(TuiNode *n, int16_t d);

/* ---- 焦点系统 ---- */
TuiNode *tui_hit_test(TuiNode *root, int16_t mx, int16_t my);
bool     tui_focus_set(TuiNode *root, TuiNode *target);
TuiNode *tui_focus_get(TuiNode *root);
void     tui_focus_next(TuiNode *root);

/* ---- 渲染 ---- */
typedef struct {
    int w, h;
    unsigned char *buf;
    char *out;
    int out_cap, out_len;
} Canvas;

/* ---- 基础画布 ---- */
Canvas *cv_new(int w, int h);
void    cv_free(Canvas *c);
void    cv_flush(Canvas *c);

void tui_draw_node(Canvas *c, TuiNode *n);
void draw_scene_focus_init(int w, int h);
void draw_scene_focus(TuiNode *root);
void draw_scene_focus_cleanup(void);

#endif