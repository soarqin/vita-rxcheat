#include <vitasdk.h>
#include <taihen.h>

#include "net.h"
#include "mem.h"
#include "util.h"
#include "debug.h"

#define HOOKS_NUM      2

static SceUID hooks[HOOKS_NUM];
static tai_hook_ref_t ref[HOOKS_NUM];

// Generic variables
static char titleid[32];
static uint32_t old_buttons = 0;

static int running = 1;

// Checking buttons startup/closeup
void checkInput() {
    SceCtrlData ctrl;
    sceCtrlPeekBufferPositive(0, &ctrl, 1);
    if (old_buttons == ctrl.buttons) return;
    if ((ctrl.buttons & (SCE_CTRL_LTRIGGER | SCE_CTRL_SELECT)) == (SCE_CTRL_LTRIGGER | SCE_CTRL_SELECT)) {
        net_init();
        debug_init(DEBUG);
        net_kcp_listen(9527);
    }
    if ((ctrl.buttons & (SCE_CTRL_START | SCE_CTRL_SELECT)) == (SCE_CTRL_START | SCE_CTRL_SELECT)) {
        mem_init();
    }
    old_buttons = ctrl.buttons;
}

int scePowerSetUsingWireless_patched(int enable) {
    return TAI_CONTINUE(int, ref[0], 1);
}

int scePowerSetConfigurationMode_patched(int mode) {
    return 0;
}

int rcsvr_main_thread(SceSize args, void *argp) {
    sceKernelDelayThread(5000000);
    while(running) {
        checkInput();
        net_kcp_process();
        static uint64_t next_tick = 0ULL;
        uint64_t curr_tick = util_gettick();
        if (curr_tick >= next_tick) {
            next_tick = curr_tick + 1000ULL;
        }
    }
    net_finish();
    return sceKernelExitDeleteThread(0);
}

void _start() __attribute__((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
    util_init();
    sceAppMgrAppParamGetString(0, 12, titleid, 32);

    hooks[0] = taiHookFunctionImport(&ref[0], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x4D695C1F, scePowerSetUsingWireless_patched);
    hooks[1] = taiHookFunctionImport(&ref[1], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x3CE187B6, scePowerSetConfigurationMode_patched);

    running = 1;
    SceUID thid = sceKernelCreateThread("rcsvr_main_thread", (SceKernelThreadEntry)rcsvr_main_thread, 0x10000100, 0x10000, 0, 0, NULL);
    if (thid >= 0)
        sceKernelStartThread(thid, 0, NULL);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    running = 0;

    int i;
    for (i = 0; i < HOOKS_NUM; i++)
        taiHookRelease(hooks[i], ref[i]);

    return SCE_KERNEL_STOP_SUCCESS;
}
