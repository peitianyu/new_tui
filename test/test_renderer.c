#include "../src/minitest.h"
#include "../src/renderer.h"

#include <windows.h>

// FIXME: 类似🌍 🚀在之后必须带有' '不然会挤到一起, 得具体查一下原因
TEST(test, renderer) {
    SetConsoleOutputCP(65001);

    renderer_t *r = renderer_new(19, 9, (style_t){.fg=-1, .bg=-1, .raw=0});

    const char *s   = "Hello 世界🌍🚀";
    style_t style = {.fg=0xFF00FF, .bg=0x00FF00, .bold=1};
    renderer_set_str(r, 3, 1, s, &style, 15);

    style.strike = 1;
    renderer_set_str(r, 1, 3, s, &style, 15);

    char *s2 = renderer_to_string(r);
    printf("%s\n", s2);
    free(s2);
}
