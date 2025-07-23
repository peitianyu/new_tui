#include "core/c_test.h"
#include "core/tui.h"
#include "core/widgets.h"
#include "core/term.h"
#include <locale.h>

#define BG(c)           (TuiColor){c, c, c}
#define PANEL(name,x,y,w,h,title) \
        tui_panel(name,(TuiRect){x,y,w,h},title,BG(50),true)
#define BTN(name) \
        tui_button(name,(TuiRect){0,0,16,3},BG(70),BG(255),true)
#define INP(name,len) \
        tui_input(name,(TuiRect){0,0,28,1},len,BG(60),BG(254),true)
#define LBL(name) \
        tui_label(name,(TuiRect){0,0,28,1},BG(250),BG(50))

static bool clicked(TuiNode *b, TuiEvent *e){ (void)e; free(b->data); b->data=strdup("[Pressed]"); return 1; }

TEST(tui, test)
{
    setlocale(LC_ALL,"");
    term_init(); int w,h; term_size(&w,&h); draw_scene_focus_init(w,h);

    TuiNode *root = tui_node_new(0,0,w,h);
    root->style.bg = BG(45);
    tui_node_set_layout(root, TUI_LAYOUT_VERT);

    /* 自上而下：菜单 / 内容 / 状态 */
    tui_node_add(root, PANEL("Menu",0,0,w,1,true));
    TuiNode *content = PANEL("",0,1,w,h-2,false);
    tui_node_set_layout(content, TUI_LAYOUT_HORZ);
    tui_node_add(root, content);
    tui_node_add(root, PANEL("Status: Ready",0,h-2,w,1,true));

    /* 三栏 */
    TuiNode *left = PANEL("Buttons",0,0,20,h-2,true);
    tui_node_set_layout(left, TUI_LAYOUT_VERT); left->style.spacing=1;
    TuiNode *mid  = PANEL("Input",0,0,30,h-2,true);
    tui_node_set_layout(mid, TUI_LAYOUT_VERT);  mid->style.spacing=1;
    TuiNode *right= PANEL("Info",0,0,w-50,h-2,true);
    tui_node_set_layout(right,TUI_LAYOUT_VERT); right->style.spacing=1;

    tui_node_add(content, left);
    tui_node_add(content, mid);
    tui_node_add(content, right);

    /* 子控件 */
    TuiNode *b1=BTN("Click Me"), *b2=BTN("Tab Focus");
    b1->handler=b2->handler=clicked;
    tui_node_add(left,b1); tui_node_add(left,b2);

    TuiNode *inp1=INP("Type here",20), *inp2=INP("Second",20);
    tui_node_add(mid,LBL("↓ use Tab / Arrow / Enter / Mouse"));
    tui_node_add(mid,inp1); tui_node_add(mid,inp2);

    tui_node_add(right,LBL("Hover/Focus demo"));
    tui_node_add(right,LBL("Scroll: ↑↓"));

    tui_focus_set(root,inp1);

    bool run=1;
    while(run){
        draw_scene_focus(root);
        TuiEvent ev=read_event();
        switch(ev.type){
        case EVENT_KEY:
            for(int i=0;i<ev.key.num;++i){
                int k=ev.key.key[i];
                if(k==K_ESCAPE||k==CTRL_KEY('c')) run=0;
                else if(k==K_TAB) tui_focus_next(root);
                else if(k==K_UP||k==K_DOWN) tui_scroll(right,(k==K_UP)?-2:2);
                else if(k==K_ENTER){
                    if(tui_node_has_state(b1,TUI_NODE_STATE_FOCUS)) clicked(b1,NULL);
                    if(tui_node_has_state(b2,TUI_NODE_STATE_FOCUS)) clicked(b2,NULL);
                } else if(ev.key.type[i]==KEY_NORMAL){
                    TuiNode *in = tui_node_has_state(inp1,TUI_NODE_STATE_FOCUS)?inp1:
                                 (tui_node_has_state(inp2,TUI_NODE_STATE_FOCUS)?inp2:NULL);
                    if(in){
                        char *s=in->data; size_t l=strlen(s); if(l<20){
                            char buf[64]; snprintf(buf,sizeof(buf),"%s%c",s,k);
                            free(s); in->data=strdup(buf);
                        }
                    }
                }
            }
            break;
        case EVENT_MOUSE:{
            if(ev.mouse.type==MOUSE_PRESS){
                TuiNode *hit=tui_hit_test(root,ev.mouse.x-1,ev.mouse.y-1);
                if(hit) tui_focus_set(root,hit);
                if(hit==b1||hit==b2) clicked(hit,NULL);
            } else if(ev.mouse.type==MOUSE_WHEEL){
                tui_scroll(right,ev.mouse.scroll*2);
            }
            break;}
        default: break;
        }
    }

    tui_node_free(root);
    draw_scene_focus_cleanup(); 
    term_restore();
}