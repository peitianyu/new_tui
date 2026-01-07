#ifndef __UI_RENDERER_H__
#define __UI_RENDERER_H__ 

#include "microui.h"
#include "term.h"
#include "renderer.h"
#include "log.h"

void r_init(void);
void r_clear(mu_Color color);
void r_draw_rect(mu_Rect r, mu_Color color);
void r_draw_text(const char *text, mu_Vec2 pos, mu_Color color);
void r_draw_icon(int id, mu_Rect rect, mu_Color color);
int  r_get_text_width(const char *text, int len);
int  r_get_text_height(void);
void r_set_clip_rect(mu_Rect rect);
void r_present(void);

#endif /* __UI_RENDERER_H__ */