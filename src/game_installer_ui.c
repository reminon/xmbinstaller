#include <string.h>
#include <stdio.h>
#include <delaythread.h>
#include "game_installer.h"
#include "ui_gskit.h"

void run_game_installer_ui(const void *pad_raw)
{
    (void)pad_raw;

    gskit_ui_init();

    game_scan_result_t scan;
    memset(&scan, 0, sizeof(scan));
    game_scan_usb(&scan);

    for (;;) {
        if (scan.count == 0) {
            const char *empty[] = { "NO GAMES FOUND - INSERT USB" };
            int r = gskit_ui_choose_list("XMB INSTALLER", "SCAN USB FOR ISO FILES",
                                          empty, 1);
            if (r < 0) return;
            /* rescan */
            memset(&scan, 0, sizeof(scan));
            game_scan_usb(&scan);
            continue;
        }

        /* build titles array */
        const char *titles[GAME_INSTALLER_MAX_GAMES];
        for (unsigned int i = 0; i < scan.count; i++)
            titles[i] = scan.games[i].title;

        int selected = gskit_ui_choose_list("XMB INSTALLER", "SELECT GAME TO INSTALL",
                                             titles, (int)scan.count);
        if (selected < 0) return;

        /* confirm screen */
        char confirm_msg[128];
        snprintf(confirm_msg, sizeof(confirm_msg), "INSTALL: %.50s", scan.games[selected].title);
        const char *confirm_items[] = { confirm_msg, "CANCEL" };
        int confirm = gskit_ui_choose_list("XMB INSTALLER", "CONFIRM INSTALLATION",
                                            confirm_items, 2);
        if (confirm != 0) continue;

        /* install */
        const char *progress_items[] = { "INSTALLING - PLEASE WAIT..." };
        /* show progress screen briefly */
        gskit_ui_choose_list("XMB INSTALLER", scan.games[selected].title,
                              progress_items, 1);

        int result = game_install(scan.games[selected].path,
                                  scan.games[selected].title);

        /* result screen */
        char result_msg[64];
        if (result >= 0)
            snprintf(result_msg, sizeof(result_msg), "INSTALL COMPLETE");
        else
            snprintf(result_msg, sizeof(result_msg), "INSTALL FAILED: %d", result);

        const char *result_items[] = { result_msg, "INSTALL ANOTHER", "EXIT" };
        int next = gskit_ui_choose_list("XMB INSTALLER", scan.games[selected].title,
                                         result_items, 3);
        if (next == 2) return;
    }
}
