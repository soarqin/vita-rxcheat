#include "cheat.h"

#include "debug.h"
#include "util.h"

#include "libcheat/libcheat.h"

#include <vitasdk.h>

static cheat_t *cheat = NULL;

static int cheat_loadfile(const char *filename) {
    char buf[256];
    SceUID f = sceIoOpen(filename, SCE_O_RDONLY, 0666);
    if (f < 0) return f;
    if (cheat != NULL) {
        cheat_free();
    }
    cheat = cheat_new2(CH_CWCHEAT, my_realloc, my_free);
    while(1) {
        int n = sceIoRead(f, buf, 256);
        if (n <= 0) break;
    }
    sceIoClose(f);
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
