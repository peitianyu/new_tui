#ifndef __WIDGETS_H__
#define __WIDGETS_H__

#include "render.h"
#include "tui.h"
#include "term.h"
#include "utf8.h"
#include "log.h"
#include <stddef.h>  

typedef struct LabelData LabelData;
struct LabelData {
    char     *text;     /* 当前内容 */
    style_t  st;        /* 样式 */
    int8_t   state;     /* 聚焦状态 */
};
TuiNode *label_new(TuiRect r, const char *text, LabelData *data);


typedef struct ButtonData ButtonData;
struct ButtonData {
    char     *text;     /* 当前内容 */
    style_t  st;        /* 样式 */
    int8_t   state;     /* 聚焦状态 */
    void     (*hover_func) (ButtonData *b);
    void     (*focus_func) (ButtonData *b, void *event);
};
TuiNode *button_new(TuiRect r, const char *text, ButtonData *data);


typedef struct InputBoxData InputBoxData;
struct InputBoxData {
    char        *text;         /* 当前内容（动态分配/修改） */
    size_t      len;           /* 已用长度 */
    size_t      cap;           /* 缓冲区容量 */
    size_t      cursor;        /* 光标字符位置（0~len） */
    style_t     st;            /* 样式 */
    style_t     st_focus;      /* 聚焦时的样式 */
    size_t      scroll_x;      /* 水平滚动偏移量（字符宽度单位） */
    int8_t      state;         /* 聚焦状态 */
};
TuiNode *inputbox_new(TuiRect r, InputBoxData *data);


typedef struct { size_t off, len; int lineno;} RichLine;
typedef struct {
    char   *text;
    size_t  len, cap;

    size_t cursor;
    size_t cursor_line;   
    size_t cursor_col;    
    int scroll_x, scroll_y;

    RichLine *lines;
    size_t    line_cnt, line_cap;

    struct { size_t start, end; style_t style; } *styles;
    size_t style_cnt, style_cap;

    struct {
        uint8_t show_line_no : 1;
        uint8_t show_scroll  : 1;
        uint8_t show_info    : 1;
    };

    style_t default_style;
    style_t info_style;
    style_t line_no_style;
    style_t scroll_style;

    uint8_t dirty_flags;

    int8_t state;

    // 选中文本相关
    size_t select_start;   // 选中起始位置（字节偏移）
    size_t select_end;     // 选中结束位置（字节偏移）
    int8_t is_selecting;   // 是否正在选中状态
    style_t select_style;  // 选中文本的样式
    
    // 剪贴板相关
    char *clipboard;       // 内部剪贴板缓冲区
    size_t clipboard_len;  // 剪贴板内容长度
    size_t clipboard_cap;  // 剪贴板容量
} RichTextData;

TuiNode *richtext_new(TuiRect r, RichTextData *data);
void     richtext_free(RichTextData *d);

#endif // __WIDGETS_H__