#include "cheat.h"

#include "mem.h"
#include "debug.h"
#include "util.h"

#include "kernel/api.h"

#include "libcheat/libcheat.h"

#include <vitasdk.h>

typedef struct cheat_data_t {
    uint32_t base_addr;
} cheat_data_t;

static cheat_t *cheat = NULL;
static cheat_data_t cheat_data;

enum {
    CO_SETBASEADDR = CO_EXTENSION,
};

int read_cb(void *arg, uint32_t addr, void *data, int len, int need_conv) {
    void *paddr = need_conv ? (void*)(uintptr_t)mem_convert(cheat_data.base_addr + addr, NULL) : (void*)(uintptr_t)addr;
    if (!paddr) return -1;
    memcpy(data, paddr, len);
    return len;
}

int write_cb(void *arg, uint32_t addr, const void *data, int len, int need_conv) {
    int readonly = 0;
    void *paddr = need_conv ? (void*)(uintptr_t)mem_convert(cheat_data.base_addr + addr, &readonly) : (void*)(uintptr_t)addr;
    if (!paddr) return -1;
    if (readonly)
        rcsvrMemcpyForce(paddr, data, len, 1);
    else
        rcsvrMemcpy(paddr, data, len);
    return len;
}

int trans_cb(void *arg, uint32_t addr, uint32_t value, int len, int op, int need_conv) {
    int readonly = 0;
    void *paddr = need_conv ? (void*)(uintptr_t)mem_convert(cheat_data.base_addr + addr, &readonly) : (void*)(uintptr_t)addr;
    uint32_t val = 0;
    if (!paddr) return -1;
    sceClibMemcpy(&val, paddr, len);
    switch(op) {
        case 0:
            val += value; break;
        case 1:
            val -= value; break;
        case 2:
            val |= value; break;
        case 3:
            val &= value; break;
        case 4:
            val ^= value; break;
    }
    if (readonly)
        rcsvrMemcpyForce(paddr, &val, len, 1);
    else
        rcsvrMemcpy(paddr, &val, len);
    return len;
}

int copy_cb(void *arg, uint32_t toaddr, uint32_t fromaddr, int len, int need_conv) {
    void *paddr1, *paddr2;
    paddr2 = need_conv ? (void*)(uintptr_t)mem_convert(cheat_data.base_addr + fromaddr, NULL) : (void*)(uintptr_t)fromaddr;
    if (!paddr2) return -1;
    int readonly = 0;
    paddr1 = need_conv ? (void*)(uintptr_t)mem_convert(cheat_data.base_addr + toaddr, &readonly) : (void*)(uintptr_t)toaddr;
    if (!paddr1) return -1;
    if (readonly)
        rcsvrMemcpyForce(paddr1, paddr2, len, 1);
    else
        rcsvrMemcpy(paddr1, paddr2, len);
    return len;
}

extern uint32_t old_buttons;
static int input_cb(void *arg, uint32_t buttons) {
    return (buttons & old_buttons) == buttons;
}

int ext_cb(void *arg, cheat_code_t *code, const char *op, uint32_t val1, uint32_t val2) {
    switch (op[0]) {
        case 'B':
            code->op = CO_SETBASEADDR;
            code->type = CT_I32;
            code->extra = 0;
            code->addr = val1;
            code->value = 0;
            return CR_OK;
        default:
            return CR_INVALID;
    }
}

int ext_call_cb(void *arg, int line, const cheat_code_t *code) {
    switch (code->op) {
        case CO_SETBASEADDR:
            ((cheat_data_t*)arg)->base_addr = code->addr;
            return CR_OK;
        default:
            return CR_INVALID;
    }
}

static int readline_cb(const char *line, void *arg) {
    if (*(int*)arg) {
        if (cheat != NULL) {
            cheat_free();
        }
        cheat = cheat_new2(CH_CWCHEAT, my_realloc, my_free, &cheat_data);
        cheat_set_read_cb(cheat, read_cb);
        cheat_set_write_cb(cheat, write_cb);
        cheat_set_trans_cb(cheat, trans_cb);
        cheat_set_copy_cb(cheat, copy_cb);
        cheat_set_button_cb(cheat, input_cb);
        cheat_set_ext_cb(cheat, ext_cb);
        cheat_set_ext_call_cb(cheat, ext_call_cb);
        *(int*)arg = 0;
    }
    cheat_add(cheat, line);
    return 0;
}

static int cheat_loadfile(const char *filename) {
    int first = 1;
    return util_readfile_by_line(filename, readline_cb, &first);
}

int cheat_load(const char *titleid) {
    char filename[256];
    int ret;
    sceClibSnprintf(filename, 256, "ux0:data/rcsvr/cheat/%s.txt", titleid);
    ret = cheat_loadfile(filename);
    if (ret == 0) return 0;
    sceClibSnprintf(filename, 256, "ux0:data/rcsvr/cheat/%s.ini", titleid);
    ret = cheat_loadfile(filename);
    if (ret == 0) return 0;
    sceClibSnprintf(filename, 256, "ux0:data/rcsvr/cheat/%s.dat", titleid);
    return cheat_loadfile(filename);
}

void cheat_free() {
    if (cheat == NULL) return;
    cheat_finish(cheat);
    cheat = NULL;
}

int cheat_loaded() {
    return cheat != NULL;
}

void cheat_process() {
    if (cheat == NULL) return;
    mem_check_reload();
    cheat_data.base_addr = 0U;
    cheat_apply(cheat);
}

cheat_t *cheat_get_handle() {
    return cheat;
}
