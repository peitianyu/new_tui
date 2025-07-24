#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "core/render.h"
#include "core/c_test.h"

TEST(render, test) {
    canvas_init(80, 24);
    printf("\x1b[?25h");
    style_t st = { .fg = 15, .bg = 4, .text = 1, .cursor = 1, .rect = 1 }; // block 光标
    canvas_draw((rect_t){5, 5, 20, 3}, "光标在下一格 ▶", st);

    canvas_flush();                 // 会输出带光标的帧
    canvas_cursor_move(15, 5);      // 把真实光标挪到文字后面
    getchar();                      // 按回车结束
    canvas_free();
}