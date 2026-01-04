#include "ui_renderer.h"

static renderer_t *g_renderer; 
static int g_width, g_height;
static mu_Rect g_clip_rect = {0};
static int g_is_clipping = 0;

static int rgb_to_mu(int r, int g, int b) {
    return (r << 16) | (g << 8) | b;
}

void r_init(void) {
    term_init();
    term_get_size(&g_width, &g_height);
    g_renderer = renderer_new(g_width, g_height, (style_t){.fg=-1, .bg=-1, .raw=0});
}

void r_clear(mu_Color color) {
    for (int i = 0; i < g_renderer->w * g_renderer->h; i++)
        g_renderer->styles[i].bg = rgb_to_mu(color.r, color.g, color.b);
}

void r_draw_rect(mu_Rect r, mu_Color color) {
    for(int y = r.y; y < r.y + r.h; y++)
    for(int x = r.x; x < r.x + r.w; x++) {
        g_renderer->styles[y * g_renderer->w + x].bg = (rgb_to_mu(color.r, color.g, color.b));
    }
}

void r_draw_text(const char *text, mu_Vec2 pos, mu_Color color) {
    style_t s = (style_t){.fg=rgb_to_mu(color.r, color.g, color.b), .bg=-1, .raw=0};
    renderer_set_str(g_renderer, pos.x, pos.y, text, &s);
}

void r_draw_icon(int id, mu_Rect rect, mu_Color color) {
    const char* icon;
    switch (id) {
    case MU_ICON_CLOSE:     icon = "ｘ";  break;
    case MU_ICON_CHECK:     icon = "☑ "; break;
    case MU_ICON_COLLAPSED: icon = "⯈ "; break;
    case MU_ICON_EXPANDED:  icon = "⯆ "; break;
    default:                             break;
    }
    
    r_draw_text(icon, mu_vec2(rect.x, rect.y), color);
}

int  r_get_text_width(const char *text, int len) {
    utf8_t utf8_buf[UTF8_STR_MAX];
    int n = str_to_utf8(text, utf8_buf, UTF8_STR_MAX);
    int width = 0;
    for(int i = 0; i < n; i++) width += utf8_buf[i].width;
    return width;
}

int  r_get_text_height(void) {
    return 1;
}

void r_set_clip_rect(mu_Rect rect) {

}

void r_present(void) {
    term_clear_screen();
    char *s = renderer_to_string(g_renderer);
    printf("%s", s);
    // free(s);
}