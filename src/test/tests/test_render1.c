#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include "core/render.h"
#include "core/term.h"
#include "core/c_test.h"


static int g_term_size_changed = 0;
void term_sigwinch_received() {
    g_term_size_changed = 1;
}

static void draw_ui0(int w, int h) {
    style_t st1 = { .fg = 7, .bg = 3, .rect = 1, .border = 1, .text = 1, .align_horz = 1, .align_vert = 1 };
    canvas_draw((rect_t){10, 2, 30, 4}, "Hello 世界!☑ ☐ ▶ ◀ \nCentered", st1);

    style_t st2 = { .fg = 8, .bg = 2, .rect = 1, .text = 1, .focus = 0, .align_horz = 0, .align_vert = 1 };
    canvas_draw((rect_t){5, 7, 15, 3}, "This text is too long", st2);
}

static void draw_ui1(int w, int h) {
    style_t st1 = { .fg = 0, .bg = 6, .rect = 1, .border = 1, .text = 1, .align_horz = 2, .align_vert = 2 };
    canvas_draw((rect_t){5, 5, 35, 5}, "Style 1: Right/Bottom", st1);
}

static void draw_ui2(int w, int h) {
    style_t st1 = { .fg = 2, .bg = 4, .rect = 1, .border = 1, .text = 1, .align_horz = 0, .align_vert = 0 };
    canvas_draw((rect_t){15, 8, 20, 3}, "Style 2: Left/Top", st1);
}

static int g_style_idx = 0;
typedef void (*draw_fn)(int,int);
static draw_fn g_styles[] = { draw_ui0, draw_ui1, draw_ui2 };
static const int g_style_cnt = sizeof(g_styles)/sizeof(g_styles[0]);

TEST(render1, test) {
    signal(SIGWINCH, term_sigwinch_received);

    int w, h;
    term_size(&w, &h);
    printf("\x1b[?25l");
    canvas_init(w, h);
    while (1) {
        if (g_term_size_changed) {
            term_size(&w, &h);
            canvas_free();
            canvas_init(w, h);
        }

        g_styles[g_style_idx](w, h);
        if(g_term_size_changed) {g_term_size_changed = 0; canvas_flush_all();}
        canvas_flush();

        usleep(10000);
        g_style_idx = (g_style_idx + 1) % g_style_cnt;
    }

    canvas_free();
}