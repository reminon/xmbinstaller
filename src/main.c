#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <iopcontrol.h>
#include <sifcmd.h>
#include <loadfile.h>
#include <sbv_patches.h>
#include <iopheap.h>
#define NEWLIB_PORT_AWARE
#include <fileXio_rpc.h>
#include <libpad.h>
#include "storage.h"
#include "bootstrap.h"
#include "game_installer_ui.h"

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    SifInitRpc(0);
    SifIopReset("", 0);
    while (!SifIopSync()) {}
    SifInitRpc(0);
    SifInitIopHeap();
    SifLoadFileInit();
    sbv_patch_enable_lmb();
    sbv_patch_disable_prefix_check();

    storage_diagnostics_t diag;
    storage_initialize(&diag);

    fileXioInit();
    fileXioSetRWBufferSize(128 * 1024);

    padInit(0);
    padPortOpen(0, 0, NULL);

    run_game_installer_ui(NULL);

    return 0;
}
