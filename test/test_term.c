#include "../src/minitest.h"
#include "../src/term.h"

#include <windows.h>

TEST(test, term) {
    SetConsoleOutputCP(65001);

    if (term_init() < 0) {
        fprintf(stderr, "init fail\n");
        return ;
    }

    term_clear_screen();
    int width, height;
    term_get_size(&width, &height);
    printf("终端大小 %dx%d\n", width, height);
    puts("键盘/鼠标/滚轮/窗口大小变化测试，按 ESC 退出");
    puts("=============================================");
    
    while (1) {
        TermEvent e = term_poll_event();
        if (e.type == TERM_EV_NONE) {
            Sleep(10);
            continue;
        }

        // 显示事件信息在底部
        term_move_cursor(1, 5);
        
        switch (e.type) {
        case TERM_EV_KEY:
            if (e.u.key.pressed && e.u.key.key_code == VK_ESCAPE) {
                term_move_cursor(1, 5);
                puts("\nESC 按下，退出。");
                goto QUIT;
            }
            if(e.u.key.pressed) {
                printf("KEY %s vk=%d scan=%u utf8=%s repeat=%d",
                       e.u.key.pressed ? "DN" : "UP", e.u.key.key_code, e.u.key.scan,
                       e.u.key.utf8[0] ? e.u.key.utf8 : "ø", e.u.key.repeat);
            }
            break;

        case TERM_EV_MOUSE:
            printf("MOUSE %d x=%d y=%d bt=%d ctrl=0x%x wheel=%d",
                   e.u.mouse.type, e.u.mouse.x, e.u.mouse.y,
                   e.u.mouse.btn, e.u.mouse.ctrl, e.u.mouse.wheel);
            break;

        case TERM_EV_RESIZE:
            printf("RESIZE %dx%d", e.u.size.cols, e.u.size.rows);
            break;

        default:
            break;
        }
        
        fflush(stdout);
    }

QUIT:
    term_shutdown();
    term_clear_screen();
}
