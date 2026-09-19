#include "ui_gskit.h"
#include <gsKit.h>
#include <dmaKit.h>
#include <gsToolkit.h>
#include <libpad.h>
#include <kernel.h>
#include <string.h>
#include <ctype.h>

/* 5x7 bitmap font — extracted from DESR_XMB_Installer reference binary */
static const unsigned char font_glyphs[] = {
    /* A */ 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11,
    /* B */ 0x1E, 0x11, 0x11, 0x1E, 0x11, 0x11, 0x1E,
    /* C */ 0x0E, 0x11, 0x10, 0x10, 0x10, 0x11, 0x0E,
    /* D */ 0x1E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x1E,
    /* E */ 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x1F,
    /* F */ 0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10,
    /* G */ 0x0E, 0x11, 0x10, 0x17, 0x11, 0x11, 0x0F,
    /* H */ 0x11, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11,
    /* I */ 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0E,
    /* J */ 0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C,
    /* K */ 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11,
    /* L */ 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1F,
    /* M */ 0x11, 0x1B, 0x15, 0x11, 0x11, 0x11, 0x11,
    /* N */ 0x11, 0x19, 0x15, 0x13, 0x11, 0x11, 0x11,
    /* O */ 0x0E, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E,
    /* P */ 0x1E, 0x11, 0x11, 0x1E, 0x10, 0x10, 0x10,
    /* Q */ 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D,
    /* R */ 0x1E, 0x11, 0x11, 0x1E, 0x14, 0x12, 0x11,
    /* S */ 0x0F, 0x10, 0x10, 0x0E, 0x01, 0x01, 0x1E,
    /* T */ 0x1F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    /* U */ 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x0E,
    /* V */ 0x11, 0x11, 0x11, 0x11, 0x11, 0x0A, 0x04,
    /* W */ 0x11, 0x11, 0x11, 0x15, 0x15, 0x1B, 0x11,
    /* X */ 0x11, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x11,
    /* Y */ 0x11, 0x11, 0x0A, 0x04, 0x04, 0x04, 0x04,
    /* Z */ 0x1F, 0x01, 0x02, 0x04, 0x08, 0x10, 0x1F,
    /* 0 */ 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E,
    /* 1 */ 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E,
    /* 2 */ 0x0E, 0x11, 0x01, 0x06, 0x08, 0x10, 0x1F,
    /* 3 */ 0x1F, 0x01, 0x02, 0x06, 0x01, 0x11, 0x0E,
    /* 4 */ 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02,
    /* 5 */ 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E,
    /* 6 */ 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E,
    /* 7 */ 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08,
    /* 8 */ 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E,
    /* 9 */ 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C,
    /* - */ 0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00,
    /* . */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C,
    /* : */ 0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00,
    /* / */ 0x01, 0x02, 0x02, 0x04, 0x08, 0x08, 0x10,
    /* _ */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1F,
    /* ! */ 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04,
    /* space */ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

static const char font_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-.:/_! ";

#define GS_COLOR(r,g,b,a) ((u64)(a)<<24|(u64)(b)<<16|(u64)(g)<<8|(u64)(r))

/* Colors from reference binary */
#define COLOR_HEADER_BG     GS_COLOR(0x20,0x50,0x90,0x80)
#define COLOR_FOOTER_BG     GS_COLOR(0x15,0x30,0x55,0x80)
#define COLOR_SEL_BAR       GS_COLOR(0x1C,0x4A,0x82,0x80)
#define COLOR_UNSEL_BAR     GS_COLOR(0x0D,0x1D,0x37,0x80)
#define COLOR_TEXT_MAIN     GS_COLOR(0xD6,0xEA,0xFA,0x80)
#define COLOR_TEXT_SUB      GS_COLOR(0xA0,0xC2,0xE7,0x80)
#define COLOR_TEXT_DIM      GS_COLOR(0x60,0x80,0xA0,0x80)
#define COLOR_ACCENT        GS_COLOR(0xFF,0xFA,0xE6,0x80)

#define SCREEN_W 640
#define SCREEN_H 448
#define VISIBLE_ROWS 5
#define ROW_STRIDE 52
#define ROW_START_Y 126

static GSGLOBAL *gs = NULL;
static struct padButtonStatus pad_buttons;
static unsigned int old_buttons = 0;

static int char_to_glyph(char c) {
    c = toupper(c);
    for (int i = 0; font_chars[i]; i++)
        if (font_chars[i] == c) return i;
    return -1; /* unknown char */
}

static void draw_text(float x, float y, float scale, u64 color, const char *str) {
    float cx = x;
    for (int i = 0; str[i]; i++) {
        if (str[i] == ' ') { cx += scale * 6; continue; }
        int g = char_to_glyph(str[i]);
        if (g < 0) { cx += scale * 6; continue; }
        const unsigned char *glyph = &font_glyphs[g * 7];
        for (int row = 0; row < 7; row++) {
            for (int col = 4; col >= 0; col--) {
                if (glyph[row] & (1 << col)) {
                    float px = cx + (4-col) * scale;
                    float py = y + row * scale;
                    gsKit_prim_sprite(gs, px, py, px+scale, py+scale, 1, color);
                }
            }
        }
        cx += scale * 6;
    }
}

static void draw_frame(const char *title, const char *subtitle) {
    /* Header bar */
    gsKit_prim_sprite(gs, 0, 0, SCREEN_W, 60, 1, COLOR_HEADER_BG);
    /* Footer bar */
    gsKit_prim_sprite(gs, 0, 400, SCREEN_W, SCREEN_H, 1, COLOR_FOOTER_BG);
    /* Divider lines */
    gsKit_prim_sprite(gs, 0, 58, SCREEN_W, 62, 1, COLOR_ACCENT);
    gsKit_prim_sprite(gs, 0, 398, SCREEN_W, 402, 1, COLOR_ACCENT);
    /* Title */
    draw_text(36, 12, 3.0f, COLOR_TEXT_MAIN, title);
    /* Subtitle */
    draw_text(36, 38, 1.5f, COLOR_TEXT_DIM, subtitle);
    /* Footer hint */
    draw_text(36, 414, 1.5f, COLOR_TEXT_SUB, "D-PAD MOVE  X SELECT  O BACK");
}

void gskit_ui_init(void) {
    dmaKit_init(0, 0, 0, 0, 0, 4);
    dmaKit_chan_init(2);
    gs = gsKit_init_global_custom(0x100000, 0x40000);
    gs->ZBuffering = 0;
    gs->DoubleBuffering = 1;
    gsKit_init_screen(gs);
    gsKit_mode_switch(gs, 1);
}

void gskit_ui_flip(void) {
    gsKit_queue_exec(gs);
    gsKit_sync_flip(gs);
    gsKit_queue_reset(gs->Per_Queue);
}

unsigned int gskit_ui_read_press(void) {
    int state = padGetState(0, 0);
    if (state != PAD_STATE_STABLE && state != PAD_STATE_FINDCTP1)
        return 0;
    if (padRead(0, 0, &pad_buttons) == 0)
        return 0;
    unsigned int current = 0xffffu ^ pad_buttons.btns;
    unsigned int pressed = current & ~old_buttons;
    old_buttons = current;
    return pressed;
}

int gskit_ui_choose_list(const char *title, const char *subtitle,
                    const char **items, int count) {
    int selected = 0;
    int scroll = 0;

    for (;;) {
        /* Draw frame */
        draw_frame(title, subtitle);

        /* Draw rows */
        int visible = count < VISIBLE_ROWS ? count : VISIBLE_ROWS;
        for (int i = 0; i < visible; i++) {
            int idx = scroll + i;
            float ry = ROW_START_Y + i * ROW_STRIDE;
            u64 bar_color = (idx == selected) ? COLOR_SEL_BAR : COLOR_UNSEL_BAR;
            gsKit_prim_sprite(gs, 36, ry, SCREEN_W-36, ry+ROW_STRIDE-4, 1, bar_color);
            draw_text(72, ry+16, 1.55f, COLOR_TEXT_MAIN, items[idx]);
            draw_text(72, ry+43, 0.9f, COLOR_TEXT_SUB, "PS2 GAME");
        }

        gskit_ui_flip();

        unsigned int pressed = gskit_ui_read_press();
        if (pressed & 0x0010) { /* up */
            if (selected > 0) {
                selected--;
                if (selected < scroll) scroll--;
            }
        } else if (pressed & 0x0040) { /* down */
            if (selected < count-1) {
                selected++;
                if (selected >= scroll + VISIBLE_ROWS) scroll++;
            }
        } else if (pressed & 0x4000) { /* confirm */
            return selected;
        } else if (pressed & 0x1000) { /* cancel */
            return -1;
        }
    }
}
