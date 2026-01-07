#include "../src/minitest.h"
#include "../src/microui.h"
#include "../src/ui_renderer.h"
#include "../src/term.h"
#include "../src/log.h"

static  char logbuf[64000];
static   int logbuf_updated = 0;
static void write_log(const char *text) {
  if (logbuf[0]) { strcat(logbuf, "\n"); }
  strcat(logbuf, text);
  logbuf_updated = 1;
}

static int text_width(mu_Font font, const char *text, int len) {
    return r_get_text_width(text, (len == -1) ? strlen(text) : len);
}

static int text_height(mu_Font font) {return r_get_text_height();}

static void process_frame(mu_Context *ctx) {
    mu_begin(ctx);
    if (mu_begin_window(ctx, "Log Window", mu_rect(0, 1, 40, 6))) {

        /* window info */
        if (mu_header(ctx, "Window Info")) {
            mu_Container *win = mu_get_current_container(ctx);
            char buf[64];
            mu_layout_row(ctx, 2, (int[]){20, -1}, 0);
            mu_label(ctx, " Position: ");
            sprintf(buf, "%d, %d", win->rect.x, win->rect.y);
            mu_label(ctx, buf);
            mu_label(ctx, " Size:");
            sprintf(buf, "%d, %d", win->rect.w, win->rect.h);
            mu_label(ctx, buf);

            mu_layout_row(ctx, 1, (int[]){-1}, 0);
            mu_label(ctx, "====================");
            mu_text(ctx, logbuf);
            mu_label(ctx, "====================");
        }

        if (mu_header_ex(ctx, "Tree and Text", MU_OPT_EXPANDED)) {
            if (mu_begin_treenode(ctx, "Test 1")) {
                if (mu_begin_treenode(ctx, "Test 1a")) {
                    mu_label(ctx, "Hello");
                    mu_label(ctx, "world");
                    mu_end_treenode(ctx);
                }
                if (mu_begin_treenode(ctx, "Test 1b")) {
                    if (mu_button(ctx, "Button 1")) {write_log("Pressed button 1");}
                    if (mu_button(ctx, "Button 2")) {write_log("Pressed button 2");}
                    mu_end_treenode(ctx);
                }
                mu_end_treenode(ctx);
            }
            if (mu_begin_treenode(ctx, "Test 2")) {
                mu_layout_row(ctx, 2, (int[]){20, -1}, 0);
                if (mu_button(ctx, "Button 3")) {write_log("Pressed button 3");}
                if (mu_button(ctx, "Button 4")) {write_log("Pressed button 4");}
                if (mu_button(ctx, "Button 5")) {write_log("Pressed button 5");}
                if (mu_button(ctx, "Button 6")) {write_log("Pressed button 6");}
                mu_end_treenode(ctx);
            }
            if (mu_begin_treenode(ctx, "Test 3")) {
                static int checks[3] = {1, 0, 1};
                mu_checkbox(ctx, "Checkbox 1", &checks[0]);
                mu_checkbox(ctx, "Checkbox 2", &checks[1]);
                mu_checkbox(ctx, "Checkbox 3", &checks[2]);
                mu_end_treenode(ctx);
            }
        }

        mu_end_window(ctx);
    }

    mu_end(ctx);
}

TEST(test, microui) {
    SetConsoleOutputCP(65001);

    /* init microui */
    mu_Context *ctx = malloc(sizeof(mu_Context));
    mu_init(ctx);
    ctx->text_width = text_width;
    ctx->text_height = text_height;

    r_init();
    term_hide_cursor();

    while (1) {
        TermEvent e = term_poll_event();
        if (e.type == TERM_EV_NONE) {
            Sleep(10);
            continue;
        }

        switch (e.type) {
        case TERM_EV_KEY:
            if (e.u.key.pressed && e.u.key.key_code == VK_ESCAPE) { goto QUIT; }

            if (e.u.key.pressed)    mu_input_keydown(ctx, e.u.key.utf8[0]);
            else                    mu_input_keyup(ctx, e.u.key.utf8[0]);
            break;

        case TERM_EV_MOUSE:
            switch (e.u.mouse.type) {
                case 1: mu_input_mousemove(ctx, e.u.mouse.x, e.u.mouse.y); break;
                case 2: mu_input_mousedown(ctx, e.u.mouse.x, e.u.mouse.y, MU_MOUSE_LEFT); break;
                case 3: mu_input_mouseup(ctx, e.u.mouse.x, e.u.mouse.y, MU_MOUSE_LEFT | MU_MOUSE_RIGHT | MU_MOUSE_MIDDLE); break;
                case 4: mu_input_mousedown(ctx, e.u.mouse.x, e.u.mouse.y, MU_MOUSE_RIGHT); break;
                case 6: mu_input_mousedown(ctx, e.u.mouse.x, e.u.mouse.y, MU_MOUSE_MIDDLE); break;
                case 8: mu_input_scroll(ctx, 0, -e.u.mouse.wheel); break;
            }
            break;

        case TERM_EV_RESIZE:
            r_init();
            break;

        default:
            break;
        }

        process_frame(ctx);

        r_clear(mu_color(255, 255, 0, 255));
        mu_Command *cmd = NULL;
        while (mu_next_command(ctx, &cmd)) {
            switch (cmd->type) {
                case MU_COMMAND_TEXT: r_draw_text(cmd->text.str, cmd->text.pos, cmd->text.color); break;
                case MU_COMMAND_RECT: r_draw_rect(cmd->rect.rect, cmd->rect.color); break;
                case MU_COMMAND_ICON: r_draw_icon(cmd->icon.id, cmd->icon.rect, cmd->icon.color); break;
                case MU_COMMAND_CLIP: r_set_clip_rect(cmd->clip.rect); break;
            }
        }
        r_present();
    }

QUIT:
    term_shutdown();
    term_clear_screen();
    term_show_cursor();
}