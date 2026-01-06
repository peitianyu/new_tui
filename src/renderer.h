#ifndef __RENDERER_H__
#define __RENDERER_H__

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utf8.h"
#include "style.h"

typedef struct {
    utf8_t  *cells;     /* 每格一个 utf8_t 字符 */
    style_t *styles;    /* 每格一个样式 */
    int      w, h;      /* 逻辑列数、行数 */
} renderer_t;

#define UTF8_STR_MAX (1024) 
static inline renderer_t *renderer_new(int width, int height, style_t s) {
    renderer_t *r = (renderer_t *)malloc(sizeof(*r));
    if (!r) return NULL;
    r->w = width;
    r->h = height;
    size_t n = (size_t)width * height;
    r->cells  = (utf8_t  *)calloc(n, sizeof(utf8_t));
    r->styles = (style_t *)calloc(n, sizeof(style_t));
    if (!r->cells || !r->styles) {
        free(r->cells);
        free(r->styles);
        free(r);
        return NULL;
    }

    for(int i = 0; i < n; i++) r->cells[i] = (utf8_t){" ", 1, 1};
    for(int i = 0; i < n; i++) r->styles[i] = s; 
    return r;
}

static inline void renderer_free(renderer_t *r) {
    if (!r) return;
    free(r->cells);
    free(r->styles);
    free(r);
}

static utf8_t utf8_empty = {0};
static inline void renderer_set(renderer_t *r, int x, int y, const utf8_t *u, const style_t *s) {
    if (!r || x < 0 || y < 0 || y >= r->h || x >= r->w) return;
    if (x + u->width > r->w) return;

    int idx = y * r->w + x;
    r->cells[idx]  = *u;
    r->styles[idx] = *s;
    
    if (u->width == 1) return;
    for (int i = 1; i < u->width; i++) 
        renderer_set(r, x + i, y, &utf8_empty, s);
}


static inline void renderer_set_str(renderer_t *r, int x, int y, const char *str, const style_t *s, int utf8_width) {
    utf8_t utf8_buf[UTF8_STR_MAX];
    int n = str_to_utf8(str, utf8_buf, UTF8_STR_MAX);
    int width = 0;
    for(int i = 0; i < n; i++) {
        if(width >= utf8_width) break;
        renderer_set(r, width+x, y, &utf8_buf[i], s);
        width += utf8_buf[i].width;
    }
}

static inline char* renderer_to_string(renderer_t *r) {
    if (!r) return NULL;
    char *buf = (char *)malloc(r->w * r->h * 5 + 1);
    if (!buf) return NULL;
    char *p = buf;
    for (int y = 0; y < r->h; y++) {
        style_t s = {.fg=-1, .bg=-1, .raw=0};
        for (int x = 0; x < r->w; x++) {
            utf8_t *u = &r->cells[y * r->w + x];
            if (u->len == 0) { continue;}
            char* style_str = style_to_string(r->styles[y * r->w + x]);
            if(style_cmp(s, r->styles[y * r->w + x])) {
                memcpy(p, "\x1b[0m", 4);
                p += 4;
                int style_str_len = strlen(style_str);
                memcpy(p, style_str, style_str_len);
                p += style_str_len;
                s = r->styles[y * r->w + x];
            }
            memcpy(p, u->bytes, u->len);
            p += u->len;
        }
        memcpy(p, "\x1b[0m", 4);
        p += 4;
        *p++ = '\n';
    }
    *p = '\0';
    return buf;
}

static inline char* renderer_xy_to_string(renderer_t *r, int x, int y) {
    char buf[1024];
    char *p = buf;
    char* style_str = style_to_string(r->styles[y * r->w + x]);
    memcpy(p, style_str, strlen(style_str));
    p += strlen(style_str);

    memcpy(p, r->cells[y * r->w + x].bytes, r->cells[y * r->w + x].len);
    p += r->cells[y * r->w + x].len;
    *p = '\0';
    return strdup(buf);
}

static inline void renderer_print(renderer_t *r) {
    for(int y = 0; y < r->h; y++) {
        for(int x = 0; x < r->w; x++) {
            utf8_t u = r->cells[y * r->w + x];
            style_t s = r->styles[y * r->w + x];
            // printf("[%2s %d %d]", u.bytes, u.len, u.width);
            printf("[%2s %2d %2d %2d] ", u.bytes, s.fg, s.bg, s.raw);
            // printf("[%2s %s] ", u.bytes, style_to_string(s));
        }
        printf("\n");
    }
}

#endif /* __RENDERER_H__ */