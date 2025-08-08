#ifndef RENDER_STYLE_H
#define RENDER_STYLE_H

#include <stdint.h>

// 仅支持16色
#define SEQ_LEN(s)  (sizeof(s) - 1)
static const char * const RENDER_FG[16] = {
    "", "\e[34m", "\e[35m", "\e[32m", "\e[31m", "\e[90m", "\e[37m", "\e[97m",
    "\e[91m", "\e[33m", "\e[93m", "\e[92m", "\e[36m", "\e[95m", "\e[96m", "\e[94m"
};
static const char * const RENDER_BG[16] = {
    "", "\e[44m", "\e[45m", "\e[42m", "\e[41m", "\e[100m", "\e[47m", "\e[107m",
    "\e[101m", "\e[43m", "\e[103m", "\e[102m", "\e[106m", "\e[105m", "\e[106m", "\e[104m"
};
static const uint8_t RENDER_FG_LEN[16] = {
    0, SEQ_LEN("\e[34m"), SEQ_LEN("\e[35m"), SEQ_LEN("\e[32m"),
    SEQ_LEN("\e[31m"), SEQ_LEN("\e[90m"), SEQ_LEN("\e[37m"), SEQ_LEN("\e[97m"),
    SEQ_LEN("\e[91m"), SEQ_LEN("\e[33m"), SEQ_LEN("\e[93m"), SEQ_LEN("\e[92m"),
    SEQ_LEN("\e[36m"), SEQ_LEN("\e[95m"), SEQ_LEN("\e[96m"), SEQ_LEN("\e[94m")
};
static const uint8_t RENDER_BG_LEN[16] = {
    0, SEQ_LEN("\e[44m"), SEQ_LEN("\e[45m"), SEQ_LEN("\e[42m"),
    SEQ_LEN("\e[41m"), SEQ_LEN("\e[100m"), SEQ_LEN("\e[47m"), SEQ_LEN("\e[107m"),
    SEQ_LEN("\e[101m"), SEQ_LEN("\e[43m"), SEQ_LEN("\e[103m"), SEQ_LEN("\e[102m"),
    SEQ_LEN("\e[106m"), SEQ_LEN("\e[105m"), SEQ_LEN("\e[106m"), SEQ_LEN("\e[104m")
};
#undef SEQ_LEN

#define FC_DEFAULT         0
#define FC_BLUE            1
#define FC_MAGENTA         2
#define FC_GREEN           3
#define FC_RED             4
#define FC_DARKGRAY        5
#define FC_LIGHTGRAY       6
#define FC_WHITE           7
#define FC_BRIGHT_RED      8
#define FC_YELLOW          9
#define FC_BRIGHT_YELLOW   10
#define FC_BRIGHT_GREEN    11
#define FC_CYAN            12
#define FC_BRIGHT_MAGENTA  13
#define FC_BRIGHT_CYAN     14
#define FC_NONE            15

#define BC_DEFAULT          0
#define BC_BLUE             1
#define BC_MAGENTA          2
#define BC_GREEN            3
#define BC_RED              4
#define BC_DARKGRAY         5
#define BC_LIGHTGRAY        6
#define BC_WHITE            7
#define BC_BRIGHT_RED       8
#define BC_YELLOW           9
#define BC_BRIGHT_YELLOW    10
#define BC_BRIGHT_GREEN     11
#define BC_CYAN             12
#define BC_BRIGHT_MAGENTA   13
#define BC_BRIGHT_CYAN      14
#define BC_BRIGHT_BLUE      15

// 仅支持4种风格
typedef enum {
    BORDER_LIGHT=0, /* ─ │ ┌ ┐ └ ┘ */
    BORDER_HEAVY,   /* ═ ║ ╔ ╗ ╚ ╝ */
    BORDER_DASHED,  /* ┄ ┆ ┌ ┐ └ ┘ */
    BORDER_ROUND,   /* ─ │ ╭ ╮ ╰ ╯ */
} border_style_t;

typedef struct {
    uint32_t horz, vert, tl, tr, bl, br;
} border_chars_t;

static const border_chars_t BORDER_TBL[] = {
    [BORDER_LIGHT]  = {0x2500, 0x2502, 0x250C, 0x2510, 0x2514, 0x2518},
    [BORDER_HEAVY]  = {0x2550, 0x2551, 0x2554, 0x2557, 0x255A, 0x255D},
    [BORDER_DASHED] = {0x2504, 0x2506, 0x250C, 0x2510, 0x2514, 0x2518},
    [BORDER_ROUND]  = {0x2500, 0x2502, 0x256D, 0x256E, 0x2570, 0x256F},
};

#endif /* RENDER_STYLE_H */