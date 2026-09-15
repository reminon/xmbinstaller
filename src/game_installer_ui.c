#include <stdio.h>
#include <string.h>
#include <libpad.h>
#include <delaythread.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include "game_installer.h"
#include "ui.h"
#include "storage.h"

static void draw_game_list(const game_scan_result_t *scan, unsigned int selected)
{
    unsigned int i;
    ui_begin();
    ui_printf("XMB Game Installer\n\n");
    if (scan->count == 0) {
        ui_inverse_status("NO GAMES FOUND");
        ui_printf("Place ISO files on USB drive.\n");
    } else {
        ui_printf("Select a game to install:\n\n");
        for (i = 0; i < scan->count && i < 10; ++i) {
            if (i == selected)
                ui_inverse_status(scan->games[i].title);
            else
                ui_printf("%s\n", scan->games[i].title);
        }
    }
    ui_set_position(UI_SAFE_LEFT, 220);
    ui_printf("X = Install   O = Back   Triangle = Rescan");
    ui_sync();
}

void run_game_installer_ui(const void *pad_raw)
{
    (void)pad_raw;
    game_scan_result_t scan;
    struct padButtonStatus buttons;
    unsigned int old_buttons = 0;
    unsigned int selected = 0;

    memset(&scan, 0, sizeof(scan));
    game_scan_usb(&scan);
    draw_game_list(&scan, selected);

    for (;;) {
        int state = padGetState(0, 0);
        if ((state == PAD_STATE_STABLE || state == PAD_STATE_FINDCTP1) &&
            padRead(0, 0, &buttons) != 0) {
            unsigned int current = 0xffffu ^ buttons.btns;
            unsigned int pressed = current & ~old_buttons;
            old_buttons = current;

            if ((pressed & PAD_UP) != 0 && selected > 0) {
                --selected;
                draw_game_list(&scan, selected);
            } else if ((pressed & PAD_DOWN) != 0 && selected + 1 < scan.count) {
                ++selected;
                draw_game_list(&scan, selected);
            } else if ((pressed & PAD_CROSS) != 0 && scan.count > 0) {
                ui_begin();
                ui_inverse_status("INSTALLING");
                ui_printf("Installing %s...\n", scan.games[selected].title);
                ui_sync();
                int result = game_install(scan.games[selected].path,
                                          scan.games[selected].title);
                ui_begin();
                if (result >= 0)
                    ui_inverse_status("INSTALL COMPLETE");
                else {
                    ui_inverse_status("INSTALL FAILED");
                    ui_printf("Error: %d\n", result);
                }
                ui_printf("Press any button to continue.\n");
                ui_sync();
                DelayThread(2000000);
                draw_game_list(&scan, selected);
            } else if ((pressed & PAD_TRIANGLE) != 0) {
                memset(&scan, 0, sizeof(scan));
                game_scan_usb(&scan);
                selected = 0;
                draw_game_list(&scan, selected);
            } else if ((pressed & PAD_CIRCLE) != 0) {
                return;
            }
        }
        DelayThread(16000);
    }
}
