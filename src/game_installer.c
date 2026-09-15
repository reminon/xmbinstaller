#include <hdd-ioctl.h>
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

#define COPY_BUFFER_SECTORS 128
#define HDL_HEADER_SECTORS  8192  /* 4MB header */

static unsigned char copy_buf[COPY_BUFFER_SECTORS * 512] __attribute__((aligned(64)));

int game_write_iso(const char *iso_path, const char *game_name,
                   u32 size_in_mb)
{
    int iso_fd, result;
    u32 lba, sectors_written, sectors_total;


    /* open ISO */
    iso_fd = fileXioOpen(iso_path, O_RDONLY, 0);
    if (iso_fd < 0)
        return iso_fd;

    /* get partition start LBA */
    {
        char part_path[256];
        iox_stat_t stat;
        snprintf(part_path, sizeof(part_path), "hdd0:__%s", game_name);
        result = fileXioGetStat(part_path, &stat);
        if (result < 0) {
            fileXioClose(iso_fd);
            return result;
        }
        /* start LBA is in private_0 field */
        lba = stat.private_0 + HDL_HEADER_SECTORS;
    }

    sectors_total = (u32)((u64)size_in_mb * 1024 * 1024 / 512) - HDL_HEADER_SECTORS;
    sectors_written = 0;
    result = 0;

    while (sectors_written < sectors_total) {
        static unsigned char xferbuf[sizeof(hddAtaTransfer_t) + COPY_BUFFER_SECTORS * 512] __attribute__((aligned(64)));
        hddAtaTransfer_t *xfer = (hddAtaTransfer_t *)xferbuf;
        u32 count = sectors_total - sectors_written;
        u32 bytes_read;
        if (count > COPY_BUFFER_SECTORS)
            count = COPY_BUFFER_SECTORS;

        bytes_read = fileXioRead(iso_fd, copy_buf, count * 512);
        if ((int)bytes_read <= 0) {
            result = (int)bytes_read;
            break;
        }
        count = bytes_read / 512;


        xfer->lba = lba + sectors_written;
        xfer->size = count;
        memcpy(xfer->data, copy_buf, count * 512);

        result = fileXioDevctl("hdd0:", HDIOC_WRITESECTOR,
                               xfer, sizeof(hddAtaTransfer_t) + count * 512,
                               NULL, 0);
        if (result < 0)
            break;

        sectors_written += count;
    }

    fileXioClose(iso_fd);
    return result;
}
