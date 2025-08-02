#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "core/render.h"
#include "core/c_test.h"

TEST(render, test) {
    canvas_init(80, 24);
    canvas_clear();
    style_t st = { .fg = 15, .bg = 4, .text = 1, .rect = 1, .border = 1, .border_st = 2 };
    canvas_draw((rect_t){5, 5, 20, 3}, "光标在下一格 ▶", st);

    st = (style_t){ .fg = 0, .bg = 3, .text = 1, .bold = 1 };
    canvas_draw((rect_t){5, 3, 20, 1}, "──────────────", st);

    canvas_flush();                 // 会输出带光标的帧
    getchar();                      // 按回车结束
    canvas_free();
    printf("\e[?25h");
}