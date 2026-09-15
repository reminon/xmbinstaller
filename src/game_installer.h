#ifndef XMBINSTALLER_GAME_INSTALLER_H
#define XMBINSTALLER_GAME_INSTALLER_H

#include <tamtypes.h>

#define GAME_INSTALLER_MAX_GAMES 256
#define GAME_INSTALLER_MAX_PATH 256
#define GAME_INSTALLER_MAX_ID 16
#define GAME_INSTALLER_MAX_TITLE 64

typedef struct game_entry {
    char path[GAME_INSTALLER_MAX_PATH];
    char id[GAME_INSTALLER_MAX_ID];
    char title[GAME_INSTALLER_MAX_TITLE];
    u64 size;
} game_entry_t;

typedef struct game_scan_result {
    game_entry_t games[GAME_INSTALLER_MAX_GAMES];
    unsigned int count;
} game_scan_result_t;

void game_scan_usb(game_scan_result_t *result);
void game_scan_mmce(game_scan_result_t *result);
int game_extract_id(const char *iso_path, char id_out[GAME_INSTALLER_MAX_ID]);

#endif

int game_create_hdl_partition(const char *game_name, u32 size_in_mb);
int game_create_pp_partition(const char *game_name);
int game_write_iso(const char *iso_path, const char *game_name, u32 size_in_mb);
int game_write_hdl_header(const char *game_name, const char *startup,
                          const char *title, u32 size_in_kb, u32 disc_type);
int game_install(const char *iso_path, const char *title);
