#define NEWLIB_PORT_AWARE
#include <fileio.h>
#include "kelf_port.h"
#include "ps2keys_dat.h"
#include "opl_launcher.h"
#include "des_compat.h"
#include <string.h>
#include <malloc.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>

static uint8_t MG_IV_NULL[8] = { 0 };

static int TdesEncrypt(void *out, const void *in, size_t len, const void *keys, int keycount, const void *iv) {
    DES_key_schedule sc1, sc2, sc3;
    DES_cblock iv_copy;
    memcpy(&iv_copy, iv, 8);
    DES_set_key((const_DES_cblock*)keys, &sc1);
    if (keycount >= 2) DES_set_key((const_DES_cblock*)((uint8_t*)keys+8), &sc2);
    if (keycount >= 3) DES_set_key((const_DES_cblock*)((uint8_t*)keys+16), &sc3);
    if (keycount == 1) DES_cbc_encrypt(in, out, len, &sc1, &iv_copy, DES_ENCRYPT);
    else if (keycount == 2) DES_ede2_cbc_encrypt(in, out, len, &sc1, &sc2, &iv_copy, DES_ENCRYPT);
    else if (keycount == 3) DES_ede3_cbc_encrypt(in, out, len, &sc1, &sc2, &sc3, &iv_copy, DES_ENCRYPT);
    else return -1;
    return 0;
}

static int TdesDecrypt(void *out, const void *in, size_t len, const void *keys, int keycount, const void *iv) {
    DES_key_schedule sc1, sc2, sc3;
    DES_cblock iv_copy;
    memcpy(&iv_copy, iv, 8);
    DES_set_key((const_DES_cblock*)keys, &sc1);
    if (keycount >= 2) DES_set_key((const_DES_cblock*)((uint8_t*)keys+8), &sc2);
    if (keycount >= 3) DES_set_key((const_DES_cblock*)((uint8_t*)keys+16), &sc3);
    if (keycount == 1) DES_cbc_encrypt(in, out, len, &sc1, &iv_copy, DES_DECRYPT);
    else if (keycount == 2) DES_ede2_cbc_encrypt(in, out, len, &sc1, &sc2, &iv_copy, DES_DECRYPT);
    else if (keycount == 3) DES_ede3_cbc_encrypt(in, out, len, &sc1, &sc2, &sc3, &iv_copy, DES_DECRYPT);
    else return -1;
    return 0;
}

static void xor_bit(const void *a, const void *b, void *out, size_t len) {
    for (size_t i = 0; i < len; i++)
        ((uint8_t*)out)[i] = ((uint8_t*)a)[i] ^ ((uint8_t*)b)[i];
}

static void GetHeaderSignature(KelfCtx *k, KELFHeader *hdr, uint8_t sig[8]) {
    uint8_t enc[sizeof(KELFHeader)];
    TdesEncrypt(enc, hdr, sizeof(KELFHeader), k->ks.SignatureMasterKey, 1, MG_IV_NULL);
    memcpy(sig, enc + sizeof(enc) - 8, 8);
    TdesDecrypt(sig, sig, 8, k->ks.SignatureHashKey, 1, MG_IV_NULL);
    TdesEncrypt(sig, sig, 8, k->ks.SignatureMasterKey, 1, MG_IV_NULL);
}

static void DeriveKEK(KelfCtx *k, KELFHeader *hdr, uint8_t kek[16]) {
    uint8_t *h = (uint8_t*)hdr;
    uint8_t hdata[8];
    xor_bit(h, h+8, hdata, 8);
    xor_bit(k->ks.KbitIV, hdata, kek, 8);
    xor_bit(k->ks.KcIV, hdata, kek+8, 8);
    TdesEncrypt(kek, kek, 8, k->ks.KbitMasterKey, 2, MG_IV_NULL);
    TdesEncrypt(kek+8, kek+8, 8, k->ks.KcMasterKey, 2, MG_IV_NULL);
}

static void EncryptKeys(KelfCtx *k, uint8_t kek[16]) {
    TdesEncrypt(k->Kbit, k->Kbit, 8, kek, 2, MG_IV_NULL);
    TdesEncrypt(k->Kbit+8, k->Kbit+8, 8, kek, 2, MG_IV_NULL);
    TdesEncrypt(k->Kc, k->Kc, 8, kek, 2, MG_IV_NULL);
    TdesEncrypt(k->Kc+8, k->Kc+8, 8, kek, 2, MG_IV_NULL);
}

static void GetBitTableSignature(KelfCtx *k, uint8_t sig[8]) {
    uint8_t hash[8];
    memcpy(hash, k->Kbit, 8);
    if (memcmp(k->Kbit, k->Kbit+8, 8) != 0) xor_bit(k->Kbit+8, hash, hash, 8);
    xor_bit(k->Kc, hash, hash, 8);
    if (memcmp(k->Kc, k->Kc+8, 8) != 0) xor_bit(k->Kc+8, hash, hash, 8);
    int n = k->bitTable.BlockCount * 2 + 1;
    for (int i = 0; i < n; i++)
        xor_bit(&((uint8_t*)&k->bitTable)[i*8], hash, hash, 8);
    uint8_t sigkey[16];
    memcpy(sigkey, k->ks.SignatureMasterKey, 8);
    memcpy(sigkey+8, k->ks.SignatureHashKey, 8);
    TdesEncrypt(sig, hash, 8, sigkey, 2, MG_IV_NULL);
}

static void GetRootSignature(KelfCtx *k, uint8_t *hsig, uint8_t *btsig, uint8_t root[8]) {
    /* max: hsig(8) + btsig(8) + 256*8 = 2064 */
    uint8_t sigs[2064];
    int len = 0;
    memcpy(sigs+len, hsig, 8); len+=8;
    memcpy(sigs+len, btsig, 8); len+=8;
    for (int i = 0; i < k->bitTable.BlockCount; i++)
        if (k->bitTable.Blocks[i].Flags & BIT_BLOCK_SIGNED) {
            memcpy(sigs+len, k->bitTable.Blocks[i].Signature, 8); len+=8;
        }
    TdesEncrypt(sigs, sigs, len, k->ks.RootSignatureMasterKey, 1, MG_IV_NULL);
    TdesDecrypt(root, sigs+len-8, 8, k->ks.RootSignatureHashKey, 2, MG_IV_NULL);
}

int kelf_load_content(KelfCtx *k, const char *path) {
    int fd = fileXioOpen(path, O_RDONLY, 0);
    if (fd < 0) return -1;
    iox_stat_t st;
    fileXioGetStat(path, &st);
    k->ContentSize = st.size;
    k->Content = (uint8_t*)malloc(k->ContentSize);
    if (!k->Content) { fileXioClose(fd); return -2; }
    fileXioRead(fd, k->Content, k->ContentSize);
    fileXioClose(fd);

    memset(k->Kbit, 0xAA, 16);
    memset(k->Kc, 0xBB, 16);

    k->bitTable.HeaderSize = sizeof(KELFHeader) + 8 + 16 + 16 + 8 + 16 + 16 + 8 + 8;
    k->bitTable.BlockCount = 2;
    k->bitTable.Blocks[0].Size = 0x20;
    k->bitTable.Blocks[0].Flags = BIT_BLOCK_SIGNED | BIT_BLOCK_ENCRYPTED;
    memset(k->bitTable.Blocks[0].Signature, 0, 8);

    for (int j = 0; j < 0x20; j += 8)
        xor_bit(&k->Content[j], k->bitTable.Blocks[0].Signature, k->bitTable.Blocks[0].Signature, 8);

    uint8_t sigkey[16];
    memcpy(sigkey, k->ks.SignatureMasterKey, 8);
    memcpy(sigkey+8, k->ks.SignatureHashKey, 8);
    TdesEncrypt(k->bitTable.Blocks[0].Signature, k->bitTable.Blocks[0].Signature, 8, sigkey, 2, MG_IV_NULL);
    TdesEncrypt(k->Content, k->Content, 0x20, k->Kc, 2, k->ks.ContentIV);

    k->bitTable.Blocks[1].Size = k->ContentSize - 0x20;
    k->bitTable.Blocks[1].Flags = 0;
    memset(k->bitTable.Blocks[1].Signature, 0, 8);
    return 0;
}

int kelf_save_kelf(KelfCtx *k, const char *path) {
    KELFHeader hdr;
    static uint8_t PSX_USER[] = {0x01,0x03,0x00,0x04,0x00,0x02,0x00,0x4A,0x00,0x07,0x01,0x00,0x00,0x00,0x01,0x78};
    memcpy(hdr.UserDefined, PSX_USER, 16);
    hdr.ContentSize = k->ContentSize;
    hdr.HeaderSize = sizeof(KELFHeader) + 8 + 16 + 16 + 8 + 16 + 16 + 8 + 8;
    hdr.SystemType = SYSTEM_TYPE_PSX;
    hdr.ApplicationType = 1;
    hdr.Flags = 0x22C;
    hdr.BitCount = 0;
    hdr.MGZones = 0xFF;

    uint8_t hsig[8], btsig[8], rsig[8];
    GetHeaderSignature(k, &hdr, hsig);
    GetBitTableSignature(k, btsig);
    GetRootSignature(k, hsig, btsig, rsig);

    int btsz = (k->bitTable.BlockCount * 2 + 1) * 8;
    TdesEncrypt(&k->bitTable, &k->bitTable, btsz, k->Kbit, 2, k->ks.ContentTableIV);

    uint8_t kek[16];
    DeriveKEK(k, &hdr, kek);
    EncryptKeys(k, kek);

    int fd = fileXioOpen(path, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fd < 0) return -1;
    fileXioWrite(fd, &hdr, sizeof(hdr));
    fileXioWrite(fd, hsig, 8);
    fileXioWrite(fd, k->Kbit, 16);
    fileXioWrite(fd, k->Kc, 16);
    fileXioWrite(fd, &k->bitTable, btsz);
    fileXioWrite(fd, btsig, 8);
    fileXioWrite(fd, rsig, 8);
    fileXioWrite(fd, k->Content, k->ContentSize);
    fileXioClose(fd);
    return 0;
}

int kelf_sign_elf(const char *elf_path, const char *kelf_path) {
    KelfCtx k;
    memset(&k, 0, sizeof(k));
    int r = KeyStore_LoadMemory(&k.ks, (const char*)ps2keys_dat, ps2keys_dat_len);
    if (r != 0) return r;
    r = kelf_load_content(&k, elf_path);
    if (r != 0) return r;
    r = kelf_save_kelf(&k, kelf_path);
    if (k.Content) free(k.Content);
    return r;
}

int kelf_sign_embedded_opl(const char *kelf_out_path) {
    /* Write embedded OPL-Launcher to temp location */
    const char *tmp = "hdd0:__common/OPL-TMP.ELF";
    int fd = fileXioOpen(tmp, O_WRONLY|O_CREAT|O_TRUNC, 0666);
    if (fd < 0) return -1;
    fileXioWrite(fd, opl_launcher_elf, opl_launcher_elf_len);
    fileXioClose(fd);
    int r = kelf_sign_elf(tmp, kelf_out_path);
    fileXioRemove(tmp);
    return r;
}
