#include "ui.h"

#include "blit.h"
#include "font_pgf.h"
#include "util.h"
#include "cheat.h"
#include "mem.h"

#include "libcheat/libcheat.h"

#include <vitasdk.h>
#include <taihen.h>
#include <stdarg.h>

#define HOOKS_NUM 9

static SceUID hooks[HOOKS_NUM] = {};
static tai_hook_ref_t ref[HOOKS_NUM] = {};

#define LINE_HEIGHT (18)

#define MSG_MAX   (5)
#define MSG_Y_TOP (2)

static char show_msg[MSG_MAX][80] = {};
static uint64_t msg_deadline = 0ULL;

#define MENU_MAX        (10)
#define MENU_SCROLL_MAX (10)
#define MENU_X_SELECTOR (280)
#define MENU_X_LEFT     (300)
#define MENU_Y_TOP      (180)

// 0 - closed  1 - normal menu  2 - cheat code list
static int menu_mode = 0;
static int menu_count = 0;
static int menu_index = 0;
static int menu_top = 0;
static const char menu_items[MENU_MAX][80] = {};

uint32_t old_buttons = 0;
uint32_t enter_button = SCE_CTRL_CIRCLE, cancel_button = SCE_CTRL_CROSS;

#define CHEAT_MENU_TRIGGER (SCE_CTRL_LTRIGGER | SCE_CTRL_RTRIGGER | SCE_CTRL_LEFT | SCE_CTRL_SELECT)

static int check_input(SceCtrlData *pad_data) {
    if (old_buttons == pad_data->buttons) return 0;
    old_buttons = pad_data->buttons;
    switch (menu_mode) {
    case 0:
        if ((old_buttons & CHEAT_MENU_TRIGGER) == CHEAT_MENU_TRIGGER) {
            menu_mode = 2;
            break;
        }
        return 0;
    case 1:
        break;
    case 2:
        if ((old_buttons & enter_button) == enter_button) {
            int count;
            const cheat_section_t *secs;
            cheat_t *ch = cheat_get_handle();
            count = cheat_get_sections(ch, &secs);
            if (menu_index >= 0 && menu_index < count) {
                cheat_section_toggle(ch, menu_index, !secs[menu_index].enabled);
            }
        }
        if ((old_buttons & cancel_button) == cancel_button) {
            menu_mode = 0;
            mem_reload();
        }
        if ((old_buttons & SCE_CTRL_UP) == SCE_CTRL_UP) {
            if (--menu_index < 0) {
                int count;
                const cheat_section_t *secs;
                cheat_t *ch = cheat_get_handle();
                count = cheat_get_sections(ch, &secs);
                menu_index = count - 1;
            }
            if (menu_index < menu_top) menu_top = menu_index;
            else if (menu_index >= menu_top + MENU_MAX) menu_top = menu_index + 1 - MENU_MAX;
        }
        if ((old_buttons & SCE_CTRL_DOWN) == SCE_CTRL_DOWN) {
            int count;
            const cheat_section_t *secs;
            cheat_t *ch = cheat_get_handle();
            count = cheat_get_sections(ch, &secs);
            if (++menu_index >= count) {
                menu_index = 0;
            }
            if (menu_index < menu_top) menu_top = menu_index;
            else if (menu_index >= menu_top + MENU_MAX) menu_top = menu_index + 1 - MENU_MAX;
        }
        break;
    }
    return 1;
}

static inline void ui_draw(const SceDisplayFrameBuf *param) {
    int i;
    if (msg_deadline || menu_mode) {
        if (blit_set_frame_buf(param) < 0) return;
    }
    if (msg_deadline) {
        for (i = 0; i < MSG_MAX && show_msg[i][0] != 0; ++i) {
            blit_string_ctr(MSG_Y_TOP + LINE_HEIGHT * i, show_msg[i]);
        }
    }
    switch (menu_mode) {
        case 1:
            blit_string(MENU_X_SELECTOR, MENU_Y_TOP + LINE_HEIGHT * menu_index, CHAR_RIGHT);
            for (i = 0; i < menu_count; ++i) {
                blit_string(MENU_X_LEFT, MENU_Y_TOP + LINE_HEIGHT * i, menu_items[i]);
            }
            break;
        case 2: {
            int count, menu_bot;
            const cheat_section_t *secs;
            count = cheat_get_sections(cheat_get_handle(), &secs);
            menu_bot = count;
            blit_string(MENU_X_LEFT, MENU_Y_TOP - LINE_HEIGHT - 10, "Cheat Codes");
            if (menu_bot > menu_top + MENU_MAX) menu_bot = menu_top + MENU_MAX;
                blit_string(MENU_X_SELECTOR, MENU_Y_TOP + LINE_HEIGHT * (menu_index - menu_top), CHAR_RIGHT);
            for (i = menu_top; i < menu_bot; ++i) {
                int y = MENU_Y_TOP + LINE_HEIGHT * (i - menu_top);
                if (secs[i].enabled)
                    blit_string(MENU_X_LEFT + 2, y, CHAR_CIRCLE);
                blit_string(MENU_X_LEFT + 22, y, secs[i].name);
            }
            break;
        }
        default:
            break;
    }
}

int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *param, int sync) {
    ui_draw(param);
    return TAI_CONTINUE(int, ref[0], param, sync);
}

int sceCtrlPeekBufferPositive_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, ref[1], port, pad_data, count);
    if (ret <= 0) return ret;
    check_input(&pad_data[ret - 1]);
    return ret;
}

int sceCtrlPeekBufferPositive2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, ref[2], port, pad_data, count);
    if (ret <= 0) return ret;
    check_input(&pad_data[ret - 1]);
    return ret;
}

int sceCtrlPeekBufferPositiveExt_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, ref[3], port, pad_data, count);
    if (ret <= 0) return ret;
    check_input(&pad_data[ret - 1]);
    return ret;
}

int sceCtrlPeekBufferPositiveExt2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, ref[4], port, pad_data, count);
    if (ret <= 0) return ret;
    check_input(&pad_data[ret - 1]);
    return ret;
}

int sceCtrlReadBufferPositive_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, ref[5], port, pad_data, count);
    if (ret <= 0) return ret;
    check_input(&pad_data[ret - 1]);
    return ret;
}

int sceCtrlReadBufferPositive2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, ref[6], port, pad_data, count);
    if (ret <= 0) return ret;
    check_input(&pad_data[ret - 1]);
    return ret;
}

int sceCtrlReadBufferPositiveExt_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, ref[7], port, pad_data, count);
    if (ret <= 0) return ret;
    check_input(&pad_data[ret - 1]);
    return ret;
}

int sceCtrlReadBufferPositiveExt2_patched(int port, SceCtrlData *pad_data, int count) {
    int ret = TAI_CONTINUE(int, ref[8], port, pad_data, count);
    if (ret <= 0) return ret;
    check_input(&pad_data[ret - 1]);
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
}

void ui_set_show_msg(uint64_t millisec, int count, ...) {
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
