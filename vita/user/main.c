#include "net.h"
#include "mem.h"
#include "trophy.h"
#include "cheat.h"
#include "util.h"
#include "debug.h"
#include "ui.h"
#include "font_pgf.h"

#include "../version.h"

#include <vitasdk.h>
#include <taihen.h>

#define HOOKS_NUM      6

static SceUID hooks[HOOKS_NUM] = {};
static tai_hook_ref_t refs[HOOKS_NUM] = {};

static int running = 1;

static uint64_t last_tick = 0ULL;

static void main_net_init();

int scePowerSetUsingWireless_patched(int enable) {
    if (refs[0] <= 0) return -1;
    return TAI_CONTINUE(int, refs[0], 1);
}

int scePowerSetConfigurationMode_patched(int mode) {
    return 0;
}

int sceSysmoduleLoadModule_patched(SceSysmoduleModuleId id) {
    if (refs[2] <= 0) return -1;
    switch(id) {
    case SCE_SYSMODULE_NET:
        if (net_loaded())
            return 0;
        break;
    case SCE_SYSMODULE_PGF:
        if (font_pgf_loaded())
            return 0;
        break;
    case SCE_SYSMODULE_NP:
    case SCE_SYSMODULE_NP_TROPHY:
        return 0;
    default: break;
    }
    int ret = TAI_CONTINUE(int, refs[2], id);
    return ret;
}

int sceSysmoduleUnloadModule_patched(SceSysmoduleModuleId id) {
    if (refs[3] <= 0) return -1;
    switch(id) {
    case SCE_SYSMODULE_NET:
    case SCE_SYSMODULE_PGF:
        return 0;
    case SCE_SYSMODULE_NP:
    case SCE_SYSMODULE_NP_TROPHY:
        return 0;
    default: break;
    }
    int ret = TAI_CONTINUE(int, refs[3], id);
    return ret;
}

int sceNetInit_patched(SceNetInitParam *param) {
    if (refs[4] <= 0) return -1;
    if (net_loaded()) return 0;
    return TAI_CONTINUE(int, refs[4], param);
}

int sceNetCtlInit_patched() {
    if (refs[5] <= 0) return -1;
    if (net_loaded()) return 0;
    return TAI_CONTINUE(int, refs[5]);
}

static void main_cheat_load() {
    char titleid[16];
    sceAppMgrAppParamGetString(0, 12, titleid, 16);
    cheat_load(titleid);
}

static void main_net_init() {
    if (net_init() == 0) {
        debug_init(TRACE); // DEBUG
        net_kcp_listen(9527);
        hooks[4] = taiHookFunctionImport(&refs[4], TAI_MAIN_MODULE, 0x6BF8B2A2, 0xEB03E265, sceNetInit_patched);
        hooks[5] = taiHookFunctionImport(&refs[5], TAI_MAIN_MODULE, 0x6BF8B2A2, 0x495CA1DB, sceNetCtlInit_patched);
    }
}

int rcsvr_main_thread(SceSize args, void *argp) {
    sceKernelDelayThread(10000000);

    util_init();
    main_cheat_load();
    main_net_init();
#ifdef RCSVR_DEBUG
    SceKernelFreeMemorySizeInfo fmsi;
    fmsi.size = sizeof(fmsi);
    sceKernelGetFreeMemorySize(&fmsi);
    log_trace("Free memory: %X %X %X\n", fmsi.size_user, fmsi.size_phycont, fmsi.size_cdram);
#endif
    ui_init();
    mem_init();

    if (cheat_loaded())
        ui_set_show_msg(15000, 3, PLUGIN_NAME " v" VERSION_STR, "by " PLUGIN_AUTHOR, "Cheat code loaded, L+R+" CHAR_LEFT "+SELECT for menu");
    else
        ui_set_show_msg(15000, 2, PLUGIN_NAME " v" VERSION_STR, "by " PLUGIN_AUTHOR);
    while(running) {
        // checkInput();
        uint64_t curr_tick = util_gettick();
        ui_check_msg_timeout(curr_tick);
        net_kcp_process(curr_tick);
        if (curr_tick >= last_tick + 200) {
            mem_lockdata_process();
            cheat_process();
            last_tick = curr_tick;
        }
    }
    mem_finish();
    ui_finish();
    net_finish();
    cheat_free();

    return sceKernelExitDeleteThread(0);
}

void _start() __attribute__((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
    sceIoMkdir("ux0:data", 0777);
    sceIoMkdir("ux0:data/rcsvr", 0777);
    sceIoMkdir("ux0:data/rcsvr/cheat", 0777);

    trophy_init();

    hooks[0] = taiHookFunctionImport(&refs[0], TAI_MAIN_MODULE, 0x1082DA7F, 0x4D695C1F, scePowerSetUsingWireless_patched);
    hooks[1] = taiHookFunctionImport(&refs[1], TAI_MAIN_MODULE, 0x1082DA7F, 0x3CE187B6, scePowerSetConfigurationMode_patched);
    hooks[2] = taiHookFunctionImport(&refs[2], TAI_MAIN_MODULE, 0x03FCF19D, 0x79A0160A, sceSysmoduleLoadModule_patched);
    hooks[3] = taiHookFunctionImport(&refs[3], TAI_MAIN_MODULE, 0x03FCF19D, 0x31D87805, sceSysmoduleUnloadModule_patched);

    running = 1;
    SceUID thid = sceKernelCreateThread("rcsvr_main_thread", (SceKernelThreadEntry)rcsvr_main_thread, 0x10000100, 0xF000, 0, 0, NULL);
    if (thid >= 0)
        sceKernelStartThread(thid, 0, NULL);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    int i;

    running = 0;

    for (i = 0; i < HOOKS_NUM; i++)
        if (hooks[i] > 0)
            taiHookRelease(hooks[i], refs[i]);

    trophy_finish();
    return SCE_KERNEL_STOP_SUCCESS;
}
