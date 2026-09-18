#ifndef DES_COMPAT_H
#define DES_COMPAT_H

#include <wolfssl/wolfcrypt/des3.h>
#include <string.h>
#include <stdint.h>

typedef uint8_t DES_cblock[8];
typedef const uint8_t const_DES_cblock[8];
#define DES_ENCRYPT 1
#define DES_DECRYPT 0

typedef struct {
    Des des;
} DES_key_schedule;

static inline int DES_set_key(const const_DES_cblock *key, DES_key_schedule *sc) {
    return wc_Des_SetKey(&sc->des, (const byte*)key, NULL, DES_ENCRYPTION);
}

static inline void DES_cbc_encrypt(const uint8_t *in, uint8_t *out, long len,
                                    DES_key_schedule *sc, DES_cblock *iv, int enc) {
    wc_Des_SetIV(&sc->des, (const byte*)*iv);
    if (enc == DES_ENCRYPT)
        wc_Des_CbcEncrypt(&sc->des, out, in, len);
    else
        wc_Des_CbcDecrypt(&sc->des, out, in, len);
    memcpy(*iv, out + len - 8, 8);
}

static inline void DES_ede2_cbc_encrypt(const uint8_t *in, uint8_t *out, long len,
                                         DES_key_schedule *sc1, DES_key_schedule *sc2,
                                         DES_cblock *iv, int enc) {
    Des3 des3;
    byte key[16];
    memcpy(key, &sc1->des, 8);
    memcpy(key + 8, &sc2->des, 8);
    wc_Des3_SetKey(&des3, key, (const byte*)*iv,
                   enc == DES_ENCRYPT ? DES_ENCRYPTION : DES_DECRYPTION);
    if (enc == DES_ENCRYPT)
        wc_Des3_CbcEncrypt(&des3, out, in, len);
    else
        wc_Des3_CbcDecrypt(&des3, out, in, len);
}

static inline void DES_ede3_cbc_encrypt(const uint8_t *in, uint8_t *out, long len,
                                         DES_key_schedule *sc1, DES_key_schedule *sc2,
                                         DES_key_schedule *sc3, DES_cblock *iv, int enc) {
    Des3 des3;
    byte key[24];
    memcpy(key, &sc1->des, 8);
    memcpy(key + 8, &sc2->des, 8);
    memcpy(key + 16, &sc3->des, 8);
    wc_Des3_SetKey(&des3, key, (const byte*)*iv,
                   enc == DES_ENCRYPT ? DES_ENCRYPTION : DES_DECRYPTION);
    if (enc == DES_ENCRYPT)
        wc_Des3_CbcEncrypt(&des3, out, in, len);
    else
        wc_Des3_CbcDecrypt(&des3, out, in, len);
}

#endif
