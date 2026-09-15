#include <stdio.h>
#include <string.h>
#define NEWLIB_PORT_AWARE
#include <fileio.h>
#include <fileXio_rpc.h>
#include <iox_stat.h>
#include "game_installer.h"

static int ends_with_iso(const char *name)
{
    size_t len = strlen(name);
    if (len < 4) return 0;
    const char *ext = name + len - 4;
    return (ext[0] == '.' &&
            (ext[1] == 'i' || ext[1] == 'I') &&
            (ext[2] == 's' || ext[2] == 'S') &&
            (ext[3] == 'o' || ext[3] == 'O'));
}

static void scan_path(game_scan_result_t *result, const char *base_path)
{
    iox_dirent_t entry;
    int fd = fileXioDopen(base_path);
    if (fd < 0) return;

    while (fileXioDread(fd, &entry) > 0) {
        if (strcmp(entry.name, ".") == 0 || strcmp(entry.name, "..") == 0)
            continue;
        if (result->count >= GAME_INSTALLER_MAX_GAMES)
            break;
        if (ends_with_iso(entry.name)) {
            game_entry_t *g = &result->games[result->count];
            snprintf(g->path, sizeof(g->path), "%s/%s", base_path, entry.name);
            snprintf(g->title, sizeof(g->title), "%.*s",
                     (int)(strlen(entry.name) - 4), entry.name);
            g->size = ((u64)entry.stat.hisize << 32) | entry.stat.size;
            g->id[0] = '\0';
            ++result->count;
        }
    }
    fileXioDclose(fd);
}

void game_scan_usb(game_scan_result_t *result)
{
    scan_path(result, "mass:/");
    scan_path(result, "mass:/DVD");
    scan_path(result, "mass:/CD");
}

void game_scan_mmce(game_scan_result_t *result)
{
    scan_path(result, "mc0:/");
}

int game_extract_id(const char *iso_path, char id_out[GAME_INSTALLER_MAX_ID])
{
    /* TODO: mount ISO and read SYSTEM.CNF */
    (void)iso_path; id_out[0] = '\0';
    return -1;
}

int game_create_hdl_partition(const char *game_name, u32 size_in_mb)
{
    char cmd[256];
    int fd;
    snprintf(cmd, sizeof(cmd), "hdd0:__%s,,,%luM,HDL", game_name,
             (unsigned long)size_in_mb);
    fd = fileXioOpen(cmd, O_CREAT | O_TRUNC | O_WRONLY, 0);
    if (fd < 0)
        return fd;
    fileXioClose(fd);
    return 0;
}

int game_create_pp_partition(const char *game_name)
{
    int formatArgs[3] = {8192, 0x2d66, 0};
    char cmd[256];
    char blockdev[256];
    int fd, result;
    snprintf(cmd, sizeof(cmd), "hdd0:PP.%s,,,128M,PFS", game_name);
    fd = fileXioOpen(cmd, O_CREAT | O_TRUNC | O_WRONLY, 0);
    if (fd < 0)
        return fd;
    fileXioClose(fd);
    snprintf(blockdev, sizeof(blockdev), "hdd0:PP.%s", game_name);
    result = fileXioFormat("pfs:", blockdev,
                           (const char *)formatArgs, sizeof(formatArgs));
    return result;
}
