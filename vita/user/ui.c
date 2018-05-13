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

#define HOOKS_NUM 9

static SceUID hooks[HOOKS_NUM] = {};
static tai_hook_ref_t ref[HOOKS_NUM] = {};

enum {
    LINE_HEIGHT = 18,

    MSG_MAX   = 5,
    MSG_Y_TOP = 2,

    /* menu modes */
    MENU_MODE_CLOSE = 0,
    MENU_MODE_MAIN  = 1,
    MENU_MODE_CHEAT = 2,
    MENU_MODE_TROP  = 3,
    MENU_MODE_MAX   = 4,

    MENU_SCROLL_MAX = 10,
    MENU_X_SELECTOR = 280,
    MENU_X_LEFT     = 300,
    MENU_Y_TOP      = 180,
};

static char show_msg[MSG_MAX][80] = {};
static uint64_t msg_deadline = 0ULL;
static SceUID msgMutex = -1;

static int menu_mode = 0;
static int menu_index[MENU_MODE_MAX] = {};
static int menu_top[MENU_MODE_MAX] = {};

uint32_t old_buttons = 0;
uint32_t enter_button = SCE_CTRL_CIRCLE, cancel_button = SCE_CTRL_CROSS;

#define CHEAT_MENU_TRIGGER (SCE_CTRL_L1 | SCE_CTRL_R1 | SCE_CTRL_LEFT | SCE_CTRL_SELECT)

static inline int menu_get_count() {
    switch (menu_mode) {
        case MENU_MODE_MAIN:
            return cheat_loaded() ? 3 : 2;
        case MENU_MODE_CHEAT:
            return cheat_get_section_count(cheat_get_handle());
        case MENU_MODE_TROP:
            return trophy_get_count();
        default: return 0;
    }
}

static inline void _show_menu() {
    if (menu_mode == MENU_MODE_CLOSE) return;
    blit_clear(240, 135, 960 - 480, 544 - 270);
    switch (menu_mode) {
        case MENU_MODE_MAIN: {
            const char *text1[3] = {"Cheats", "Trophies", "Close"};
            const char *text2[2] = {"Trophies", "Close"};
            const char **text;
            int count;
            if (cheat_loaded()) {
                count = 3;
                text = text1;
            } else {
                count = 2;
                text = text2;
            }
            int menu_bot = menu_get_count();
            if (menu_bot > menu_top[menu_mode] + MENU_SCROLL_MAX) menu_bot = menu_top[menu_mode] + MENU_SCROLL_MAX;
            blit_string(MENU_X_LEFT, MENU_Y_TOP - LINE_HEIGHT - 10, "Main Menu");
            blit_string(MENU_X_SELECTOR, MENU_Y_TOP + LINE_HEIGHT * menu_index[menu_mode], CHAR_RIGHT);
            for (int i = 0; i < count; ++i) {
                blit_string(MENU_X_LEFT, MENU_Y_TOP + LINE_HEIGHT * i, text[i]);
            }
            break;
        }
        case MENU_MODE_CHEAT: {
            const cheat_section_t *secs;
            int menu_bot = cheat_get_sections(cheat_get_handle(), &secs);
            int mt = menu_top[menu_mode];
            int mi = menu_index[menu_mode];
            if (menu_bot > mt + MENU_SCROLL_MAX) menu_bot = mt + MENU_SCROLL_MAX;
            blit_string(MENU_X_LEFT, MENU_Y_TOP - LINE_HEIGHT - 10, "Cheats");
            blit_string(MENU_X_SELECTOR, MENU_Y_TOP + LINE_HEIGHT * (mi - mt), CHAR_RIGHT);
            for (int i = mt; i < menu_bot; ++i) {
                int y = MENU_Y_TOP + LINE_HEIGHT * (i - mt);
                if (secs[i].enabled)
                    blit_string(MENU_X_LEFT + 2, y, CHAR_CIRCLE);
                blit_string(MENU_X_LEFT + 22, y, secs[i].name);
            }
            break;
        }
        case MENU_MODE_TROP: {
            blit_string(MENU_X_LEFT, MENU_Y_TOP - LINE_HEIGHT - 10, "Trophies");
            switch (trophy_get_status()) {
                case 0:
                    break;
                case 1:
                    blit_string(MENU_X_LEFT, MENU_Y_TOP, "Loading trophies, be patient...");
                    break;
                case 2: {
                    const char *gradeName[5] = {"", "|P|", "|G|", "|S|", "|B|"};
                    const trophy_info *trops;
                    int count = trophy_get_info(&trops);
                    int menu_bot = count;
                    int mt = menu_top[menu_mode];
                    int mi = menu_index[menu_mode];
                    if (menu_bot > mt + MENU_SCROLL_MAX) menu_bot = mt + MENU_SCROLL_MAX;
                    blit_string(MENU_X_SELECTOR, MENU_Y_TOP + LINE_HEIGHT * (mi - mt), CHAR_RIGHT);
                    for (int i = mt; i < menu_bot; ++i) {
                        int y = MENU_Y_TOP + LINE_HEIGHT * (i - menu_top[menu_mode]);
                        const trophy_info *ti = &trops[i];
                        if (ti->unlocked)
                            blit_string(MENU_X_LEFT + 2, y, CHAR_CIRCLE);
                        if(ti->grade) blit_string(MENU_X_LEFT + 22, y, gradeName[ti->grade]);
                        blit_string(MENU_X_LEFT + 44, y, ti->unlocked || !ti->hidden ? ti->name : "[HIDDEN]");
                    }
                    if (mi >= 0 && mi < count) {
                        const trophy_info *ti = &trops[mi];
                        if (ti->unlocked || !ti->hidden) blit_string(MENU_X_LEFT - 40, MENU_Y_TOP + LINE_HEIGHT * (MENU_SCROLL_MAX + 1), ti->desc);
                    }
                    break;
                }
            }
            break;
        }
        default:
            break;
    }
}

static void menu_cancel() {
    switch(menu_mode) {
        case MENU_MODE_MAIN:
            menu_mode = MENU_MODE_CLOSE;
            return;
        case MENU_MODE_CHEAT:
            mem_force_reload();
            break;
    }
    menu_mode = MENU_MODE_MAIN;
}

static void menu_run() {    
    switch(menu_mode) {
        case MENU_MODE_MAIN: {
            int idx = menu_index[menu_mode];
            if (!cheat_loaded()) ++idx;
            switch(idx) {
                case 0:
                    menu_mode = MENU_MODE_CHEAT;
                    log_debug("Enter cheat menu\n");
                    break;
                case 1:
                    trophy_list(NULL, NULL);
                    menu_mode = MENU_MODE_TROP;
                    log_debug("Enter trophy menu\n");
                    break;
                case 2:
                    menu_cancel();
                    return;
            }
            break;
        }
        case MENU_MODE_CHEAT: {
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

static int check_input(SceCtrlData *pad_data, int pad2) {
    uint32_t cur_button = pad_data->buttons;
    if (!pad2) {
        if (cur_button & SCE_CTRL_LTRIGGER)
            cur_button = (cur_button & ~SCE_CTRL_LTRIGGER) | SCE_CTRL_L1;
        if (cur_button & SCE_CTRL_RTRIGGER)
            cur_button = (cur_button & ~SCE_CTRL_RTRIGGER) | SCE_CTRL_R1;
    }
    if (old_buttons == cur_button) return 0;
    old_buttons = cur_button;
    switch (menu_mode) {
    case 0:
        if ((old_buttons & CHEAT_MENU_TRIGGER) == CHEAT_MENU_TRIGGER) {
            menu_mode = 0;
            menu_mode = MENU_MODE_MAIN;
            break;
        }
        return 0;
    default:
        if ((old_buttons & enter_button) == enter_button) {
            menu_run();
        }
        if ((old_buttons & cancel_button) == cancel_button) {
            menu_cancel();
        }
        if ((old_buttons & SCE_CTRL_UP) == SCE_CTRL_UP) {
            int count = menu_get_count();
            if (--menu_index[menu_mode] < 0) {
                menu_index[menu_mode] = count - 1;
            }
            if (menu_index[menu_mode] < menu_top[menu_mode]) menu_top[menu_mode] = menu_index[menu_mode];
            else if (menu_index[menu_mode] >= menu_top[menu_mode] + MENU_SCROLL_MAX) menu_top[menu_mode] = menu_index[menu_mode] + 1 - MENU_SCROLL_MAX;
        }
        if ((old_buttons & SCE_CTRL_DOWN) == SCE_CTRL_DOWN) {
            int count = menu_get_count();
            if (++menu_index[menu_mode] >= count) {
                menu_index[menu_mode] = 0;
            }
            if (menu_index[menu_mode] < menu_top[menu_mode]) menu_top[menu_mode] = menu_index[menu_mode];
            else if (menu_index[menu_mode] >= menu_top[menu_mode] + MENU_SCROLL_MAX) menu_top[menu_mode] = menu_index[menu_mode] + 1 - MENU_SCROLL_MAX;
        }
        break;
    }
    return 1;
}

int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *param, int sync) {
    int i;
    if (msg_deadline || menu_mode) {
        if (blit_set_frame_buf(param) < 0)
            return TAI_CONTINUE(int, ref[0], param, sync);
    }
    if (msg_deadline) {
        sceKernelLockMutex(msgMutex, 1, NULL);
        for (i = 0; i < MSG_MAX && show_msg[i][0] != 0; ++i) {
            blit_string_ctr(MSG_Y_TOP + LINE_HEIGHT * i, show_msg[i]);
        }
        sceKernelUnlockMutex(msgMutex, 1);
    }
    _show_menu();
    return TAI_CONTINUE(int, ref[0], param, sync);
}

int sceCtrlPeekBufferPositive_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, ref[1], port, pad_data, count);
    if (ret <= 0) return ret;
    check_input(&pad_data[ret - 1], 0);
    return ret;
}

int sceCtrlPeekBufferPositive2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, ref[2], port, pad_data, count);
    if (ret <= 0) return ret;
    check_input(&pad_data[ret - 1], 1);
    return ret;
}

int sceCtrlPeekBufferPositiveExt_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, ref[3], port, pad_data, count);
    if (ret <= 0) return ret;
    check_input(&pad_data[ret - 1], 0);
    return ret;
}

int sceCtrlPeekBufferPositiveExt2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, ref[4], port, pad_data, count);
    if (ret <= 0) return ret;
    check_input(&pad_data[ret - 1], 1);
    return ret;
}

int sceCtrlReadBufferPositive_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, ref[5], port, pad_data, count);
    if (ret <= 0) return ret;
    check_input(&pad_data[ret - 1], 0);
    return ret;
}

int sceCtrlReadBufferPositive2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, ref[6], port, pad_data, count);
    if (ret <= 0) return ret;
    check_input(&pad_data[ret - 1], 1);
    return ret;
}

int sceCtrlReadBufferPositiveExt_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, ref[7], port, pad_data, count);
    if (ret <= 0) return ret;
    check_input(&pad_data[ret - 1], 0);
    return ret;
}

int sceCtrlReadBufferPositiveExt2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, ref[8], port, pad_data, count);
    if (ret <= 0) return ret;
    check_input(&pad_data[ret - 1], 1);
    return ret;
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

    hooks[0] = taiHookFunctionImport(&ref[0], TAI_MAIN_MODULE, 0x4FAACD11, 0x7A410B64, sceDisplaySetFrameBuf_patched);
    hooks[1] = taiHookFunctionImport(&ref[1], TAI_MAIN_MODULE, 0xD197E3C7, 0xA9C3CED6, sceCtrlPeekBufferPositive_patched);
    hooks[2] = taiHookFunctionImport(&ref[2], TAI_MAIN_MODULE, 0xD197E3C7, 0x15F81E8C, sceCtrlPeekBufferPositive2_patched);
    hooks[3] = taiHookFunctionImport(&ref[3], TAI_MAIN_MODULE, 0xD197E3C7, 0xA59454D3, sceCtrlPeekBufferPositiveExt_patched);
    hooks[4] = taiHookFunctionImport(&ref[4], TAI_MAIN_MODULE, 0xD197E3C7, 0x860BF292, sceCtrlPeekBufferPositiveExt2_patched);
    hooks[5] = taiHookFunctionImport(&ref[5], TAI_MAIN_MODULE, 0xD197E3C7, 0x67E7AB83, sceCtrlReadBufferPositive_patched);
    hooks[6] = taiHookFunctionImport(&ref[6], TAI_MAIN_MODULE, 0xD197E3C7, 0xC4226A3E, sceCtrlReadBufferPositive2_patched);
    hooks[7] = taiHookFunctionImport(&ref[7], TAI_MAIN_MODULE, 0xD197E3C7, 0xE2D99296, sceCtrlReadBufferPositiveExt_patched);
    hooks[8] = taiHookFunctionImport(&ref[8], TAI_MAIN_MODULE, 0xD197E3C7, 0xA7178860, sceCtrlReadBufferPositiveExt2_patched);
}

void ui_finish() {
    int i;
    for (i = 0; i < HOOKS_NUM; i++)
        if (hooks[i] > 0)
            taiHookRelease(hooks[i], ref[i]);

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
