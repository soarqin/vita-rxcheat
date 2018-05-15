#include "ui.h"

#include "blit.h"
#include "font_pgf.h"
#include "util.h"
#include "cheat.h"
#include "trophy.h"
#include "mem.h"
#include "debug.h"

#include "libcheat/libcheat.h"

#include <vitasdk.h>
#include <taihen.h>
#include <stdarg.h>

#define HOOKS_NUM 1

static SceUID hooks[HOOKS_NUM] = {};
static tai_hook_ref_t refs[HOOKS_NUM] = {};

enum {
    LINE_HEIGHT = 18,

    MSG_MAX   = 5,
    MSG_Y_TOP = 2,

    /* menu modes */
    MENU_MODE_CLOSE = 0,
    MENU_MODE_MAIN  = 1,
    MENU_MODE_CHEAT = 2,
    MENU_MODE_TROP  = 3,
    MENU_MODE_ADV   = 4,
    MENU_MODE_MAX   = 5,

    MENU_SCROLL_MAX = 10,
    MENU_X_SELECTOR = 280,
    MENU_X_LEFT     = 300,
    MENU_Y_TOP      = 180,
};

static char show_msg[MSG_MAX][80] = {};
static uint64_t msg_deadline = 0ULL;
static SceUID msgMutex = -1;

static int menu_mode = MENU_MODE_CLOSE;
static int menu_index[MENU_MODE_MAX] = {};
static int menu_top[MENU_MODE_MAX] = {};

uint32_t old_buttons = 0;
uint32_t enter_button = SCE_CTRL_CIRCLE, cancel_button = SCE_CTRL_CROSS;

#define CHEAT_MENU_TRIGGER (SCE_CTRL_L1 | SCE_CTRL_R1 | SCE_CTRL_LEFT | SCE_CTRL_SELECT)

static inline int menu_get_count() {
    switch (menu_mode) {
        case MENU_MODE_MAIN:
            return 3;
        case MENU_MODE_ADV:
            return 5;
        case MENU_MODE_CHEAT:
            return cheat_loaded() ? cheat_get_section_count(cheat_get_handle()) : 0;
        case MENU_MODE_TROP:
            return trophy_get_count();
        default: return 0;
    }
}

static inline void _show_menu(int standalone) {
    if (standalone) blit_setup();
    blit_clear(240, 135, 960 - 480, 544 - 270);
    switch (menu_mode) {
        case MENU_MODE_MAIN: {
            const char *text[3] = {"Cheats", "Trophies", "Advance"};
            int count = menu_get_count();
            blit_string(MENU_X_LEFT, MENU_Y_TOP - LINE_HEIGHT - 10, 0, "Main Menu");
            blit_string(MENU_X_SELECTOR, MENU_Y_TOP + LINE_HEIGHT * menu_index[menu_mode], 0, CHAR_RIGHT);
            for (int i = 0; i < count; ++i) {
                blit_string(MENU_X_LEFT, MENU_Y_TOP + LINE_HEIGHT * i, 0, text[i]);
            }
            break;
        }
        case MENU_MODE_ADV: {
            const char *text[5] = {"Dump Memory", "CPU CLOCKS", "BUS CLOCKS", "GPU CLOCKS", "GPU XBAR CLOCKS"};
            if (mem_is_dumping()) text[0] = "Dump Memory - Dumping...";
            int count = menu_get_count();
            blit_string(MENU_X_LEFT, MENU_Y_TOP - LINE_HEIGHT - 10, 0, "Advance");
            blit_string(MENU_X_SELECTOR, MENU_Y_TOP + LINE_HEIGHT * menu_index[menu_mode], 0, CHAR_RIGHT);
            for (int i = 0; i < count; ++i) {
                blit_string(MENU_X_LEFT, MENU_Y_TOP + LINE_HEIGHT * i, 0, text[i]);
            }
            blit_string(MENU_X_LEFT - 40, MENU_Y_TOP + LINE_HEIGHT * MENU_SCROLL_MAX, 0,
                "CPU/BUS/GPU/XBAR CLOCKS(MHz):");
            blit_stringf(MENU_X_LEFT - 40, MENU_Y_TOP + LINE_HEIGHT * (MENU_SCROLL_MAX + 1), 0,
                "%d / %d / %d / %d",
                scePowerGetArmClockFrequency(),
                scePowerGetBusClockFrequency(),
                scePowerGetGpuClockFrequency(),
                scePowerGetGpuXbarClockFrequency());
            break;
        }
        case MENU_MODE_CHEAT: {
            blit_string(MENU_X_LEFT, MENU_Y_TOP - LINE_HEIGHT - 10, 0, "Cheats");
            if (!cheat_loaded()) {
                blit_string(MENU_X_LEFT, MENU_Y_TOP, 0, "Cheat codes not found!");
                break;
            }
            const cheat_section_t *secs;
            int menu_bot = cheat_get_sections(cheat_get_handle(), &secs);
            int mt = menu_top[menu_mode];
            int mi = menu_index[menu_mode];
            if (menu_bot > mt + MENU_SCROLL_MAX) menu_bot = mt + MENU_SCROLL_MAX;
            blit_string(MENU_X_SELECTOR, MENU_Y_TOP + LINE_HEIGHT * (mi - mt), 0, CHAR_RIGHT);
            for (int i = mt; i < menu_bot; ++i) {
                int y = MENU_Y_TOP + LINE_HEIGHT * (i - mt);
                if (secs[i].enabled)
                    blit_string(MENU_X_LEFT + 2, y, 0, CHAR_CIRCLE);
                blit_string(MENU_X_LEFT + 22, y, 0, secs[i].name);
            }
            break;
        }
        case MENU_MODE_TROP: {
            blit_string(MENU_X_LEFT, MENU_Y_TOP - LINE_HEIGHT - 10, 0, "Trophies");
            switch (trophy_get_status()) {
                case 0:
                    break;
                case 1:
                    blit_string(MENU_X_LEFT, MENU_Y_TOP, 0, "Loading trophies, be patient...");
                    break;
                case 2: {
                    const char *gradeName[5] = {"", "|P|", "|G|", "|S|", "|B|"};
                    const trophy_info *trops;
                    int count = trophy_get_info(&trops);
                    int menu_bot = count;
                    int mt = menu_top[menu_mode];
                    int mi = menu_index[menu_mode];
                    if (menu_bot > mt + MENU_SCROLL_MAX) menu_bot = mt + MENU_SCROLL_MAX;
                    blit_string(MENU_X_SELECTOR, MENU_Y_TOP + LINE_HEIGHT * (mi - mt), 0, CHAR_RIGHT);
                    for (int i = mt; i < menu_bot; ++i) {
                        int y = MENU_Y_TOP + LINE_HEIGHT * (i - menu_top[menu_mode]);
                        const trophy_info *ti = &trops[i];
                        if (ti->unlocked)
                            blit_string(MENU_X_LEFT + 2, y, 0, CHAR_CIRCLE);
                        if(ti->grade) blit_string(MENU_X_LEFT + 22, y, 0, gradeName[ti->grade]);
                        blit_string(MENU_X_LEFT + 44, y, 0, ti->unlocked || !ti->hidden ? ti->name : "[HIDDEN]");
                    }
                    if (mi >= 0 && mi < count) {
                        const trophy_info *ti = &trops[mi];
                        if (ti->unlocked || !ti->hidden) blit_string(MENU_X_LEFT - 40, MENU_Y_TOP + LINE_HEIGHT * (MENU_SCROLL_MAX + 1), 0, ti->desc);
                    }
                    break;
                }
            }
            break;
        }
        default:
            break;
    }
    if (standalone) sceDisplayWaitVblankStart();
}

static void menu_cancel() {
    switch(menu_mode) {
        case MENU_MODE_MAIN:
            util_resume_main_thread();
            menu_mode = MENU_MODE_CLOSE;
            util_unlock_power();
            return;
        case MENU_MODE_CHEAT:
            if (!cheat_loaded()) break;
            mem_force_reload();
            break;
    }
    menu_mode = MENU_MODE_MAIN;
}

static void menu_run() {    
    switch(menu_mode) {
        case MENU_MODE_MAIN: {
            switch(menu_index[menu_mode]) {
                case 0:
                    menu_mode = MENU_MODE_CHEAT;
                    break;
                case 1:
                    trophy_list(NULL, NULL);
                    menu_mode = MENU_MODE_TROP;
                    break;
                case 2:
                    menu_mode = MENU_MODE_ADV;
                    break;
                default:
                    menu_cancel();
                    return;
            }
            break;
        }
        case MENU_MODE_ADV: {
            switch(menu_index[menu_mode]) {
                case 0:
                    if (!mem_is_dumping()) mem_dump();
                    break;
                case 1: {
                    int c = scePowerGetArmClockFrequency();
                    if (scePowerSetArmClockFrequency(c + 1) != 0) {
                        c = 1;
                        while (scePowerSetArmClockFrequency(c) != 0) ++c;
                    }
                    break;
                }
                case 2: {
                    int c = scePowerGetBusClockFrequency();
                    if (scePowerSetBusClockFrequency(c + 1) != 0) {
                        c = 1;
                        while (scePowerSetBusClockFrequency(c) != 0) ++c;
                    }
                    break;
                }
                case 3: {
                    int c = scePowerGetGpuClockFrequency();
                    if (scePowerSetGpuClockFrequency(c + 1) != 0) {
                        c = 1;
                        while (scePowerSetGpuClockFrequency(c) != 0) ++c;
                    }
                    break;
                }
                case 4: {
                    int c = scePowerGetGpuXbarClockFrequency();
                    if (scePowerSetGpuXbarClockFrequency(c + 1) != 0) {
                        c = 1;
                        while (scePowerSetGpuXbarClockFrequency(c) != 0) ++c;
                    }
                    break;
                }
            }
            break;
        }
        case MENU_MODE_CHEAT: {
            if (!cheat_loaded()) break;
            cheat_t *ch = cheat_get_handle();
            int count = cheat_get_section_count(ch);
            if (menu_index[menu_mode] >= 0 && menu_index[menu_mode] < count) {
                cheat_section_toggle(ch, menu_index[menu_mode], !cheat_get_section(ch, menu_index[menu_mode])->enabled);
            }
            break;
        }
        case MENU_MODE_TROP: {
            const trophy_info *trops;
            int count = trophy_get_info(&trops);
            int mi = menu_index[menu_mode];
            if (mi >= 0 && mi < count) {
                const trophy_info *ti = &trops[mi];
                if (!ti->unlocked)
                    trophy_unlock(ti->id, ti->hidden, NULL, NULL);
            }
            break;
        }
    }
}

int sceCtrlPeekBufferPositive2(int port, SceCtrlData *pad_data, int count);

void ui_process(uint64_t tick) {
    static uint64_t last_tick = 0ULL;
    SceCtrlData pad_data;
    if (sceCtrlPeekBufferPositive2(0, &pad_data, 1) > 0) {
        uint32_t cur_button = pad_data.buttons;
        if (old_buttons == cur_button) {
            if (tick < last_tick + 125ULL) return;
        } else {
            old_buttons = cur_button;
        }
        last_tick = tick;
        switch (menu_mode) {
        case 0:
            if ((old_buttons & CHEAT_MENU_TRIGGER) == CHEAT_MENU_TRIGGER) {
                util_lock_power();
                util_pause_main_thread();
                menu_mode = MENU_MODE_MAIN;
            }
            break;
        default:
            if ((old_buttons & cancel_button) == cancel_button) {
                menu_cancel();
                break;
            }
            if ((old_buttons & enter_button) == enter_button) {
                menu_run();
                break;
            }
            if ((old_buttons & SCE_CTRL_UP) == SCE_CTRL_UP) {
                int count = menu_get_count();
                if (count <= 0) break;
                if (--menu_index[menu_mode] < 0) {
                    menu_index[menu_mode] = count - 1;
                }
                if (menu_index[menu_mode] < menu_top[menu_mode]) menu_top[menu_mode] = menu_index[menu_mode];
                else if (menu_index[menu_mode] >= menu_top[menu_mode] + MENU_SCROLL_MAX) menu_top[menu_mode] = menu_index[menu_mode] + 1 - MENU_SCROLL_MAX;
                break;
            }
            if ((old_buttons & SCE_CTRL_DOWN) == SCE_CTRL_DOWN) {
                int count = menu_get_count();
                if (count <= 0) break;
                if (++menu_index[menu_mode] >= count) {
                    menu_index[menu_mode] = 0;
                }
                if (menu_index[menu_mode] < menu_top[menu_mode]) menu_top[menu_mode] = menu_index[menu_mode];
                else if (menu_index[menu_mode] >= menu_top[menu_mode] + MENU_SCROLL_MAX) menu_top[menu_mode] = menu_index[menu_mode] + 1 - MENU_SCROLL_MAX;
                break;
            }
            break;
        }
    }
    if (menu_mode == MENU_MODE_CLOSE) return;
    sceKernelLockMutex(msgMutex, 1, NULL);
    _show_menu(1);
    sceKernelUnlockMutex(msgMutex, 1);
}

int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *param, int sync) {
    if (refs[0] == 0) return -1;
    if (msg_deadline || menu_mode != MENU_MODE_CLOSE) {
        sceKernelLockMutex(msgMutex, 1, NULL);
        if (blit_set_frame_buf(param) < 0)
            return TAI_CONTINUE(int, refs[0], param, sync);
        if (msg_deadline) {
            for (int i = 0; i < MSG_MAX && show_msg[i][0] != 0; ++i) {
                blit_string_ctr(MSG_Y_TOP + LINE_HEIGHT * i, 1, show_msg[i]);
            }
        }
        if (menu_mode != MENU_MODE_CLOSE) _show_menu(0);
        sceKernelUnlockMutex(msgMutex, 1);
    }
    return TAI_CONTINUE(int, refs[0], param, sync);
}

void ui_init() {
    int but;
    sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &but);
    if (but == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE) {
        enter_button = SCE_CTRL_CIRCLE;
        cancel_button = SCE_CTRL_CROSS;
    } else {
        enter_button = SCE_CTRL_CROSS;
        cancel_button = SCE_CTRL_CIRCLE;
    }
    msgMutex = sceKernelCreateMutex("rcsvr_msg_mutex", 0, 0, 0);
    font_pgf_init();
    blit_set_color(0xffffffff);

    hooks[0] = taiHookFunctionImport(&refs[0], TAI_MAIN_MODULE, 0x4FAACD11, 0x7A410B64, sceDisplaySetFrameBuf_patched);
}

void ui_finish() {
    int i;
    for (i = 0; i < HOOKS_NUM; i++)
        if (hooks[i] > 0)
            taiHookRelease(hooks[i], refs[i]);

    font_pgf_finish();
    if (msgMutex >= 0) {
        sceKernelDeleteMutex(msgMutex);
        msgMutex = -1;
    }
}

void ui_set_show_msg(uint64_t millisec, int count, ...) {
    va_list arg_ptr;
    int i;
    sceKernelLockMutex(msgMutex, 1, NULL);
    msg_deadline = millisec + util_gettick();
    va_start(arg_ptr, count);
    if (count > MSG_MAX) count = MSG_MAX;
    for (i = 0; i < count; ++i) {
        sceClibStrncpy(show_msg[i], va_arg(arg_ptr, const char*), 79);
        show_msg[i][79] = 0;
    }
    va_end(arg_ptr);
    sceKernelUnlockMutex(msgMutex, 1);
}

void ui_clear_show_msg() {
    msg_deadline = 0;
    show_msg[0][0] = 0;
}

void ui_check_msg_timeout(uint64_t curr_tick) {
    if (msg_deadline > 0 && curr_tick >= msg_deadline) {
        show_msg[0][0] = 0;
        msg_deadline = 0;
    }
}
