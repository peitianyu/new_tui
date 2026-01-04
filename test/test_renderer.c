#include "../src/minitest.h"
#include "../src/renderer.h"

#include <windows.h>

TEST(test, renderer) {
    SetConsoleOutputCP(65001);

    renderer_t *r = renderer_new(19, 9, (style_t){.fg=-1, .bg=0xFF00FF, .raw=0});

    const char *s   = "Hello 世界 🌍 🚀 ";
    style_t style = {.fg=0xFF00FF, .bg=0x00FF00, .italic=1, .bold=1};
    renderer_set_str(r, 3, 1, s, &style);

    style.strike = 1;
    renderer_set_str(r, 1, 3, s, &style);

    char *s2 = renderer_to_string(r);
    printf("%s", s2);
    free(s2);
}
