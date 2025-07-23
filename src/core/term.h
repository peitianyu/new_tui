#ifndef __TERM_H__
#define __TERM_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <poll.h>
#include <locale.h>
#include <wchar.h>

/* ---------------- 键盘 ---------------- */
#define K_ENTER     0xF0001
#define K_TAB       0xF0002
#define K_ESCAPE    0xF0003
#define K_BACKSPACE 0xF0004
#define K_UP        0xF0011
#define K_DOWN      0xF0012
#define K_LEFT      0xF0013
#define K_RIGHT     0xF0014
#define K_DEL       0xF0015
#define CTRL_KEY(k) ((k) & 0x1f)

typedef enum { KEY_SPECIAL, KEY_NORMAL } key_type_t;
typedef struct {
    int         num;
    key_type_t  type[32];
    int         key[32];
} key_event_t;

/* ---------------- 鼠标 ---------------- */
typedef enum { MOUSE_NONE, MOUSE_LEFT, MOUSE_MIDDLE, MOUSE_RIGHT } mouse_button_t;
typedef enum { MOUSE_PRESS, MOUSE_RELEASE, MOUSE_MOVE, MOUSE_DRAG, MOUSE_WHEEL, MOUSE_HOVER } mouse_event_type_t;
typedef struct {
    mouse_button_t      button;
    mouse_event_type_t  type;
    int                 x, y, scroll;
} mouse_data_t;

typedef enum { EVENT_NONE, EVENT_KEY, EVENT_MOUSE } event_type_t;
typedef struct {
    event_type_t type;
    union {
        key_event_t key;
        mouse_data_t mouse;
    };
} event_t;

void term_init(void);
void term_restore(void);
void term_clear(void);
void term_size(int *width, int *height);
int  term_sigwinch_received(void);

int input_ready(int ms);
event_t read_event(void);


#endif // __TERM_H__