#include "iso_reader.h"
#include <string.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <fileio.h>

#define SECTOR_SIZE     2048
#define PVD_SECTOR      16
#define PVD_OFFSET      (PVD_SECTOR * SECTOR_SIZE)
#define DIR_RECORD_SIZE 33

static unsigned char iso_buf[SECTOR_SIZE] __attribute__((aligned(64)));

/* Read a sector from an open ISO file descriptor */
static int read_sector(int fd, unsigned int sector, void *buf) {
    fileXioLseek(fd, sector * SECTOR_SIZE, SEEK_SET);
    return fileXioRead(fd, buf, SECTOR_SIZE);
}

/* Parse directory record to get name and location */
static int parse_dir_record(const unsigned char *rec, char *name, int *namelen,
                              unsigned int *lba, unsigned int *size) {
    int reclen = rec[0];
    if (reclen == 0) return -1;
    *lba     = rec[2] | (rec[3]<<8) | (rec[4]<<16) | (rec[5]<<24);
    *size    = rec[10] | (rec[11]<<8) | (rec[12]<<16) | (rec[13]<<24);
    *namelen = rec[32];
    if (*namelen > 0 && *namelen <= 128)
        memcpy(name, rec + 33, *namelen);
    name[*namelen] = '\0';
    /* strip version ;1 */
    char *semi = (char*)memchr(name, ';', *namelen);
    if (semi) *semi = '\0';
    return reclen;
}

/* Find a file in the root directory of the ISO */
static int find_file(int fd, unsigned int root_lba, unsigned int root_size,
                     const char *target, unsigned int *file_lba, unsigned int *file_size) {
    unsigned int sectors = (root_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    for (unsigned int s = 0; s < sectors; s++) {
        read_sector(fd, root_lba + s, iso_buf);
        unsigned int offset = 0;
        while (offset < SECTOR_SIZE) {
            char name[129];
            int namelen;
            unsigned int lba, size;
            int reclen = parse_dir_record(iso_buf + offset, name, &namelen, &lba, &size);
            if (reclen <= 0) break;
            /* skip . and .. */
            if (namelen == 1 && (iso_buf[offset+33] == 0 || iso_buf[offset+33] == 1)) {
                offset += reclen;
                continue;
            }
            if (strcasecmp(name, target) == 0) {
                *file_lba = lba;
                *file_size = size;
                return 0;
            }
            offset += reclen;
        }
    }
    return -1;
}

int iso_get_game_id(const char *iso_path, char *id_out, int id_max) {
    int fd = fileXioOpen(iso_path, O_RDONLY, 0);
    if (fd < 0) return -1;

    /* Read PVD */
    read_sector(fd, PVD_SECTOR, iso_buf);
    if (iso_buf[0] != 1 || memcmp(iso_buf+1, "CD001", 5) != 0) {
        fileXioClose(fd);
        return -2; /* not ISO9660 */
    }

    /* Root directory record is at offset 156 in PVD */
    unsigned int root_lba  = iso_buf[158] | (iso_buf[159]<<8) |
                             (iso_buf[160]<<16) | (iso_buf[161]<<24);
    unsigned int root_size = iso_buf[166] | (iso_buf[167]<<8) |
                             (iso_buf[168]<<16) | (iso_buf[169]<<24);

    /* Find SYSTEM.CNF */
    unsigned int cnf_lba, cnf_size;
    if (find_file(fd, root_lba, root_size, "SYSTEM.CNF", &cnf_lba, &cnf_size) < 0) {
        fileXioClose(fd);
        return -3;
    }

    /* Read SYSTEM.CNF */
    if (cnf_size > SECTOR_SIZE) cnf_size = SECTOR_SIZE;
    read_sector(fd, cnf_lba, iso_buf);
    fileXioClose(fd);

    /* Parse BOOT2 = cdrom0:\SLUS_123.45 */
    iso_buf[cnf_size] = '\0';
    char *boot = (char*)strstr((char*)iso_buf, "BOOT2");
    if (!boot) boot = (char*)strstr((char*)iso_buf, "BOOT");
    if (!boot) return -4;

    char *eq = strchr(boot, '=');
    if (!eq) return -4;
    eq++;
    while (*eq == ' ') eq++;

    /* Skip cdrom0:\ or cdrom: prefix */
    char *slash = strrchr(eq, '\\');
    if (!slash) slash = strrchr(eq, '/');
    if (slash) eq = slash + 1;

    /* Copy game ID, strip ;1 and whitespace */
    int i = 0;
    while (*eq && *eq != ';' && *eq != '\r' && *eq != '\n' && *eq != ' ' && i < id_max-1)
        id_out[i++] = *eq++;
    id_out[i] = '\0';

    return i > 0 ? 0 : -5;
}
