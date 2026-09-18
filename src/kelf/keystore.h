#ifndef KEYSTORE_H
#define KEYSTORE_H

#include <stdint.h>

#define KEYSTORE_ERROR_OPEN_FAILED        1
#define KEYSTORE_ERROR_LINE_NOT_KEY_VALUE 2
#define KEYSTORE_ERROR_ODD_LEN_VALUE      3
#define KEYSTORE_ERROR_MISSING_KEY        4

typedef struct {
    uint8_t SignatureMasterKey[8];
    uint8_t SignatureHashKey[8];
    uint8_t KbitMasterKey[16];
    uint8_t KbitIV[8];
    uint8_t KcMasterKey[16];
    uint8_t KcIV[8];
    uint8_t RootSignatureMasterKey[8];
    uint8_t RootSignatureHashKey[8];
    uint8_t ContentTableIV[8];
    uint8_t ContentIV[8];
} KeyStore;

int KeyStore_LoadMemory(KeyStore *ks, const char *data, unsigned int size);

#endif
