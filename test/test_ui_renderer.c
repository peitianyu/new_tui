#include "../src/minitest.h"
#include "../src/ui_renderer.h"

#include <windows.h>

TEST(test, ui_renderer) {
    SetConsoleOutputCP(65001);

    r_init();

    r_clear(mu_color(0, 0, 255, 0));
    
    r_draw_text("中文", mu_vec2(2, 10), mu_color(0, 255, 255, 255));

    r_draw_icon(1, mu_rect(10, 3, 1, 3), mu_color(255, 0, 255, 0));
    r_draw_icon(2, mu_rect(5, 3, 1, 3), mu_color(255, 0, 255, 0));
    r_draw_icon(3, mu_rect(5, 4, 1, 3), mu_color(255, 0, 255, 0));
    r_draw_icon(4, mu_rect(5, 6, 1, 3), mu_color(255, 0, 255, 0));
    
    r_set_clip_rect(mu_rect(20, 10, 4, 1));

    r_draw_rect(mu_rect(20, 10, 4, 2), mu_color(0, 255, 0, 0));
    
    r_set_clip_rect(mu_rect(2, 11, 5, 1));
    
    r_draw_text("你好中国", mu_vec2(2, 11), mu_color(0, 255, 255, 0));

    r_present();

    term_shutdown();
}
