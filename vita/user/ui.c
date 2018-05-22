#include "ui.h"

#include "config.h"
#include "lang.h"
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
    MENU_MODE_COUNT = 5,

    MENU_SCROLL_MAX = 10,
    MENU_X_LEFT     = -180,
    MENU_X_SELECTOR = MENU_X_LEFT-20,
    MENU_Y_TOP      = -90,
};

static char show_msg[MSG_MAX][80] = {};
static uint64_t msg_deadline = 0ULL;
static SceUID drawMutex = -1;

static int menu_mode = MENU_MODE_CLOSE;
static int menu_index[MENU_MODE_COUNT] = {};
static int menu_top[MENU_MODE_COUNT] = {};

uint32_t old_buttons = 0;
uint32_t enter_button = SCE_CTRL_CIRCLE, cancel_button = SCE_CTRL_CROSS;

#define CHEAT_MENU_TRIGGER (SCE_CTRL_L1 | SCE_CTRL_R1 | SCE_CTRL_LEFT | SCE_CTRL_SELECT)

static inline int menu_get_count() {
    switch (menu_mode) {
        case MENU_MODE_MAIN:
            return 3;
        case MENU_MODE_ADV:
            return 3;
        case MENU_MODE_CHEAT:
            return cheat_loaded() ? cheat_get_section_count(cheat_get_handle()) : 0;
        case MENU_MODE_TROP:
            return trophy_get_count();
        default: return 0;
    }
}

static inline void _show_menu(int standalone) {
    if (standalone) blit_setup();
    if (msg_deadline) {
        for (int i = 0; i < MSG_MAX && show_msg[i][0] != 0; ++i) {
            blit_string_ctr(MSG_Y_TOP + LINE_HEIGHT * i, 1, show_msg[i]);
        }
    }
    if (menu_mode == MENU_MODE_CLOSE) return;
    int centerx = blit_pwidth / 2, centery = blit_pheight / 2;
    if (centerx >= 280 && centery >= 145)
        blit_clear(centerx - 280, centery - 145, 560, 290);
    else
        blit_clear_all();
    switch (menu_mode) {
        case MENU_MODE_MAIN: {
            const char *text[3] = {LS(CHEATS), LS(TROPHIES), LS(ADVANCE)};
            int count = menu_get_count();
            blit_string(centerx + MENU_X_LEFT, centery + MENU_Y_TOP - LINE_HEIGHT - 10, 0, LS(MAINMENU));
            blit_string(centerx + MENU_X_SELECTOR, centery + MENU_Y_TOP + LINE_HEIGHT * menu_index[menu_mode], 0, CHAR_RIGHT);
            for (int i = 0; i < count; ++i) {
                blit_string(centerx + MENU_X_LEFT, centery + MENU_Y_TOP + LINE_HEIGHT * i, 0, text[i]);
            }
            break;
        }
        case MENU_MODE_ADV: {
            const char *text[3] = {LS(LANGUAGE), LS(DUMP), LS(CHCLOCKS)};
            if (mem_is_dumping()) text[1] = LS(DUMPING);
            int count = menu_get_count();
            blit_string(centerx + MENU_X_LEFT, centery + MENU_Y_TOP - LINE_HEIGHT - 10, 0, LS(ADVANCE));
            blit_string(centerx + MENU_X_SELECTOR, centery + MENU_Y_TOP + LINE_HEIGHT * menu_index[menu_mode], 0, CHAR_RIGHT);
            for (int i = 0; i < count; ++i) {
                blit_string(centerx + MENU_X_LEFT, centery + MENU_Y_TOP + LINE_HEIGHT * i, 0, text[i]);
            }
            char curlang[64];
            sceClibSnprintf(curlang, 64, LS(CURLANG), lang_get_name(lang_get_index()));
            blit_string(centerx + MENU_X_LEFT - 40, centery + MENU_Y_TOP + LINE_HEIGHT * (MENU_SCROLL_MAX - 1), 0, curlang);
            blit_string(centerx + MENU_X_LEFT - 40, centery + MENU_Y_TOP + LINE_HEIGHT * MENU_SCROLL_MAX, 0, LS(CLOCKS));
            blit_stringf(centerx + MENU_X_LEFT - 40, centery + MENU_Y_TOP + LINE_HEIGHT * (MENU_SCROLL_MAX + 1), 0,
                "%d / %d / %d / %d",
                scePowerGetArmClockFrequency(),
                scePowerGetBusClockFrequency(),
                scePowerGetGpuClockFrequency(),
                scePowerGetGpuXbarClockFrequency());
            break;
        }
        case MENU_MODE_CHEAT: {
            blit_string(centerx + MENU_X_LEFT, centery + MENU_Y_TOP - LINE_HEIGHT - 10, 0, LS(CHEATS));
            if (!cheat_loaded()) {
                blit_string(centerx + MENU_X_LEFT, centery + MENU_Y_TOP, 0, LS(CHEAT_NOT_FOUND));
                break;
            }
            const cheat_section_t *secs;
            int menu_bot = cheat_get_sections(cheat_get_handle(), &secs);
            int mt = menu_top[menu_mode];
            int mi = menu_index[menu_mode];
            if (menu_bot > mt + MENU_SCROLL_MAX) menu_bot = mt + MENU_SCROLL_MAX;
            blit_string(centerx + MENU_X_SELECTOR, centery + MENU_Y_TOP + LINE_HEIGHT * (mi - mt), 0, CHAR_RIGHT);
            for (int i = mt; i < menu_bot; ++i) {
                int y = centery + MENU_Y_TOP + LINE_HEIGHT * (i - mt);
                if (secs[i].status & 1)
                    blit_string(centerx + MENU_X_LEFT + 2, y, 0, CHAR_CIRCLE);
                blit_string(centerx + MENU_X_LEFT + 22, y, 0, secs[i].name);
            }
            break;
        }
        case MENU_MODE_TROP: {
            blit_string(centerx + MENU_X_LEFT, centery + MENU_Y_TOP - LINE_HEIGHT - 10, 0, LS(TROPHIES));
            switch (trophy_get_status()) {
                case 0:
                    break;
                case 1:
                    blit_string(centerx + MENU_X_LEFT, centery + MENU_Y_TOP, 0, LS(TROPHIES_READING));
                    break;
                case 2: {
                    const char *gradeName[5] = {"", LS(GRADE_P), LS(GRADE_G), LS(GRADE_S), LS(GRADE_B)};
                    const trophy_info *trops;
                    int count = trophy_get_info(&trops);
                    int menu_bot = count;
                    int mt = menu_top[menu_mode];
                    int mi = menu_index[menu_mode];
                    if (menu_bot > mt + MENU_SCROLL_MAX) menu_bot = mt + MENU_SCROLL_MAX;
                    blit_string(centerx + MENU_X_SELECTOR, centery + MENU_Y_TOP + LINE_HEIGHT * (mi - mt), 0, CHAR_RIGHT);
                    for (int i = mt; i < menu_bot; ++i) {
                        int y = centery + MENU_Y_TOP + LINE_HEIGHT * (i - menu_top[menu_mode]);
                        const trophy_info *ti = &trops[i];
                        if (ti->unlocked)
                            blit_string(centerx + MENU_X_LEFT + 2, y, 0, CHAR_CIRCLE);
                        if(ti->grade) blit_string(centerx + MENU_X_LEFT + 26, y, 0, gradeName[ti->grade]);
                        blit_string(centerx + MENU_X_LEFT + 54, y, 0, ti->unlocked || !ti->hidden ? ti->name : LS(HIDDEN));
                    }
                    if (mi >= 0 && mi < count) {
                        const trophy_info *ti = &trops[mi];
                        if (ti->unlocked || !ti->hidden) blit_string_ctr(centery + MENU_Y_TOP + LINE_HEIGHT * (MENU_SCROLL_MAX + 1), 0, ti->desc);
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

// 0 - decrease   1 - increase
static inline void language_toggle(int direction) {
    int count = lang_get_count();
    if (count == 0) return;
    int n = lang_get_index();
    n = (direction ? (n + 1) : (n + count - 1)) % count;
    lang_set(n);
}

// 0 - decrease   1 - increase
static inline void clocks_toggle(int direction) {
    static int clocks[5][4] = {
        {111, 111, 111, 111},
        {266, 166, 111, 111},
        {333, 222, 111, 111},
        {366, 222, 166, 111},
        {444, 222, 222, 166},
    };
    int c = scePowerGetArmClockFrequency();
    for (int i = 4; i >= 0; --i) {
        if (c >= clocks[i][0]) {
            int n = direction ? ((i + 1) % 5) : ((i + 4) % 5);
            scePowerSetArmClockFrequency(clocks[n][0]);
            scePowerSetBusClockFrequency(clocks[n][1]);
            scePowerSetGpuClockFrequency(clocks[n][2]);
            scePowerSetGpuXbarClockFrequency(clocks[n][3]);
            break;
        }
    }
}

// 0 - CIRCLE pressed  1 - LEFT pressed  2 - RIGHT pressed
static void menu_run(int type) {
    switch(type) {
    case 0:
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
                        language_toggle(1);
                        break;
                    case 1:
                        if (!mem_is_dumping()) mem_dump();
                        break;
                    case 2:
                        clocks_toggle(1);
                        break;
                }
                break;
            }
            case MENU_MODE_CHEAT: {
                if (!cheat_loaded()) break;
                cheat_t *ch = cheat_get_handle();
                int count = cheat_get_section_count(ch);
                if (menu_index[menu_mode] >= 0 && menu_index[menu_mode] < count) {
                    cheat_section_toggle(ch, menu_index[menu_mode], !cheat_get_section(ch, menu_index[menu_mode])->status & 1);
                }
                break;
            }
            case MENU_MODE_TROP: {
                const trophy_info *trops;
                int count = trophy_get_info(&trops);
                int mi = menu_index[menu_mode];
                if (mi >= 0 && mi < count) {
                    const trophy_info *ti = &trops[mi];
                    if (!ti->unlocked && ti->grade != 1)
                        trophy_unlock(ti->id, ti->hidden, NULL, NULL);
                }
                break;
            }
        }
        break;
    case 1:
    case 2:
        switch(menu_mode) {
            case MENU_MODE_ADV:
                switch(menu_index[menu_mode]) {
                    case 0:
                        language_toggle(type == 2);
                        break;
                    case 2:
                        clocks_toggle(type == 2);
                        break;
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
                menu_run(0);
                break;
            }
            if ((old_buttons & SCE_CTRL_LEFT) == SCE_CTRL_LEFT) {
                menu_run(1);
                break;
            }
            if ((old_buttons & SCE_CTRL_RIGHT) == SCE_CTRL_RIGHT) {
                menu_run(2);
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
    if (menu_mode == MENU_MODE_CLOSE && !msg_deadline) return;
    sceKernelLockMutex(drawMutex, 1, NULL);
    _show_menu(1);
    sceKernelUnlockMutex(drawMutex, 1);
    sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
}

int sceDisplaySetFrameBuf_patched(const SceDisplayFrameBuf *param, int sync) {
    if (refs[0] == 0) return -1;
    if (menu_mode != MENU_MODE_CLOSE || msg_deadline) {
        sceKernelLockMutex(drawMutex, 1, NULL);
        if (blit_set_frame_buf(param) == 0)
            _show_menu(0);
        sceKernelUnlockMutex(drawMutex, 1);
        sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
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
    drawMutex = sceKernelCreateMutex("rcsvr_draw_mutex", 0, 0, 0);
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
    if (drawMutex >= 0) {
        sceKernelDeleteMutex(drawMutex);
        drawMutex = -1;
    }
}

void ui_set_show_msg(uint64_t millisec, int count, ...) {
    va_list arg_ptr;
    int i;
    sceKernelLockMutex(drawMutex, 1, NULL);
    msg_deadline = millisec + util_gettick();
    va_start(arg_ptr, count);
    if (count > MSG_MAX) count = MSG_MAX;
    for (i = 0; i < count; ++i) {
        sceClibStrncpy(show_msg[i], va_arg(arg_ptr, const char*), 79);
        show_msg[i][79] = 0;
    }
    if (i < MSG_MAX)
        show_msg[i][0] = 0;
    va_end(arg_ptr);
    sceKernelUnlockMutex(drawMutex, 1);
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
