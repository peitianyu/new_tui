#include "../src/minitest.h"
#include "../src/renderer.h"

#include <windows.h>

TEST(test, renderer) {
    SetConsoleOutputCP(65001);

    renderer_t *r = renderer_new(10, 5, (style_t){.fg=-1, .bg=0x1, .raw=0});

    const char *s   = "ｘ中文";
    style_t style = {.fg=0x2, .bg=-1, .strike=1};
    renderer_set_str(r, 1, 1, s, &style);

    renderer_print(r);

    // style.strike = 1;
    // renderer_set_str(r, 1, 3, s, &style);

    char *s2 = renderer_to_string(r);
    printf("%s", s2);
    free(s2);
}
