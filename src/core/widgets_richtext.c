#include "widgets.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static void richtext_draw(TuiNode *n, void *event);

/* -------------------------------------------------------------------------- */
TuiNode *richtext_new(TuiRect r, RichTextData *d)
{
    TuiNode *rt = tui_node_new(r.x, r.y, r.w, r.h);
    rt->bits.focusable = 1;          /* 可选：允许聚焦 */
    rt->draw           = richtext_draw;
    rt->data           = d;

    /* 初始化缓冲区 */
    if (!d->cap) d->cap = 256;
    if (!d->text) {
        d->text = malloc(d->cap);
        d->text[0] = '\0';
        d->len = 0;
    }
    d->state = -1;                   /* 强制第一次绘制 */
    return rt;
}

/* -------------------------------------------------------------------------- */
static int style_at(RichTextData *d, size_t pos, style_t *out)
{
    *out = d->default_style;
    for (size_t i = 0; i < d->style_cnt; ++i) {
        RichStyle *s = &d->styles[i];
        if (pos >= s->start && pos < s->end) {
            *out = s->style;
            return 1;
        }
    }
    return 0;
}

/* -------------------------------------------------------------------------- */
static void richtext_draw(TuiNode *n, void *event)
{
    RichTextData *d = (RichTextData *)n->data;

    /* 只有内容/光标/滚动变化才重绘 */
    if (d->state == 0) return;
    d->state = 0;

    style_t st = d->default_style;
    const int bw = st.border ? 1 : 0;
    const int vis_w = n->bounds.w - 2 * bw;
    const int vis_h = n->bounds.h - 2 * bw;
    if (vis_w <= 0 || vis_h <= 0) return;

    /* -------------- 1. 拆成行 -------------- */
    typedef struct { size_t off; int len, vis; } Line;
    Line lines[vis_h];
    int lc = 0;

    const char *p = d->text;
    size_t off = 0;
    while (*p && lc < vis_h) {
        const char *line_start = p;
        size_t line_off = off;
        int vis = 0;

        /* 找到一行（硬换行或自动换行） */
        while (*p) {
            if (*p == '\n') { ++p; ++off; break; }

            const char *old = p;
            uint32_t cp = utf8_decode(&p);
            int w = utf8_width(cp);
            if (vis + w > vis_w) break;   /* 自动换行 */

            vis += w;
            ++off;
        }

        lines[lc++] = (Line){ line_off, (int)(p - line_start), vis };
    }

    /* -------------- 2. 垂直滚动 -------------- */
    int y0 = n->abs_y + bw;
    if (d->scroll_y > lc - vis_h && lc > vis_h)
        d->scroll_y = lc - vis_h;
    int first_line = (int)d->scroll_y;
    if (first_line < 0) first_line = 0;

    /* -------------- 3. 逐行绘制 -------------- */
    char line_buf[vis_w * 4 + 1];
    for (int ln = 0; ln < vis_h && (first_line + ln) < lc; ++ln) {
        Line *l = &lines[first_line + ln];

        /* 水平滚动：字符宽度单位 -> 字节偏移 */
        int skip_vis = (int)d->scroll_x;
        const char *src = d->text + l->off;
        size_t skip_bytes = utf8_advance(src, 0, skip_vis);
        src += skip_bytes;

        /* 截取可见宽度 */
        size_t take_bytes = utf8_trunc_width(src, vis_w);
        memcpy(line_buf, src, take_bytes);
        line_buf[take_bytes] = '\0';

        /* 绘制整行（目前整行同一样式，如需逐字符样式可再拆分） */
        rect_t r_line = { n->abs_x + bw, y0 + ln, vis_w, 1 };
        canvas_draw(r_line, line_buf, d->default_style);
    }
}