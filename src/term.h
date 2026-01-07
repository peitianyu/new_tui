#ifndef __TERM_H__
#define __TERM_H__

#include <windows.h>

typedef enum {
    TERM_MOUSE_NONE = 0, TERM_MOUSE_MOVE, TERM_MOUSE_LEFT_DOWN, TERM_MOUSE_LEFT_UP, TERM_MOUSE_RIGHT_DOWN,
    TERM_MOUSE_RIGHT_UP, TERM_MOUSE_MIDDLE_DOWN, TERM_MOUSE_MIDDLE_UP, TERM_MOUSE_WHEEL, TERM_MOUSE_DOUBLE_CLICK
} TermMouseEventType;

typedef struct {
    int key_code;   
    union {
        int ctrl_code;
        struct {
            int right_alt_pressed   : 1;
            int left_alt_pressed    : 1;
            int right_ctrl_pressed  : 1;
            int left_ctrl_pressed   : 1;
            int shift_pressed       : 1;
            int num_lock_on         : 1;
            int scroll_lock_on      : 1;
            int caps_lock_on        : 1;
            int enhanced_key        : 1;
        };
    }; 
    char utf8[5];   // 0结尾
    int pressed;      
    int repeat;       
    unsigned int scan;
} TermKeyEvent;

typedef struct {
    TermMouseEventType type;
    int x, y; 
    int btn;        // 1|2|4
    int ctrl; 
    int wheel; 
} TermMouseEvent;

typedef enum { TERM_EV_NONE = 0, TERM_EV_KEY, TERM_EV_MOUSE, TERM_EV_RESIZE } TermEventType;
typedef struct {
    TermEventType type;
    union {
        TermKeyEvent  key;
        TermMouseEvent mouse;
        struct { int cols, rows; } size;
    } u;
} TermEvent;

int  term_init(void);       
void term_shutdown(void);      
void term_get_size(int* width, int* height);
TermEvent term_poll_event(void); // 非堵塞

// 光标操作
void term_hide_cursor(void);
void term_clear_screen(void);
void term_move_cursor(int x, int y);
void term_print_at(int x, int y, const char *text);
void term_flush_input(void);
void term_save_cursor(void);
void term_show_cursor(void);


#endif /* __TERM_H__ */