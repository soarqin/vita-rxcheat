#include "lang.h"

#include "util.h"
#include "debug.h"

#include "kernel/api.h"

#include <vitasdk.h>

const char *default_lang[] = {
    "English",
    "L+R+" CHAR_LEFT "+SELECT to open in-game menu",
    "Cheat codes loaded",
    "Main Menu",
    "Cheats",
    "Trophies",
    "Advance",
    "|P|",
    "|G|",
    "|S|",
    "|B|",
    "Dump Memory",
    "Dumping...",
    "Change CLOCKS",
    "CPU/BUS/GPU/XBAR CLOCKS(MHz):",
};

static char *lang_string = NULL;
static int lang_offset[LANG_COUNT] = {};
static char lang_list[16][2][16] = {};
static int lang_count = 0, lang_index = 0;

static inline void lang_set_default() {
    if (lang_string) my_free(lang_string);
    int totallen = 0;
    for (int i = 0; i < LANG_COUNT; ++i) {
        lang_offset[i] = totallen;
        totallen += 1 + sceClibStrnlen(default_lang[i], 128);
    }
    lang_string = (char*)my_alloc(totallen);
    lang_string[totallen - 1] = 0;
    for (int i = 0; i < LANG_COUNT; ++i) {
        sceClibStrncpy(lang_string + lang_offset[i], default_lang[i], 128);
    }
}

void lang_init() {
    lang_list[0][0][0] = 0;
    sceClibStrncpy(lang_list[0][1], default_lang[0], 15);
    lang_count = 1;
    lang_index = 0;
    lang_set_default();
    int fd = rcsvrIoDopen("ux0:data/rcsvr/language");
    if (fd < 0) return;
    SceIoDirent dir;
    while(rcsvrIoDread(fd, &dir) > 0 && lang_count < 16) {
        char *pos = sceClibStrrchr(dir.d_name, '.');
        if (sceClibStrcmp(pos, ".txt") == 0) {
            char filename[128];
            sceClibSnprintf(filename, 128, "ux0:data/rcsvr/language/%s", dir.d_name);
            int f = sceIoOpen(filename, SCE_O_RDONLY, 0644);
            if (f >= 0) {
                char line[16];
                sceIoRead(f, line, 15);
                line[15] = 0;
                int idx = 0;
                while(line[idx] != 0 && line[idx] != '\r' && line[idx] != '\n') ++idx;
                line[idx] = 0;
                sceClibStrncpy(lang_list[lang_count][1], line, 16);
                sceIoClose(f);
                log_trace("Found language %s: %s\n", dir.d_name, line);
                *pos = 0;
                sceClibStrncpy(lang_list[lang_count++][0], dir.d_name, 15);
            }
        }
    }
    rcsvrIoDclose(fd);
}

void lang_finish() {
    if (lang_string) {
        my_free(lang_string);
        lang_string = NULL;
    }
    lang_count = 0;
}

typedef struct {
    int totallen;
    int count;
} readfile_firstpass_t;

static int readfile_firstpass_cb(const char *line, void *arg) {
    readfile_firstpass_t *rf = (readfile_firstpass_t*)arg;
    lang_offset[rf->count] = rf->totallen;
    rf->totallen += 1 + sceClibStrnlen(line, 128);
    if (++rf->count >= LANG_COUNT) return -1;
    return 0;
}

static int readfile_secondpass_cb(const char *line, void *arg) {
    int *count = (int*)arg;
    sceClibStrncpy(lang_string + lang_offset[*count], line, 128);
    if (++*count >= LANG_COUNT) return -1;
    return 0;
}

int lang_set(int index) {
    if (lang_index == index) return 0;
    if (index >= lang_count) return -1;
    char filename[128];
    sceClibSnprintf(filename, 128, "ux0:data/rcsvr/language/%s.txt", lang_list[index][0]);
    readfile_firstpass_t rf = {0, 0};
    int ret = util_readfile_by_line(filename, readfile_firstpass_cb, &rf);
    if (ret < 0) return ret;
    if (lang_string) {
        my_free(lang_string);
        lang_string = (char*)my_alloc(rf.totallen);
    }
    int count = 0;
    util_readfile_by_line(filename, readfile_secondpass_cb, &count);
    lang_index = index;
    return ret;
}

int lang_get_count() {
    return lang_count;
}

const char *lang_get_name(int idx) {
    return lang_list[idx][1];
}

const char *lang_get(int id) {
    return lang_string + lang_offset[id];
}
