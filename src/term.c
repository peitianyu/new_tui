#include "term.h"
#include <stdio.h>

static HANDLE g_con_in, g_con_out;
static DWORD g_old_in_mode, g_old_out_mode;
static int g_cols = 0, g_rows = 0;
int term_init(void) {
    g_con_in = GetStdHandle(STD_INPUT_HANDLE);
    g_con_out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (g_con_in == INVALID_HANDLE_VALUE || g_con_out == INVALID_HANDLE_VALUE)
        return -1;

    GetConsoleMode(g_con_in, &g_old_in_mode);
    GetConsoleMode(g_con_out, &g_old_out_mode);

    SetConsoleOutputCP(65001); // CP_UTF8
    SetConsoleCP(65001);

    DWORD in_mode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT | 0x0080; // ENABLE_EXTENDED_FLAGS
    in_mode &= ~0x0040; // ENABLE_QUICK_EDIT_MODE
    in_mode &= ~0x0020; // ENABLE_INSERT_MODE
    
    if (!SetConsoleMode(g_con_in, in_mode)) {
        in_mode = ENABLE_WINDOW_INPUT | ENABLE_MOUSE_INPUT;
        SetConsoleMode(g_con_in, in_mode);
    }

    SetConsoleMode(g_con_out, g_old_out_mode | 0x0004); // ENABLE_VIRTUAL_TERMINAL_PROCESSING

    return 0;
}

void term_shutdown(void) {
    SetConsoleMode(g_con_in, g_old_in_mode);
    SetConsoleMode(g_con_out, g_old_out_mode);
    SetConsoleOutputCP(GetConsoleOutputCP());
}

static void update_size(void) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(g_con_out, &csbi)) {
        g_cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        g_rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    }
}

void term_get_size(int* width, int* height) { update_size(); *width = g_cols; *height = g_rows; }
void term_hide_cursor(void) { printf("\033[?25l"); fflush(stdout); }
void term_show_cursor(void) { printf("\033[u"); fflush(stdout); }
void term_move_cursor(int x, int y) { printf("\033[%d;%dH", y, x); fflush(stdout); }
void term_print_at(int x, int y, const char *text) { printf("\033[%d;%dH%s", y, x, text); fflush(stdout); }
void term_flush_input(void) { fflush(stdout); }
void term_clear_screen(void) { printf("\033[2J\033[1;1H"); fflush(stdout); }
void term_save_cursor(void) { printf("\033[s"); fflush(stdout); }
static TermMouseEvent make_mouse_event(const MOUSE_EVENT_RECORD *m) {
    TermMouseEvent e = {0};
    static DWORD last_left_down = 0;
    
    if (m->dwEventFlags == 0) {
        if (m->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
            DWORD now = GetTickCount();
            e.type = (now - last_left_down < 400) ? TERM_MOUSE_DOUBLE_CLICK : TERM_MOUSE_LEFT_DOWN;
            last_left_down = now;
        }
        else if (m->dwButtonState & RIGHTMOST_BUTTON_PRESSED) e.type = TERM_MOUSE_RIGHT_DOWN;
        else if (m->dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED) e.type = TERM_MOUSE_MIDDLE_DOWN;
        else e.type = TERM_MOUSE_LEFT_UP;
    }
    else if (m->dwEventFlags == MOUSE_MOVED) e.type = TERM_MOUSE_MOVE;
    else if (m->dwEventFlags == MOUSE_WHEELED) {
        e.type = TERM_MOUSE_WHEEL;
        e.wheel = ((SHORT)HIWORD(m->dwButtonState)) / WHEEL_DELTA;
    }
    else e.type = TERM_MOUSE_NONE;

    e.x = m->dwMousePosition.X + 1;
    e.y = m->dwMousePosition.Y + 1;
    if (m->dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) e.btn |= 1;
    if (m->dwButtonState & RIGHTMOST_BUTTON_PRESSED) e.btn |= 2;
    if (m->dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED) e.btn |= 4;
    e.ctrl = m->dwControlKeyState;
    
    return e;
}

static TermKeyEvent make_key_event(const KEY_EVENT_RECORD *k) {
    TermKeyEvent e = {0};
    e.key_code = k->wVirtualKeyCode;
    e.ctrl_code = k->dwControlKeyState;
    e.pressed = k->bKeyDown;
    e.repeat = k->wRepeatCount;
    e.scan = k->wVirtualScanCode;
    
    WCHAR wch = k->uChar.UnicodeChar;
    if (wch != 0) {
        char *out = e.utf8;
        if (wch < 0x80) *out++ = (char)wch;
        else if (wch < 0x800) {
            *out++ = (char)(0xC0 | (wch >> 6));
            *out++ = (char)(0x80 | (wch & 0x3F));
        }
        else if (wch < 0x10000) {
            *out++ = (char)(0xE0 | (wch >> 12));
            *out++ = (char)(0x80 | ((wch >> 6) & 0x3F));
            *out++ = (char)(0x80 | (wch & 0x3F));
        }
        *out = '\0';
        return e;
    }

    if (k->wVirtualKeyCode != 0) {
        BYTE kb[256] = {0};
        GetKeyboardState(kb);
        WCHAR buf[8];
        int n = ToUnicodeEx(k->wVirtualKeyCode, k->wVirtualScanCode, kb, buf, 8, 0, GetKeyboardLayout(0));
        if (n > 0) {
            char *out = e.utf8;
            for (int i = 0; i < n; ++i) {
                unsigned cp = buf[i];
                if (cp < 0x80) *out++ = (char)cp;
                else if (cp < 0x800) {
                    *out++ = (char)(0xC0 | (cp >> 6));
                    *out++ = (char)(0x80 | (cp & 0x3F));
                }
                else if (cp < 0x10000) {
                    *out++ = (char)(0xE0 | (cp >> 12));
                    *out++ = (char)(0x80 | ((cp >> 6) & 0x3F));
                    *out++ = (char)(0x80 | (cp & 0x3F));
                }
            }
            *out = '\0';
        }
    }
    
    return e;
}

TermEvent term_poll_event(void) {
    TermEvent ev = { .type = TERM_EV_NONE };
    DWORD cnt = 0;
    INPUT_RECORD rec;
    
    if (!PeekConsoleInputW(g_con_in, &rec, 1, &cnt) || cnt == 0)
        return ev;
    
    ReadConsoleInputW(g_con_in, &rec, 1, &cnt);

    switch (rec.EventType) {
    case KEY_EVENT:
        ev.type = TERM_EV_KEY;
        ev.u.key = make_key_event(&rec.Event.KeyEvent);
        break;
    case MOUSE_EVENT:
        ev.type = TERM_EV_MOUSE;
        ev.u.mouse = make_mouse_event(&rec.Event.MouseEvent);
        break;
    case WINDOW_BUFFER_SIZE_EVENT:
        ev.type = TERM_EV_RESIZE;
        update_size();
        ev.u.size.cols = g_cols;
        ev.u.size.rows = g_rows;
        break;
    }
    
    return ev;
}
