#include "keystore.h"
#include <string.h>

static int char2int(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
}

static int hex2bin(const char *src, int srclen, uint8_t *dst, int *dstlen) {
    *dstlen = 0;
    if (srclen % 2 != 0) return KEYSTORE_ERROR_ODD_LEN_VALUE;
    for (int i = 0; i < srclen; i += 2) {
        int hi = char2int(src[i]);
        int lo = char2int(src[i+1]);
        if (hi < 0 || lo < 0) return KEYSTORE_ERROR_ODD_LEN_VALUE;
        dst[(*dstlen)++] = (hi << 4) | lo;
    }
    return 0;
}

static void parse_line(const char *line, int len, KeyStore *ks) {
    const char *eq = (const char*)memchr(line, '=', len);
    if (!eq) return;
    int keylen = eq - line;
    const char *val = eq + 1;
    int vallen = len - keylen - 1;
    while (vallen > 0 && (val[vallen-1] == '\r' || val[vallen-1] == '\n'))
        vallen--;
    uint8_t bin[32];
    int binlen = 0;
    if (hex2bin(val, vallen, bin, &binlen) != 0) return;

    #define SETKEY(NAME, FIELD) \
        if (keylen == (int)sizeof(NAME)-1 && memcmp(line, NAME, keylen) == 0) \
            memcpy(ks->FIELD, bin, binlen);

    SETKEY("MG_SIG_MASTER_KEY",     SignatureMasterKey)
    SETKEY("MG_SIG_HASH_KEY",       SignatureHashKey)
    SETKEY("MG_KBIT_MASTER_KEY",    KbitMasterKey)
    SETKEY("MG_KBIT_IV",            KbitIV)
    SETKEY("MG_KC_MASTER_KEY",      KcMasterKey)
    SETKEY("MG_KC_IV",              KcIV)
    SETKEY("MG_ROOTSIG_MASTER_KEY", RootSignatureMasterKey)
    SETKEY("MG_ROOTSIG_HASH_KEY",   RootSignatureHashKey)
    SETKEY("MG_CONTENT_TABLE_IV",   ContentTableIV)
    SETKEY("MG_CONTENT_IV",         ContentIV)
    #undef SETKEY
}

int KeyStore_LoadMemory(KeyStore *ks, const char *data, unsigned int size) {
    memset(ks, 0, sizeof(*ks));
    const char *p = data;
    const char *end = data + size;
    while (p < end) {
        const char *nl = (const char*)memchr(p, '\n', end - p);
        int len = nl ? (int)(nl - p + 1) : (int)(end - p);
        parse_line(p, len, ks);
        p += len;
    }
    if (!ks->SignatureMasterKey[0] || !ks->KbitMasterKey[0] || !ks->KcMasterKey[0])
        return KEYSTORE_ERROR_MISSING_KEY;
    return 0;
}
