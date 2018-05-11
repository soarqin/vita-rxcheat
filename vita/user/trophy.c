#include "trophy.h"

#include "debug.h"

#include "np_trophy.h"

#include <vitasdk.h>
#include <taihen.h>

#define HOOKS_NUM 1

static SceUID hooks[HOOKS_NUM] = {0};
static tai_hook_ref_t ref[HOOKS_NUM];

static SceNpTrophyContext context = -1;
static SceUID trophyMutex = -1, trophySema = -1;

int sceNpTrophyCreateContext_patched(SceNpTrophyContext *c, void *commID, void *commSign, uint64_t options) {
    int ret = TAI_CONTINUE(int, ref[0], c, commID, commSign, options);
    if (ret >= 0)
        context = *c;
    return ret;
}

void trophy_init() {
    trophyMutex = sceKernelCreateMutex("rcsvr_trophy_mutex", 0, 0, 0);
    trophySema = sceKernelCreateSema("rcsvr_trophy_sema", 0, 0, 1, NULL);
}

void trophy_finish() {
    if (trophySema >= 0) {
        sceKernelDeleteSema(trophySema);
        trophySema = -1;
    }
    if (trophyMutex >= 0) {
        sceKernelDeleteMutex(trophyMutex);
        trophyMutex = -1;
    }
    trophy_unhook();
}

void trophy_hook() {
    hooks[0] = taiHookFunctionImport(
        &ref[0],
        TAI_MAIN_MODULE,
        0x4332C10D,
        0xC49FD33F,
        sceNpTrophyCreateContext_patched);
}

void trophy_unhook() {
    int i;
    for (i = 0; i < HOOKS_NUM; i++) {
        if (hooks[i] > 0) {
            taiHookRelease(hooks[i], ref[i]);
            hooks[i] = 0;
        }
    }
}

typedef struct {
    int type;
    int id;
    uint32_t hidden[4];
    void (*cb)(int id, int grade, int hidden, int unlocked, const char *name, const char *desc);
    void (*cb2)(int ret, int id, int platid);
    void (*cb_end)(int err);
} trophy_request;

static trophy_request trophy_req;

static int _trophy_thread(SceSize args, void *argp) {
    if (sceKernelTryLockMutex(trophyMutex, 1) < 0) {
        trophy_req.cb_end(-1);
        return sceKernelExitDeleteThread(0);
    }
    int type = trophy_req.type;
    int id = trophy_req.id;
    uint32_t hidden[4];
    sceClibMemcpy(hidden, trophy_req.hidden, 4 * sizeof(uint32_t));
    void (*cb)(int id, int grade, int hidden, int unlocked, const char *name, const char *desc);
    void (*cb2)(int ret, int id, int platid);
    void (*cb_end)(int err);
    cb = trophy_req.cb;
    cb2 = trophy_req.cb2;
    cb_end = trophy_req.cb_end;
    sceKernelSignalSema(trophySema, 1);
    SceNpTrophyHandle handle;
    int ret = sceNpTrophyCreateHandle(&handle);
    if (ret < 0) {
        log_error("sceNpTrophyCreateHandle: %d\n", ret);
        cb_end(-1);
        sceKernelUnlockMutex(trophyMutex, 1);
        return sceKernelExitDeleteThread(0);
    }
    switch(type) {
    case 0: {
            SceNpTrophyGameDetails detail0;
            SceNpTrophyGameData data0;
            detail0.size = sizeof(SceNpTrophyGameDetails);
            data0.size = sizeof(SceNpTrophyGameData);
            ret = sceNpTrophyGetGameInfo(context, handle, &detail0, &data0);
            if (ret < 0) {
                cb_end(-1);
                break;
            }
            SceNpTrophyDetails detail;
            SceNpTrophyData data;
            detail.size = sizeof(SceNpTrophyDetails);
            data.size = sizeof(SceNpTrophyData);
            int i;
            for (i = 0; i < detail0.numTrophies; ++i) {
                int ret = sceNpTrophyGetTrophyInfo(context, handle, i, &detail, &data);
                if (ret < 0) continue;
                cb(detail.trophyId, detail.trophyGrade, detail.hidden, data.unlocked, detail.name, detail.description);
            }
            cb_end(0);
            break;
        }
    case 1: {
            int platid;
            int ret = sceNpTrophyUnlockTrophy(context, handle, id, &platid);
            cb2(ret, id, platid);
            if (ret < 0) break;
            if (hidden[0]) {
                SceNpTrophyDetails detail;
                SceNpTrophyData data;
                detail.size = sizeof(SceNpTrophyDetails);
                data.size = sizeof(SceNpTrophyData);
                ret = sceNpTrophyGetTrophyInfo(context, handle, id, &detail, &data);
                if (ret < 0) break;
                cb(detail.trophyId, detail.trophyGrade, detail.hidden, data.unlocked, detail.name, detail.description);
            }
            break;
        }
    case 2: {
            int platid;
            SceNpTrophyFlagArray a;
            int count;
            int ret = sceNpTrophyGetTrophyUnlockState(context, handle, &a, &count);
            if (ret < 0) {
                cb2(ret, 0, 0);
                break;
            }
            SceNpTrophyGameDetails detail0;
            SceNpTrophyGameData data0;
            detail0.size = sizeof(SceNpTrophyGameDetails);
            data0.size = sizeof(SceNpTrophyGameData);
            ret = sceNpTrophyGetGameInfo(context, handle, &detail0, &data0);
            if (ret < 0) {
                cb2(ret, 0, 0);
                break;
            }
            int i;
            for (i = 0; i < detail0.numTrophies; ++i) {
                if (a.flag_bits[i >> 5] & (1U << (i & 0x1F))) continue;
                ret = sceNpTrophyUnlockTrophy(context, handle, i, &platid);
                cb2(ret, i, platid);
                if (ret < 0) continue;
                if (hidden[i >> 5] & (1U << (i & 0x1F))) {
                    SceNpTrophyDetails detail;
                    SceNpTrophyData data;
                    detail.size = sizeof(SceNpTrophyDetails);
                    data.size = sizeof(SceNpTrophyData);
                    int ret = sceNpTrophyGetTrophyInfo(context, handle, i, &detail, &data);
                    if (ret < 0) continue;
                    cb(detail.trophyId, detail.trophyGrade, detail.hidden, data.unlocked, detail.name, detail.description);
                }
            }
            break;
        }
    }
    sceNpTrophyDestroyHandle(handle);
    sceKernelUnlockMutex(trophyMutex, 1);
    return sceKernelExitDeleteThread(0);
}

void trophy_list(void (*cb)(int id, int grade, int hidden, int unlocked, const char *name, const char *desc), void (*cb_end)(int err)) {
    trophy_req.type = 0;
    trophy_req.cb = cb;
    trophy_req.cb_end = cb_end;
    SceUID thid = sceKernelCreateThread("rcsvr_trophy_thread", (SceKernelThreadEntry)_trophy_thread, 0x10000100, 0x10000, 0, 0, NULL);
    if (thid >= 0)
        sceKernelStartThread(thid, 0, NULL);
    sceKernelWaitSema(trophySema, 1, NULL);
}

void trophy_unlock(int id, int hidden, void (*cb)(int id, int grade, int hidden, int unlocked, const char *name, const char *desc), void (*cb2)(int ret, int id, int platid)) {
    trophy_req.type = 1;
    trophy_req.id = id;
    trophy_req.hidden[0] = hidden ? 1 : 0;
    trophy_req.cb = cb;
    trophy_req.cb2 = cb2;
    SceUID thid = sceKernelCreateThread("rcsvr_trophy_thread", (SceKernelThreadEntry)_trophy_thread, 0x10000100, 0x10000, 0, 0, NULL);
    if (thid >= 0)
        sceKernelStartThread(thid, 0, NULL);
    sceKernelWaitSema(trophySema, 1, NULL);
}

void trophy_unlock_all(uint32_t *hidden, void (*cb)(int id, int grade, int hidden, int unlocked, const char *name, const char *desc), void (*cb2)(int ret, int id, int platid)) {
    trophy_req.type = 2;
    sceClibMemcpy(trophy_req.hidden, hidden, 4 * sizeof(uint32_t));
    trophy_req.cb = cb;
    trophy_req.cb2 = cb2;
    SceUID thid = sceKernelCreateThread("rcsvr_trophy_thread", (SceKernelThreadEntry)_trophy_thread, 0x10000100, 0x10000, 0, 0, NULL);
    if (thid >= 0)
        sceKernelStartThread(thid, 0, NULL);
    sceKernelWaitSema(trophySema, 1, NULL);
}

void trophy_test() {
    SceNpTrophyHandle handle;
    int ret = sceNpTrophyCreateHandle(&handle);
    if (ret < 0) {
        log_error("sceNpTrophyCreateHandle: %d\n", ret);
        return;
    }
    SceNpTrophyGameDetails detail0;
    SceNpTrophyGameData data0;
    detail0.size = sizeof(SceNpTrophyGameDetails);
    data0.size = sizeof(SceNpTrophyGameData);
    ret = sceNpTrophyGetGameInfo(context, handle, &detail0, &data0);
    if (ret < 0)
        log_error("sceNpTrophyGetGameInfo: %d\n", ret);
    SceNpTrophyFlagArray a;
    int count;
    ret = sceNpTrophyGetTrophyUnlockState(context, handle, &a, &count);
    if (ret < 0)
        log_error("sceNpTrophyGetTrophyUnlockState: %d\n", ret);
    else {
        log_trace("sceNpTrophyGetTrophyUnlockState: %d %08X %08X\n", count, a.flag_bits[0], a.flag_bits[1]);
    }
    SceNpTrophyDetails detail;
    SceNpTrophyData data;
    detail.size = sizeof(SceNpTrophyDetails);
    data.size = sizeof(SceNpTrophyData);
    int id;
    for (id = 0; id < detail0.numTrophies; ++id) {
        int ret = sceNpTrophyGetTrophyInfo(context, handle, id, &detail, &data);
        if (ret < 0 && ret != 0x8055160f) break;
        log_trace("sceNpTrophyGetTrophyInfo: %08X %d %d %d %s %s\n", ret, detail.trophyId, detail.trophyGrade, detail.hidden, detail.name, detail.description);
    }
    sceNpTrophyDestroyHandle(handle);
}
