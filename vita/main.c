#include "net.h"
#include "mem.h"
#include "trophy.h"
#include "cheat.h"
#include "util.h"
#include "debug.h"
#include "blit.h"
#include "font_pgf.h"

#include "../version.h"

#include <vitasdk.h>
#include <taihen.h>
#include <stdarg.h>

#define HOOKS_NUM      7

static SceUID hooks[HOOKS_NUM];
static tai_hook_ref_t ref[HOOKS_NUM];

// Generic variables
static uint32_t old_buttons = 0;

static int running = 1;

#define MSG_MAX (5)
static char show_msg[MSG_MAX][80] = {};
static uint64_t msg_deadline = 0ULL;
static uint64_t last_tick = 0ULL;

static void main_net_init();

void set_show_msg(uint64_t millisec, int count, ...) {
    va_list arg_ptr;
    int i;
    msg_deadline = millisec + util_gettick();
    va_start(arg_ptr, count);
    if (count > MSG_MAX) count = MSG_MAX;
    for (i = 0; i < count; ++i) {
        sceClibStrncpy(show_msg[i], va_arg(arg_ptr, const char*), 79);
        show_msg[i][79] = 0;
    }
    va_end(arg_ptr);
}

void clear_show_msg() {
    msg_deadline = 0;
    show_msg[0][0] = 0;
}

void check_msg_timeout(uint64_t curr_tick) {
    if (msg_deadline > 0 && curr_tick >= msg_deadline) {
        show_msg[0][0] = 0;
        msg_deadline = 0;
    }
}

// Checking buttons startup/closeup
void checkInput() {
    SceCtrlData ctrl;
    sceCtrlPeekBufferPositive(0, &ctrl, 1);
    if (old_buttons == ctrl.buttons) return;
    if ((ctrl.buttons & (SCE_CTRL_START | SCE_CTRL_SELECT)) == (SCE_CTRL_START | SCE_CTRL_SELECT)) {
        trophy_test();
    }
    old_buttons = ctrl.buttons;
}

int scePowerSetUsingWireless_patched(int enable) {
    return TAI_CONTINUE(int, ref[0], 1);
}

int scePowerSetConfigurationMode_patched(int mode) {
    return 0;
}

int sceSysmoduleLoadModule_patched(SceSysmoduleModuleId id) {
    switch(id) {
    case SCE_SYSMODULE_NET:
        if (net_loaded())
            return 0;
        break;
    case SCE_SYSMODULE_PGF:
        if (font_pgf_loaded())
            return 0;
        break;
    default: break;
    }
    int ret = TAI_CONTINUE(int, ref[2], id);
    switch (id) {
    case SCE_SYSMODULE_NP_TROPHY:
        trophy_hook();
        break;
    default: break;
    }
    return ret;
}

int sceSysmoduleUnloadModule_patched(SceSysmoduleModuleId id) {
    switch(id) {
    case SCE_SYSMODULE_NET:
    case SCE_SYSMODULE_PGF:
        return 0;
    case SCE_SYSMODULE_NP_TROPHY:
        trophy_unhook();
        break;
    default: break;
    }
    int ret = TAI_CONTINUE(int, ref[3], id);
    return ret;
}

int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *pParam, int sync) {
    if (msg_deadline) {
        int i;
        blit_set_frame_buf(pParam);
        for (i = 0; i < MSG_MAX; ++i) {
            blit_string_ctr(2 + 18 * i, show_msg[i]);
        }
    }
    return TAI_CONTINUE(int, ref[4], pParam, sync);
}

int sceNetInit_patched(SceNetInitParam *param) {
    if (net_loaded()) return 0;
    return TAI_CONTINUE(int, ref[5], param);
}

int sceNetCtlInit_patched() {
    if (net_loaded()) return 0;
    return TAI_CONTINUE(int, ref[6]);
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
        hooks[5] = taiHookFunctionImport(&ref[5], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0xEB03E265, sceNetInit_patched);
        hooks[6] = taiHookFunctionImport(&ref[6], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x495CA1DB, sceNetCtlInit_patched);
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
    font_pgf_init();
    mem_init();
    blit_set_color(0xffffffff);

    hooks[4] = taiHookFunctionImport(&ref[4], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x7A410B64, sceDisplaySetFrameBuf_patched);

    if (cheat_loaded())
        set_show_msg(15000, 3, "VITA Remote Cheat v" VERSION_STR, "by Soar Qin", "Cheat code loaded, L+R+" CHAR_LEFT "+SELECT for menu");
    else
        set_show_msg(15000, 2, "VITA Remote Cheat v" VERSION_STR, "by Soar Qin");
    while(running) {
        // checkInput();
        uint64_t curr_tick = util_gettick();
        check_msg_timeout(curr_tick);
        net_kcp_process(curr_tick);
        if (curr_tick >= last_tick + 200) {
            mem_lockdata_process();
            cheat_process();
            last_tick = curr_tick;
        }
    }
    return sceKernelExitDeleteThread(0);
}

void _start() __attribute__((weak, alias ("module_start")));
int module_start(SceSize argc, const void *args) {
    sceIoMkdir("ux0:data", 0777);
    sceIoMkdir("ux0:data/rcsvr", 0777);
    sceIoMkdir("ux0:data/rcsvr/cheat", 0777);

    trophy_init();

    hooks[0] = taiHookFunctionImport(&ref[0], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x4D695C1F, scePowerSetUsingWireless_patched);
    hooks[1] = taiHookFunctionImport(&ref[1], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x3CE187B6, scePowerSetConfigurationMode_patched);
    hooks[2] = taiHookFunctionImport(&ref[2], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x79A0160A, sceSysmoduleLoadModule_patched);
    hooks[3] = taiHookFunctionImport(&ref[3], TAI_MAIN_MODULE, TAI_ANY_LIBRARY, 0x31D87805, sceSysmoduleUnloadModule_patched);

    running = 1;
    SceUID thid = sceKernelCreateThread("rcsvr_main_thread", (SceKernelThreadEntry)rcsvr_main_thread, 0x10000100, 0xF000, 0, 0, NULL);
    if (thid >= 0)
        sceKernelStartThread(thid, 0, NULL);

    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize argc, const void *args) {
    running = 0;

    int i;
    for (i = 0; i < HOOKS_NUM; i++)
        if (hooks[i] > 0)
            taiHookRelease(hooks[i], ref[i]);

    font_pgf_finish();
    mem_finish();
    net_finish();
    trophy_finish();
    return SCE_KERNEL_STOP_SUCCESS;
}
