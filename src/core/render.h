#ifndef __RENDER_H__
#define __RENDER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "utf8.h"

typedef struct { int x, y, w, h; } rect_t;

typedef union {
    struct {
        uint32_t fg        : 4;
        uint32_t bg        : 4;
        uint32_t text      : 1;
        uint32_t rect      : 1;
        uint32_t border    : 1;
        uint32_t hover     : 1;
        uint32_t focus     : 1; 
        uint32_t align_horz: 2;
        uint32_t align_vert: 2;
        uint32_t italic    : 1;
        uint32_t underline : 1;
        uint32_t bold      : 1;
        uint32_t strike    : 1;
        uint32_t border_fg : 4;
        uint32_t border_st : 2;
    };
    uint32_t v;
} style_t;

typedef struct {
    int x, y;
    uint8_t st;
    bool cursor_able;
}cursor_t;

typedef struct {
    uint32_t *buf;   
    style_t  *sty;
    cursor_t cursor;
    int w, h;
} canvas_t;

void canvas_init(int w, int h);
void canvas_free(void);
void canvas_clear(void);
void canvas_draw(rect_t r_orig, const char *utf8, style_t st); 
void canvas_flush_all(void);
void canvas_flush(void);
void canvas_cursor_move(int x, int y, int style);
void canvas_cursor_clear(void);

#endif // __RENDER_H__