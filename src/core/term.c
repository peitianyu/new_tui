#include "term.h"

static struct termios g_original_tio;
void term_restore(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_original_tio);
    printf("\x1b[?1000l\x1b[?1002l\x1b[?1003l\x1b[?1015l\x1b[?1006l\x1b[?25h");
    fflush(stdout);
}

void term_size(int *width, int *height) {
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    *width = ws.ws_col;
    *height = ws.ws_row;
}

void term_init(void) {
    signal(SIGINT, SIG_IGN);  
    signal(SIGTSTP, SIG_IGN); 

    tcgetattr(STDIN_FILENO, &g_original_tio);
    struct termios raw = g_original_tio;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_iflag &= ~(IXON | IXOFF | IXANY);

    raw.c_cc[VINTR] = _POSIX_VDISABLE;  
    raw.c_cc[VSUSP] = _POSIX_VDISABLE;  

    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    printf("\x1b[?1000h\x1b[?1002h\x1b[?1003h\x1b[?1015h\x1b[?1006h");
    printf("\x1b[?25l");
    fflush(stdout);

    atexit(term_restore);
}

void term_clear(void) {
    printf("\x1b[2J\x1b[H");
    fflush(stdout);
}

void term_cursor_show(int show) {
    printf("%s", show ? "\033[?25h" : "\033[?25l");
    fflush(stdout);
}

int input_ready(int ms) {
    struct pollfd p = {STDIN_FILENO, POLLIN, 0};
    return poll(&p, 1, ms) > 0;
}

static event_t parse_mouse_event(const char *b, int n) {
    event_t e = {.type = EVENT_MOUSE};
    int btn, x, y; char act;
    if (sscanf(b, "\x1b[<%d;%d;%d%c", &btn, &x, &y, &act) == 4) {
        switch (act) {
            case 'M':
                if (btn >= 32 && btn <= 34) {
                    e.mouse.type = MOUSE_DRAG;
                    e.mouse.button = (btn == 32) ? MOUSE_LEFT :
                                     (btn == 33) ? MOUSE_MIDDLE : MOUSE_RIGHT;
                } else if (btn == 35) {
                    e.mouse.type = MOUSE_HOVER; e.mouse.button = MOUSE_NONE;
                } else if (btn == 64 || btn == 65) {
                    e.mouse.type = MOUSE_WHEEL;
                    e.mouse.scroll = (btn == 64) ? 1 : -1;
                    e.mouse.button = MOUSE_NONE;
                } else if (btn >= 0 && btn <= 2) {
                    e.mouse.type = MOUSE_PRESS;
                    e.mouse.button = (btn == 0) ? MOUSE_LEFT :
                                     (btn == 1) ? MOUSE_MIDDLE : MOUSE_RIGHT;
                } else return (event_t){EVENT_NONE};
                break;
            case 'm':
                e.mouse.type = MOUSE_RELEASE;
                e.mouse.button = (btn == 0) ? MOUSE_LEFT :
                                 (btn == 1) ? MOUSE_MIDDLE : MOUSE_RIGHT;
                break;
            default: return (event_t){EVENT_NONE};
        }
        e.mouse.x = x; e.mouse.y = y;
        return e;
    }
    return (event_t){EVENT_NONE};
}

static inline int map_special(int raw) {
    switch (raw) {
        case 0x0A: return K_ENTER;
        case 0x09: return K_TAB;
        case 0x1B: return K_ESCAPE;
        case 0x7F: return K_BACKSPACE;
        default:   return raw;
    }
}

static int is_utf8_start(unsigned char c) {
    return (c & 0x80) == 0 || (c & 0xE0) == 0xC0 ||
           (c & 0xF0) == 0xE0 || (c & 0xF8) == 0xF0;
}
static int utf8_len(unsigned char c) {
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

static event_t parse_keyboard_event(const char *b, int n) {
    event_t e = {.type = EVENT_KEY, .key.num = 0};

    if (n >= 3 && b[0] == '\x1b' && b[1] == '[') {
        e.key.type[0] = KEY_SPECIAL;
        switch (b[2]) {
            case 'A': e.key.key[0] = K_UP;    e.key.num = 1; return e;
            case 'B': e.key.key[0] = K_DOWN;  e.key.num = 1; return e;
            case 'C': e.key.key[0] = K_RIGHT; e.key.num = 1; return e;
            case 'D': e.key.key[0] = K_LEFT;  e.key.num = 1; return e;
        }
    }
    if (n == 4 && memcmp(b, "\x1b[3~", 4) == 0) {
        e.key.type[0] = KEY_SPECIAL;
        e.key.key[0] = K_DEL;
        e.key.num = 1;
        return e;
    }

    int pos = 0;
    while (pos < n && e.key.num < 32) {
        unsigned char c = b[pos];

        if (c <= 0x1F || c == 0x7F) {
            e.key.type[e.key.num] = KEY_SPECIAL;
            e.key.key[e.key.num++] = map_special(c);
            pos += 1;
            continue;
        }

        if (is_utf8_start(c)) {
            int len = utf8_len(c);
            if (pos + len > n) break;
            int key = 0;
            for (int i = 0; i < len; ++i)
                key = (key << 8) | (unsigned char)b[pos + i];
            e.key.type[e.key.num] = KEY_NORMAL;
            e.key.key[e.key.num++] = key;
            pos += len;
        } else {
            e.key.type[e.key.num] = KEY_NORMAL;
            e.key.key[e.key.num++] = c;
            pos += 1;
        }
    }
    return e.key.num ? e : (event_t){EVENT_NONE};
}

event_t read_event(void) {
    char buf[256];
    int n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n <= 0) return (event_t){EVENT_NONE};

    if (n >= 6 && buf[0] == '\x1b' && buf[1] == '[' && buf[2] == '<')
        return parse_mouse_event(buf, n);

    return parse_keyboard_event(buf, n);
}