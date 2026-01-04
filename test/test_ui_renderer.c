#include "../src/minitest.h"
#include "../src/ui_renderer.h"

#include <windows.h>

TEST(test, ui_renderer) {
    SetConsoleOutputCP(65001);

    r_init();

    r_clear(mu_color(255, 255, 0, 255));

    r_draw_text("中文", mu_vec2(2, 10), mu_color(0, 255, 255, 255));

    r_draw_icon(1, mu_rect(0, 0, 1, 2), mu_color(255, 0, 255, 255));
    r_draw_icon(2, mu_rect(0, 3, 1, 2), mu_color(255, 0, 255, 255));
    r_draw_icon(3, mu_rect(0, 4, 1, 2), mu_color(255, 0, 255, 255));
    r_draw_icon(4, mu_rect(0, 6, 1, 2), mu_color(255, 0, 255, 255));

    r_draw_rect(mu_rect(20, 10, 4, 2), mu_color(0, 255, 255, 255));

    r_present();

    term_shutdown();
}
