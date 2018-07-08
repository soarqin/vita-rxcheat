#include "lang.h"

#include "config.h"
#include "util.h"
#include "debug.h"

#include "kernel/api.h"

#include <vitasdk.h>

#define LANG_BASE_DIR "ux0:data/rcsvr/lang"

const char *default_lang[] = {
    "English",
    "L+R+" CHAR_LEFT "+SELECT to open in-game menu",
    "Cheat codes loaded",
    "Main Menu",
    "Cheats",
    "Cheat codes not found",
    "Trophies",
    "Loading trophies, be patient...",
    "Advance",
    "|P|",
    "|G|",
    "|S|",
    "|B|",
    "[HIDDEN]",
    "Dump Memory",
    "Dump Memory - Dumping...",
    "Change CLOCKS",
    "Current language: %s",
    "CPU/BUS/GPU/XBAR CLOCKS(MHz):",
    "Change language",
    "Searching...",
    "Unlocked trophy",
    "Unlocked platinum trophy",
    "ON",
    "OFF",
    "ONCE",
};

static char *lang_string = NULL;
static int lang_offset[LANG_COUNT] = {};
static char lang_list[16][2][16] = {};
static uint32_t lang_sys[16] = {};
static int lang_count = 0;
static int lang_index = 0;

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
        sceClibStrncpy(lang_string + lang_offset[i], default_lang[i], totallen - lang_offset[i]);
    }
}

typedef struct {
    int totallen;
    int count;
} readfile_t;

static int readfile_firstpass_cb(const char *line, void *arg) {
    readfile_t *rf = (readfile_t*)arg;
    lang_offset[rf->count] = rf->totallen;
    rf->totallen += 1 + sceClibStrnlen(line, 128);
    if (++rf->count >= LANG_COUNT) return -1;
    return 0;
}

static int readfile_secondpass_cb(const char *line, void *arg) {
    readfile_t *rf = (readfile_t*)arg;
    sceClibStrncpy(lang_string + lang_offset[rf->count], line, rf->totallen - lang_offset[rf->count]);
    if (++rf->count >= LANG_COUNT) return -1;
    return 0;
}

static inline int lang_set_by_index(int index) {
    if (index == 0) {
        lang_set_default();
        return 0;
    }
    char filename[128];
    sceClibSnprintf(filename, 128, LANG_BASE_DIR "/%s.txt", lang_list[index][0]);
    readfile_t rf = {0, 0};
    int ret = util_readfile_by_line(filename, readfile_firstpass_cb, &rf);
    if (ret < 0) return ret;
    if (lang_string) {
        my_free(lang_string);
    }
    lang_string = (char*)my_alloc(rf.totallen);
    rf.count = 0;
    util_readfile_by_line(filename, readfile_secondpass_cb, &rf);
    return 0;
}

void lang_init() {
    sceClibStrncpy(lang_list[0][0], " ", 15);
    sceClibStrncpy(lang_list[0][1], default_lang[0], 15);
    lang_count = 1;
    int fd = rcsvrIoDopen(LANG_BASE_DIR);
    if (fd < 0) {
        lang_set_default();
        if (sceClibStrncmp(g_config.lang, lang_list[0][0], 16) != 0) {
            sceClibStrncpy(g_config.lang, lang_list[0][0], 16);
            config_save();
        }
        return;
    }
    SceIoDirent dir;
    while(rcsvrIoDread(fd, &dir) > 0 && lang_count < 16) {
        char *pos = sceClibStrrchr(dir.d_name, '.');
        if (sceClibStrcmp(pos, ".txt") == 0) {
            char filename[128];
            sceClibSnprintf(filename, 128, LANG_BASE_DIR "/%s", dir.d_name);
            int f = sceIoOpen(filename, SCE_O_RDONLY, 0644);
            if (f >= 0) {
                char line[16];
                sceIoRead(f, line, 15);
                line[15] = 0;
                int idx = 0;
                lang_sys[lang_count] = 0;
                while(line[idx] != 0 && line[idx] != ';' && line[idx] != '\r' && line[idx] != '\n') ++idx;
                if (line[idx] == ';') {
                    line[idx] = 0;
                    while(1) {
                        int s = ++idx;
                        while(line[idx] != 0 && line[idx] != ' ' && line[idx] != '\r' && line[idx] != '\n') ++idx;
                        int isend = line[idx] != ' ';
                        line[idx] = 0;
                        long n = strtoul(line + s, NULL, 10);
                        if (n < 20)
                            lang_sys[lang_count] |= 1 << n;
                        if (isend) break;
                    }
                }
                line[idx] = 0;
                sceClibStrncpy(lang_list[lang_count][1], line, 16);
                sceIoClose(f);
                log_trace("Found language %s: %s %X\n", dir.d_name, line, lang_sys[lang_count]);
                *pos = 0;
                sceClibStrncpy(lang_list[lang_count++][0], dir.d_name, 15);
            }
        }
    }
    rcsvrIoDclose(fd);
    if (g_config.lang[0] == 0) {
        int language;
        int ret = sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &language);
        log_trace("System langugage: %X %d\n", ret, language);
        if (ret < 0) {
            lang_set_by_index(0);
            return;
        }
        for (int i = 0; i < lang_count; ++i) {
            if (lang_sys[i] & (1 << language)) {
                int ret = lang_set_by_index(i);
                if (ret == 0) {
                    lang_index = i;
                    sceClibStrncpy(g_config.lang, lang_list[lang_index][0], 16);
                    config_save();
                }
                return;
            }
        }
    } else {
        for (int i = 0; i < lang_count; ++i) {
            if (sceClibStrncmp(g_config.lang, lang_list[i][0], 16) == 0) {
                int ret = lang_set_by_index(i);
                if (ret == 0) {
                    lang_index = i;
                    return;
                }
            }
        }
    }
    lang_index = 0;
    lang_set_by_index(0);
}

void lang_finish() {
    if (lang_string) {
        my_free(lang_string);
        lang_string = NULL;
    }
    lang_count = 0;
}

int lang_set(int index) {
    if (lang_index == index) return 0;
    if (index >= lang_count) return -1;
    int ret = lang_set_by_index(index);
    if (ret < 0) return ret;
    lang_index = index;
    sceClibStrncpy(g_config.lang, lang_list[lang_index][0], 16);
    config_save();
    return 0;
}

int lang_get_count() {
    return lang_count;
}

int lang_get_index() {
    return lang_index;
}

const char *lang_get_name(int idx) {
    return lang_list[idx][1];
}

const char *lang_get(int id) {
    return lang_string + lang_offset[id];
}
