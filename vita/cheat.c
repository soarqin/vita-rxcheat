#include "cheat.h"

#include "mem.h"
#include "debug.h"
#include "util.h"

#include "kernel_api.h"

#include "libcheat/libcheat.h"

#include <vitasdk.h>

static cheat_t *cheat = NULL;
static int sec_count = 0;
static const cheat_section_t *sections = NULL;

int read_cb(uint32_t addr, void *data, int len, int need_conv) {
    void *paddr = need_conv ? (void*)(uintptr_t)mem_convert(addr, NULL) : (void*)(uintptr_t)addr;
    if (!paddr) return -1;
    memcpy(data, paddr, len);
    return len;
}

int write_cb(uint32_t addr, const void *data, int len, int need_conv) {
    int readonly = 0;
    void *paddr = need_conv ? (void*)(uintptr_t)mem_convert(addr, &readonly) : (void*)(uintptr_t)addr;
    if (!paddr) return -1;
    if (readonly)
        rcsvrMemcpy(paddr, data, len);
    else
        sceClibMemcpy(paddr, data, len);
    return len;
}

int trans_cb(uint32_t addr, uint32_t value, int len, int op, int need_conv) {
    int readonly = 0;
    void *paddr = need_conv ? (void*)(uintptr_t)mem_convert(addr, &readonly) : (void*)(uintptr_t)addr;
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
        rcsvrMemcpy(paddr, &val, len);
    else
        sceClibMemcpy(paddr, &val, len);
    return len;
}

int copy_cb(uint32_t toaddr, uint32_t fromaddr, int len, int need_conv) {
    void *paddr1, *paddr2;
    paddr2 = need_conv ? (void*)(uintptr_t)mem_convert(fromaddr, NULL) : (void*)(uintptr_t)fromaddr;
    if (!paddr2) return -1;
    int readonly = 0;
    paddr1 = need_conv ? (void*)(uintptr_t)mem_convert(toaddr, &readonly) : (void*)(uintptr_t)toaddr;
    if (!paddr1) return -1;
    if (readonly)
        rcsvrMemcpy(paddr1, paddr2, len);
    else
        sceClibMemcpy(paddr1, paddr2, len);
    return len;
}

extern uint32_t old_buttons;
static int input_cb(uint32_t buttons) {
    return (buttons & old_buttons) == buttons;
}

static int cheat_loadfile(const char *filename) {
    const int BUF_SIZE = 128;
    char buf[BUF_SIZE];
    SceUID f = sceIoOpen(filename, SCE_O_RDONLY, 0666);
    if (f < 0) return f;
    if (cheat != NULL) {
        cheat_free();
    }
    cheat = cheat_new2(CH_CWCHEAT, my_realloc, my_free);
    cheat_set_read_cb(cheat, read_cb);
    cheat_set_write_cb(cheat, write_cb);
    cheat_set_trans_cb(cheat, trans_cb);
    cheat_set_copy_cb(cheat, copy_cb);
    cheat_set_button_cb(cheat, input_cb);
    {
        int pos = 0;
        while(1) {
            int i = pos, n = sceIoRead(f, buf + pos, BUF_SIZE - pos), s = 0;
            if (n <= 0) {
                if (pos > s) {
                    buf[pos] = 0;
                    cheat_add(cheat, buf + s);
                }
                break;
            }
            n += pos;
            while(1) {
                while(i < n && buf[i] != '\r' && buf[i] != '\n' && buf[i] != 0) ++i;
                if (i >= n) {
                    if (s == 0) {
                        buf[n - 1] = 0;
                        cheat_add(cheat, buf);
                        pos = 0;
                    } else {
                        memmove(buf, buf + s, n - s);
                        pos = n - s;
                    }
                    break;
                }
                buf[i] = 0;
                if (i > s) cheat_add(cheat, buf + s);
                ++i;
                while(i < n && buf[i] == '\r' && buf[i] == '\n') ++i;
                if (i >= n) {
                    pos = 0;
                    break;
                }
                pos = s = i;
            }
        }
    }
    {
        int pos = 0;
        while(1) {
            int i = pos, n = sceIoRead(f, buf + pos, 256 - pos), s;
            if (n <= 0) {
                if (pos > 0) {
                    buf[pos] = 0;
                    cheat_add(cheat, buf);
                }
                break;
            }
            n += pos;
            s = 0;
            while(1) {
                while(i < n && buf[i] != '\r' && buf[i] != '\n' && buf[i] != 0) ++i;
                if (i >= n) break;
                buf[i] = 0;
                cheat_add(cheat, buf + s);
                ++i;
                while(i < n && buf[i] == '\r' && buf[i] == '\n') ++i;
                if (i >= n) break;
                pos = s = i;
            }
        }
    }
    sec_count = cheat_get_sections(cheat, &sections);
    sceIoClose(f);
    mem_reload();
    return 0;
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
    sec_count = 0;
    sections = NULL;
    cheat_finish(cheat);
    cheat = NULL;
}

int cheat_loaded() {
    return cheat != NULL;
}

void cheat_process() {
    if (cheat == NULL) return;
    cheat_apply(cheat);
}

cheat_t *cheat_get_handle() {
    return cheat;
}
