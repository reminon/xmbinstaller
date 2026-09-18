#ifndef KELF_PORT_H
#define KELF_PORT_H

#include <stdint.h>
#include "keystore.h"

#define SYSTEM_TYPE_PSX 1
#define BIT_BLOCK_ENCRYPTED 1
#define BIT_BLOCK_SIGNED 2

#pragma pack(push, 1)
typedef struct {
    uint8_t  UserDefined[16];
    uint32_t ContentSize;
    uint16_t HeaderSize;
    uint8_t  SystemType;
    uint8_t  ApplicationType;
    uint16_t Flags;
    uint16_t BitCount;
    uint32_t MGZones;
} KELFHeader;

typedef struct {
    uint32_t HeaderSize;
    uint8_t  BlockCount;
    uint8_t  gap[3];
    struct {
        uint32_t Size;
        uint32_t Flags;
        uint8_t  Signature[8];
    } Blocks[256];
} BitTable;
#pragma pack(pop)

typedef struct {
    KeyStore ks;
    uint8_t  Kbit[16];
    uint8_t  Kc[16];
    BitTable bitTable;
    uint8_t *Content;
    uint32_t ContentSize;
} KelfCtx;

int kelf_sign_elf(const char *elf_path, const char *kelf_path);

#endif
int kelf_sign_embedded_opl(const char *kelf_out_path);
