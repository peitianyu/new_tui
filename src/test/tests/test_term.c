#include "core/c_test.h"
#include "core/term.h"

#define debug_print(...) printf(__VA_ARGS__)

/* ---------- 打印事件 ---------- */
static const char *keyname(int k) {
    switch (k) {
        case K_ENTER:     return "Enter";
        case K_TAB:       return "Tab";
        case K_ESCAPE:    return "Esc";
        case K_BACKSPACE: return "Backspace";
        case K_UP:        return "↑";
        case K_DOWN:      return "↓";
        case K_LEFT:      return "←";
        case K_RIGHT:     return "→";
        case K_DEL:       return "Del";
        default:          return NULL;
    }
}
static void print_event(const event_t *e) {
    if (e->type == EVENT_KEY) {
        debug_print("[KEY] %d: ", e->key.num);
        for (int i = 0; i < e->key.num; ++i) {
            int k = e->key.key[i];
            key_type_t t = e->key.type[i];

            if (t == KEY_SPECIAL) {
                const char *name = keyname(k);
                if (name) {
                    debug_print("【%s】 ", name);
                } else if (k > 0 && k < 32) {
                    debug_print("【Ctrl+%c】 ", 'A' + k - 1);
                } else {
                    debug_print("【0x%02X】 ", k);
                }
            } else { /* KEY_NORMAL */
                unsigned char bytes[5] = {0};
                int idx = 0;
                if (k & 0xFF000000) bytes[idx++] = (k >> 24) & 0xFF;
                if (k & 0x00FF0000) bytes[idx++] = (k >> 16) & 0xFF;
                if (k & 0x0000FF00) bytes[idx++] = (k >>  8) & 0xFF;
                if (k & 0x000000FF) bytes[idx++] =  k        & 0xFF;
                wchar_t wbuf[2] = {0};
                if (mbstowcs(wbuf, (char *)bytes, 2) != (size_t)-1)
                    debug_print("'%ls' ", wbuf);
                else
                    debug_print("'<?>' ");
            }
        }
        debug_print("\n");
    } else if (e->type == EVENT_MOUSE) {
        switch (e->mouse.type) {
            case MOUSE_PRESS:
                debug_print("[MOUSE] Press %s (%d,%d)\n",
                    e->mouse.button == MOUSE_LEFT ? "Left" :
                    e->mouse.button == MOUSE_MIDDLE ? "Middle" : "Right",
                    e->mouse.x, e->mouse.y);
                break;
            case MOUSE_RELEASE:
                debug_print("[MOUSE] Release %s (%d,%d)\n",
                    e->mouse.button == MOUSE_LEFT ? "Left" :
                    e->mouse.button == MOUSE_MIDDLE ? "Middle" : "Right",
                    e->mouse.x, e->mouse.y);
                break;
            case MOUSE_MOVE:
                debug_print("[MOUSE] Move (%d,%d)\n", e->mouse.x, e->mouse.y);
                break;
            case MOUSE_DRAG:
                debug_print("[MOUSE] Drag %s (%d,%d)\n",
                    e->mouse.button == MOUSE_LEFT ? "Left" :
                    e->mouse.button == MOUSE_MIDDLE ? "Middle" : "Right",
                    e->mouse.x, e->mouse.y);
                break;
            case MOUSE_WHEEL:
                debug_print("[MOUSE] Wheel %s (%d,%d)\n",
                    e->mouse.scroll > 0 ? "Up" : "Down",
                    e->mouse.x, e->mouse.y);
                break;
            case MOUSE_HOVER:
                debug_print("[MOUSE] Hover (%d,%d)\n", e->mouse.x, e->mouse.y);
                break;
        }
    }
}


static struct termios g_original_tio;
static void terminal_restore(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_original_tio);
    printf("\x1b[?1000l\x1b[?1002l\x1b[?1003l\x1b[?1015l\x1b[?1006l");
    printf("\x1b[?25h");
    fflush(stdout);
}

TEST(term, test) 
{
    setlocale(LC_ALL, "");
    term_init();

    debug_print("Multi-UTF-8 + Chinese input test\nPress q or Ctrl+Q to quit\n\n");

    while (1) {
        if (!input_ready(100)) continue;
        event_t e = read_event();
        if (e.type == EVENT_NONE) continue;
        print_event(&e);

        /* 退出：检测到 'q' 或 Ctrl+Q */
        if (e.type == EVENT_KEY) {
            for (int i = 0; i < e.key.num; ++i) {
                int k = e.key.key[i];
                if (k == 'q' || k == CTRL_KEY('Q')) {
                    terminal_restore();
                    return ;
                }
            }
        }
    }
}