#include "term.h"

static struct termios g_original_tio;
void term_restore(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_original_tio);
    printf("\e[?1000l\e[?1002l\e[?1003l\e[?1015l\e[?1006l\e[?25h");
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

    printf("\e[?1000h\e[?1002h\e[?1003h\e[?1015h\e[?1006h\e[?25l");
    fflush(stdout);

    atexit(term_restore);
}

void term_clear(void) {
    printf("\e[2J\e[H");
    fflush(stdout);
}

void term_cursor_show(int show) {
    printf("%s", show ? "\e[?25h" : "\e[?25l");
    fflush(stdout);
}

int input_ready(int ms) {
    struct pollfd p = {STDIN_FILENO, POLLIN, 0};
    return poll(&p, 1, ms) > 0;
}

static event_t parse_mouse_event(const char *b, int n) {
    event_t e = {.type = EVENT_MOUSE};
    int btn, x, y; char act;
    if (sscanf(b, "\e[<%d;%d;%d%c", &btn, &x, &y, &act) == 4) {
        switch (act) {
            case 'M':
                if (btn >= 32 && btn <= 34) {
                    e.mouse.type = MOUSE_DRAG;
                    e.mouse.button = (btn == 32) ? MOUSE_LEFT : (btn == 33) ? MOUSE_MIDDLE : MOUSE_RIGHT;
                } else if (btn == 35) {
                    e.mouse.type = MOUSE_HOVER; e.mouse.button = MOUSE_NONE;
                } else if (btn == 64 || btn == 65) {
                    e.mouse.type = MOUSE_WHEEL;
                    e.mouse.scroll = (btn == 64) ? 1 : -1;
                    e.mouse.button = MOUSE_NONE;
                } else if (btn >= 0 && btn <= 2) {
                    e.mouse.type = MOUSE_PRESS;
                    e.mouse.button = (btn == 0) ? MOUSE_LEFT : (btn == 1) ? MOUSE_MIDDLE : MOUSE_RIGHT;
                } else return (event_t){EVENT_NONE};
                break;
            case 'm':
                e.mouse.type = MOUSE_RELEASE;
                e.mouse.button = (btn == 0) ? MOUSE_LEFT : (btn == 1) ? MOUSE_MIDDLE : MOUSE_RIGHT;
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

static event_t parse_keyboard_event(const char *b, int n) {
    event_t e = {.type = EVENT_KEY, .key.num = 0};

    if (n == 2 && b[0] == '\e') {
        e.key.type[0] = KEY_SPECIAL;
        e.key.key[0] = 0xF8000 | b[1];
        e.key.num = 1;
        return e;
    }

    if (n >= 3 && b[0] == '\e' && b[1] == '[') {
        e.key.type[0] = KEY_SPECIAL;
        switch (b[2]) {
            case 'A': e.key.key[0] = K_UP;    e.key.num = 1; return e;
            case 'B': e.key.key[0] = K_DOWN;  e.key.num = 1; return e;
            case 'C': e.key.key[0] = K_RIGHT; e.key.num = 1; return e;
            case 'D': e.key.key[0] = K_LEFT;  e.key.num = 1; return e;
            case 'H': e.key.key[0] = K_HOME;  e.key.num = 1; return e;
            case 'F': e.key.key[0] = K_END;   e.key.num = 1; return e;
        }
    }
    
    if (n == 4) {
        if (memcmp(b, "\e[3~", 4) == 0) {
            e.key.type[0] = KEY_SPECIAL;
            e.key.key[0] = K_DEL;
            e.key.num = 1;
            return e;
        }
        if (memcmp(b, "\e[5~", 4) == 0) {
            e.key.type[0] = KEY_SPECIAL;
            e.key.key[0] = K_PGUP;
            e.key.num = 1;
            return e;
        }
        if (memcmp(b, "\e[6~", 4) == 0) {
            e.key.type[0] = KEY_SPECIAL;
            e.key.key[0] = K_PGDN;
            e.key.num = 1;
            return e;
        }
    }

    int pos = 0;
    while (pos < n && e.key.num < MAX_KEY_LEN) {
        unsigned char c = b[pos];

        if (c <= 0x1F || c == 0x7F) {
            e.key.type[e.key.num] = KEY_SPECIAL;
            e.key.key[e.key.num++] = map_special(c);
            pos += 1;
            continue;
        }

        const char *p = b + pos;
        uint32_t cp = utf8_decode(&p);
        if (cp == 0xFFFD) {
            e.key.type[e.key.num] = KEY_NORMAL;
            e.key.key[e.key.num++] = (unsigned char)b[pos];
            pos += 1;
        } else {
            e.key.type[e.key.num] = KEY_NORMAL;
            e.key.key[e.key.num++] = cp;
            pos = p - b;
        }
    }
    return e.key.num ? e : (event_t){EVENT_NONE};
}

event_t read_event(void) {
    char buf[256];
    int n = read(STDIN_FILENO, buf, sizeof(buf));
    if (n <= 0) return (event_t){EVENT_NONE};
    if (n >= 6 && buf[0] == '\e' && buf[1] == '[' && buf[2] == '<')
        return parse_mouse_event(buf, n);

    return parse_keyboard_event(buf, n);
}