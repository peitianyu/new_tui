#include "core/c_test.h"
#include "core/tui.h"
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ---------- 颜色主题 ---------- */
typedef struct { uint8_t bg,fg,hilite; } Theme;
static const Theme THEME_ROOT   = {232,250,255};
static const Theme THEME_BTN    = { 24,255, 46};
static const Theme THEME_LIST   = {236,250,118};

static TuiColor pico_8_colors[16] = {
    {0, 0, 0},          // black
    {29, 43, 83},       // dark-blue
    {126, 37, 83},      // dark-purple
    {0, 135, 81},       // dark-green
    {171, 82, 54},      // brown
    {95, 87, 79},       // dark-grey
    {194, 195, 199},    // light-grey
    {255, 245, 220},    // white
    {136, 34, 21},      // red
    {231, 76, 60},      // orange
    {255, 163, 0},      // yellow
    {113, 198, 113},    // green
    {0, 104, 135},      // blue
    {206, 162, 209},    // pink
    {255, 204, 170},    // light-pink
    {179, 157, 219}     // light-purple
};

/* ---------- 节点工具函数 ---------- */
static TuiNode *make_button(const char *label)
{
    TuiNode *btn = tui_node_new(0,0,10,1);
    btn->bits.focusable    = 1;
    btn->bits.style_bg     = 1;
    btn->bits.style_fg     = 1;
    btn->bits.style_border = 1;

    btn->style.bg  = pico_8_colors[10];
    btn->style.fg  = pico_8_colors[7];
    btn->style.align_horz = TUI_ALIGN_CENTER;
    btn->style.align_vert = TUI_ALIGN_MIDDLE;
    btn->data = strdup(label);
    return btn;
}

static TuiNode *make_list_item(const char *text)
{
    TuiNode *it = tui_node_new(0,0,0,1);
    it->bits.expand    = 1;
    it->bits.style_bg  = 1;
    it->bits.style_fg  = 1;
    it->constraints.min_h = 1;
    it->style.bg = pico_8_colors[11];
    it->style.fg = pico_8_colors[7];
    it->style.align_horz = TUI_ALIGN_LEFT;
    it->data = strdup(text);
    return it;
}

/* ---------- 简单的 500 ms 高亮计时 ---------- */
static struct {
    TuiNode *btn;
    time_t   until;
} g_flash = {0};

static void check_flash(void)
{
    if (g_flash.btn && time(NULL) >= g_flash.until) {
        free(g_flash.btn->data);
        g_flash.btn->data = strdup("Btn");
        g_flash.btn->style.bg = pico_8_colors[10];
        g_flash.btn = NULL;
    }
}

static bool btn_clicked(TuiNode *b, TuiEvent *e)
{
    (void)e;
    g_flash.btn = b;
    g_flash.until = time(NULL) + 1;   /* 1 s ≈ 500~1000 ms 足够肉眼可见 */

    free(b->data);
    b->data = strdup(" ☑ ");
    b->style.bg = pico_8_colors[11];  /* 高亮颜色 */
    return true;
}

/* ---------- 状态栏 ---------- */
static void draw_status(TuiNode *root, TuiNode *scroll) {
    char buf[128];
    TuiNode *focus = tui_focus_get(root);
    snprintf(buf,sizeof(buf)," Focus:%s | Scroll:%d ", focus && focus->id ? focus->id : "none", scroll->scroll.y);
    printf("\033[1;1H\033[2K\033[48;5;%dm\033[38;5;%dm%s\033[0m",
           THEME_ROOT.bg, THEME_ROOT.fg, buf);
    fflush(stdout);
}

TEST(layout, test)
{
    setlocale(LC_ALL,"");
    term_init();
    int scr_w, scr_h; term_size(&scr_w,&scr_h);
    draw_scene_focus_init(scr_w,scr_h);

    /* ---------- 根 ---------- */
    TuiNode *root = tui_node_new(0,1,scr_w,scr_h-2);
    root->style.bg = pico_8_colors[9];
    root->bits.style_bg = 1;
    tui_node_set_layout(root, TUI_LAYOUT_VERT);

    /* ---------- 上区：水平按钮 & 流式按钮左右并排 ---------- */
    TuiNode *upper = tui_node_new(0,0,0,0);
    upper->bits.expand = 1;
    upper->style.spacing = 1;   /* 左右两列之间的空隙 */
    upper->style.padding = 1;
    tui_node_set_layout(upper, TUI_LAYOUT_HORZ);   /* ← 关键：横向排列子节点 */
    tui_node_add(root, upper);

    /* 左列：垂直排列的 2 个按钮 */
    TuiNode *leftCol = tui_node_new(0,0,0,0);
    leftCol->bits.expand = 1;
    leftCol->style.spacing = 1;
    leftCol->style.padding = 1;
    tui_node_set_layout(leftCol, TUI_LAYOUT_VERT);
    tui_node_add(upper, leftCol);

    for (int i = 0; i < 2; ++i) {
        char name[16];
        snprintf(name, sizeof(name), "Btn %d", i + 1);
        TuiNode *btn = make_button(name);
        btn->handler = btn_clicked;
        tui_node_add(leftCol, btn);
    }

    /* 右列：垂直排列的 2 个按钮（用 FLOW 布局，效果同流式） */
    TuiNode *rightCol = tui_node_new(0,0,0,0);
    rightCol->bits.expand = 1;
    rightCol->style.spacing = 1;
    rightCol->style.padding = 1;
    tui_node_set_layout(rightCol, TUI_LAYOUT_FLOW); 
    tui_node_add(upper, rightCol);

    for (int i = 2; i < 52; ++i) {
        char name[16];
        snprintf(name, sizeof(name), "Btn %d", i + 1);
        TuiNode *btn = make_button(name);
        btn->handler = btn_clicked;
        tui_node_add(rightCol, btn);
    }

    /* ---------- 下区：滚动列表 ---------- */
    TuiNode *scroll = tui_node_new(0,0,0,8);
    scroll->bits.expand   = 1;
    scroll->bits.scroll_y = 1;
    scroll->style.padding = 1;
    tui_node_set_layout(scroll, TUI_LAYOUT_VERT);
    tui_node_add(root, scroll);

    for(int i=0;i<20;i++){
        char text[16]; snprintf(text,sizeof(text),"Line %02d",i);
        tui_node_add(scroll, make_list_item(text));
    }

    /* ---------- 初始焦点 ---------- */
    tui_focus_set(root, tui_focus_get(root));

    /* ---------- 事件循环 ---------- */
    bool run = true;
    while(run){
        check_flash();                /* 每帧检查高亮是否到期 */
        draw_status(root, scroll);
        draw_scene_focus(root);

        TuiEvent ev = read_event();
        switch(ev.type){
        case EVENT_KEY:
            for(int i=0;i<ev.key.num;i++){
                int k = ev.key.key[i];
                if(k==K_ESCAPE || k==CTRL_KEY('c')) run=false;
                else if(k==K_TAB) tui_focus_next(root);
                else if(k==K_UP   || k=='k') tui_scroll(scroll, -2);
                else if(k==K_DOWN || k=='j') tui_scroll(scroll,  2);
            }
            break;
        case EVENT_MOUSE:
            if(ev.mouse.type==MOUSE_PRESS){
                TuiNode *hit = tui_hit_test(root, ev.mouse.x-1, ev.mouse.y-1);
                if(hit) tui_focus_set(root, hit);
                if(hit && hit->handler) hit->handler(hit,&ev);
            }else if(ev.mouse.type==MOUSE_WHEEL){
                tui_scroll(scroll, ev.mouse.scroll*2);
            }
            break;
        default: break;
        }
    }

    tui_node_free(root);
    draw_scene_focus_cleanup();
    term_restore();
}