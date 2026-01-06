#include "ui_renderer.h"

static int g_first;
static renderer_t *g_last_renderer; 
static renderer_t *g_renderer; 
static mu_Rect g_clip_rect = {0};
static int g_is_clipping = 0;

static mu_Rect rect_intersect(mu_Rect a, mu_Rect b) {
    int x1 = a.x > b.x ? a.x : b.x;
    int y1 = a.y > b.y ? a.y : b.y;
    int x2 = (a.x + a.w) < (b.x + b.w) ? (a.x + a.w) : (b.x + b.w);
    int y2 = (a.y + a.h) < (b.y + b.h) ? (a.y + a.h) : (b.y + b.h);

    if (x2 <= x1 || y2 <= y1) return (mu_Rect){0, 0, 0, 0};
    return (mu_Rect){x1, y1, x2 - x1, y2 - y1};
}

static int rgb_to_mu(mu_Color color) {
    return (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b;
}

void r_init(void) {
    term_init();
    int width = 0, height = 0;
    term_get_size(&width, &height);
    g_renderer = renderer_new(width, height, (style_t){.fg=-1, .bg=-1, .raw=0});
    g_last_renderer = renderer_new(width, height, (style_t){.fg=-1, .bg=-1, .raw=0}); 
    g_first = 1;
}


void r_clear(mu_Color color) {
    int background = rgb_to_mu(color); 
    for (int i = 0; i < g_renderer->w * g_renderer->h; i++) {
        g_renderer->cells[i] = (utf8_t){" ", 1, 1};
        g_renderer->styles[i].bg = background;
    }
}

void r_draw_rect(mu_Rect r, mu_Color color) {
    r = (g_is_clipping) ? rect_intersect(r, g_clip_rect) : r;
    // printf("r: %d, %d, %d, %d, c: %d %d %d\n", r.x, r.y, r.w, r.h, color.r, color.g, color.b);
    for(int y = r.y; y < r.y + r.h; y++)
    for(int x = r.x; x < r.x + r.w; x++) {
        if(x >= g_renderer->w || y >= g_renderer->h) continue;
        g_renderer->styles[y * g_renderer->w + x].bg = rgb_to_mu(color);
    }
}

void r_draw_text(const char *text, mu_Vec2 pos, mu_Color color) {
    style_t s = g_renderer->styles[pos.y * g_renderer->w + pos.x];
    s.fg=rgb_to_mu(color);

    mu_Rect r = mu_rect(pos.x, pos.y, g_renderer->w-pos.x, g_renderer->h-pos.y);
    r = (g_is_clipping) ? rect_intersect(r, g_clip_rect) : r;
    
    renderer_set_str(g_renderer, pos.x, pos.y, text, &s, r.w);
}

void r_draw_icon(int id, mu_Rect rect, mu_Color color) {
    const char* icon;
    switch (id) {
        case MU_ICON_CLOSE:     icon = "✕";   break; 
        case MU_ICON_CHECK:     icon = "●";   break;
        case MU_ICON_COLLAPSED: icon = "▶";  break;
        case MU_ICON_EXPANDED:  icon = "▼";   break;
        default:                              break;
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
    g_is_clipping = 1;
    g_clip_rect = rect;
}

void r_present(void) {
    if (g_first) {
        term_clear_screen();
        printf("%s", renderer_to_string(g_renderer));
        g_first = 0;
    } else {
        for (int i = 0; i < g_renderer->w * g_renderer->h; i++) {
            int x = i % g_renderer->w;
            int y = i / g_renderer->w;
            if (!utf8_cmp(g_renderer->cells[i], g_last_renderer->cells[i]) && 
                !style_cmp(g_renderer->styles[i], g_last_renderer->styles[i])) {
                continue;
            }
            term_move_cursor(x, y);
            printf("%s\x1b[0m", renderer_xy_to_string(g_renderer, x, y));
        }
    }

    for(int i = 0; i < g_renderer->w * g_renderer->h; i++) {
        g_last_renderer->cells[i] = g_renderer->cells[i];
        g_last_renderer->styles[i] = g_renderer->styles[i];
    }
}