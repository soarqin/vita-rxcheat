#include "trophy.h"

#include "util.h"
#include "debug.h"

#include "np_trophy.h"

#include <vitasdk.h>
#include <taihen.h>

#define HOOKS_NUM 2

static SceUID hooks[HOOKS_NUM] = {};
static tai_hook_ref_t refs[HOOKS_NUM] = {};

static SceNpTrophyContext context = -1;
static SceUID trophyMutex = -1, trophySema = -1;
static int trophy_unlocking = 0;
static int trophy_load_status = 0; // 0 - not loaded  1 - loading  2 - loaded
static int trophy_count = 0;
static trophy_info *trophies = NULL;

static void _update_trophy_data(int index, SceNpTrophyDetails *detail, SceNpTrophyData *data) {
    if (trophy_load_status == 0) return;
    sceKernelLockMutex(trophyMutex, 1, NULL);
    if (index < trophy_count) {
        trophy_info *ti = &trophies[index];
        ti->id = detail->trophyId;
        ti->grade = detail->trophyGrade;
        ti->hidden = detail->hidden;
        ti->unlocked = data->unlocked;
        sceClibStrncpy(ti->name, detail->name, 59); ti->name[59] = 0;
        sceClibStrncpy(ti->desc, detail->description, 127); ti->desc[127] = 0;
    }
    sceKernelUnlockMutex(trophyMutex, 1);
}

int sceNpTrophyCreateContext_patched(SceNpTrophyContext *c, const SceNpCommunicationId *commId, const SceNpCommunicationSignature *commSign, SceUInt64 options) {
    int ret = TAI_CONTINUE(int, refs[0], c, commId, commSign, options);
    if (ret >= 0)
        context = *c;
    return ret;
}

int sceNpTrophyUnlockTrophy_patched(SceNpTrophyContext c, SceNpTrophyHandle handle, SceNpTrophyId trophyId, int *platinumId) {
    int ret = TAI_CONTINUE(int, refs[1], c, handle, trophyId, platinumId);
    if (ret < 0 || trophy_unlocking) return ret;

    SceNpTrophyDetails detail;
    SceNpTrophyData data;
    detail.size = sizeof(SceNpTrophyDetails);
    data.size = sizeof(SceNpTrophyData);
    int ret2 = sceNpTrophyGetTrophyInfo(context, handle, trophyId, &detail, &data);
    if (ret2 < 0) return ret;
    _update_trophy_data(trophyId, &detail, &data);

    return ret;
}

void trophy_init() {
    trophyMutex = sceKernelCreateMutex("rcsvr_trophy_mutex", 0, 0, 0);
    trophySema = sceKernelCreateSema("rcsvr_trophy_sema", 0, 0, 1, NULL);
    if (sceSysmoduleIsLoaded(SCE_SYSMODULE_NP) != SCE_SYSMODULE_LOADED)
        sceSysmoduleLoadModule(SCE_SYSMODULE_NP);
    if (sceSysmoduleIsLoaded(SCE_SYSMODULE_NP_TROPHY) != SCE_SYSMODULE_LOADED)
        sceSysmoduleLoadModule(SCE_SYSMODULE_NP_TROPHY);
    hooks[0] = taiHookFunctionExport(
        &refs[0],
        "SceNpTrophy",
        0x4332C10D,
        0xC49FD33F,
        sceNpTrophyCreateContext_patched);
    hooks[1] = taiHookFunctionExport(
        &refs[1],
        "SceNpTrophy",
        0x4332C10D,
        0xB397AA24,
        sceNpTrophyUnlockTrophy_patched);
    log_trace("Hook sceNpTrophyCreateContext: %X %X\n", hooks[0], refs[0]);
}

void trophy_finish() {
    for (int i = 0; i < HOOKS_NUM; i++) {
        if (hooks[i] > 0) {
            taiHookRelease(hooks[i], refs[i]);
            hooks[i] = 0;
        }
    }
    sceKernelLockMutex(trophyMutex, 1, NULL);
    trophy_load_status = 0;
    trophy_count = 0;
    if (trophies) {
        my_free(trophies);
        trophies = NULL;
    }
    sceKernelUnlockMutex(trophyMutex, 1);
    if (trophySema >= 0) {
        sceKernelDeleteSema(trophySema);
        trophySema = -1;
    }
    if (trophyMutex >= 0) {
        sceKernelDeleteMutex(trophyMutex);
        trophyMutex = -1;
    }
}

int trophy_get_status() {
    return trophy_load_status;
}

int trophy_get_count() {
    return (trophy_load_status == 2 ? trophy_count : 0);
}

int trophy_get_info(const trophy_info **t) {
    if (trophy_load_status != 2) {
        *t = NULL;
        return 0;
    }
    *t = trophies;
    return trophy_count;
}

typedef struct {
    int type;
    int id;
    uint32_t hidden[4];
    trophy_info_cb_t cb;
    trophy_unlock_cb_t cb2;
    trophy_info_cb_end_t cb_end;
} trophy_request;

static trophy_request trophy_req;

static int _trophy_thread(SceSize args, void *argp) {
    sceKernelLockMutex(trophyMutex, 1, NULL);
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
    sceKernelUnlockMutex(trophyMutex, 1);
    SceNpTrophyHandle handle;
    int ret = sceNpTrophyCreateHandle(&handle);
    if (ret < 0) {
        log_error("sceNpTrophyCreateHandle: %d\n", ret);
        cb_end(-1);
        if (type == 0)
            trophy_load_status = 0;
        return sceKernelExitDeleteThread(0);
    }
    switch(type) {
    case 0: {
            SceNpTrophyGameDetails detail0;
            detail0.size = sizeof(SceNpTrophyGameDetails);
            ret = sceNpTrophyGetGameInfo(context, handle, &detail0, NULL);
            if (ret < 0) {
                if (cb_end) cb_end(-1);
                trophy_load_status = 0;
                break;
            }
            SceNpTrophyDetails detail;
            SceNpTrophyData data;
            detail.size = sizeof(SceNpTrophyDetails);
            data.size = sizeof(SceNpTrophyData);
            sceKernelLockMutex(trophyMutex, 1, NULL);
            if (trophies) {
                my_free(trophies);
                trophies = NULL;
            }
            trophies = (trophy_info*)my_alloc(sizeof(trophy_info) * detail0.numTrophies);
            sceClibMemset(trophies, 0, sizeof(trophy_info) * detail0.numTrophies);
            trophy_count = detail0.numTrophies;
            sceKernelUnlockMutex(trophyMutex, 1);
            for (int i = 0; i < detail0.numTrophies; ++i) {
                int ret = sceNpTrophyGetTrophyInfo(context, handle, i, &detail, &data);
                if (ret < 0) continue;
                if (cb) cb(detail.trophyId, detail.trophyGrade, detail.hidden, data.unlocked, detail.name, detail.description);
                _update_trophy_data(i, &detail, &data);
            }
            if (cb_end) cb_end(0);
            trophy_load_status = 2;
            break;
        }
    case 1: {
            int platid;
            trophy_unlocking = 1;
            int ret = sceNpTrophyUnlockTrophy(context, handle, id, &platid);
            trophy_unlocking = 0;
            if (cb2) cb2(ret, id, platid);
            if (ret < 0) break;
            if (hidden[0]) {
                SceNpTrophyDetails detail;
                SceNpTrophyData data;
                detail.size = sizeof(SceNpTrophyDetails);
                data.size = sizeof(SceNpTrophyData);
                ret = sceNpTrophyGetTrophyInfo(context, handle, id, &detail, &data);
                if (ret < 0) break;
                if (cb) cb(detail.trophyId, detail.trophyGrade, detail.hidden, data.unlocked, detail.name, detail.description);
                _update_trophy_data(id, &detail, &data);
            } else {
                if (trophy_load_status != 0 && id < trophy_count) {
                    trophies[id].unlocked = 1;
                }
            }
            break;
        }
    case 2: {
            int platid;
            SceNpTrophyFlagArray a;
            int count;
            int ret = sceNpTrophyGetTrophyUnlockState(context, handle, &a, &count);
            if (ret < 0) {
                if (cb2) cb2(ret, 0, 0);
                break;
            }
            SceNpTrophyGameDetails detail0;
            detail0.size = sizeof(SceNpTrophyGameDetails);
            ret = sceNpTrophyGetGameInfo(context, handle, &detail0, NULL);
            if (ret < 0) {
                if (cb2) cb2(ret, 0, 0);
                break;
            }
            for (int i = 0; i < detail0.numTrophies; ++i) {
                if (a.flag_bits[i >> 5] & (1U << (i & 0x1F))) continue;
                trophy_unlocking = 1;
                ret = sceNpTrophyUnlockTrophy(context, handle, i, &platid);
                trophy_unlocking = 0;
                if (cb2) cb2(ret, i, platid);
                if (ret < 0) continue;
                if (!(hidden[i >> 5] & (1U << (i & 0x1F)))) {
                    if (trophy_load_status != 0 && id < trophy_count) {
                        trophies[id].unlocked = 1;
                    }
                    continue;
                }
                SceNpTrophyDetails detail;
                SceNpTrophyData data;
                detail.size = sizeof(SceNpTrophyDetails);
                data.size = sizeof(SceNpTrophyData);
                int ret = sceNpTrophyGetTrophyInfo(context, handle, i, &detail, &data);
                if (ret < 0) continue;
                if (cb) cb(detail.trophyId, detail.trophyGrade, detail.hidden, data.unlocked, detail.name, detail.description);
                _update_trophy_data(i, &detail, &data);
            }
            break;
        }
    }
    sceNpTrophyDestroyHandle(handle);
    return sceKernelExitDeleteThread(0);
}

void trophy_list(trophy_info_cb_t cb, trophy_info_cb_end_t cb_end) {
    switch (trophy_load_status) {
        case 0: {
            if (context < 0) cb_end(-1);
            trophy_load_status = 1;
            trophy_req.type = 0;
            trophy_req.cb = cb;
            trophy_req.cb_end = cb_end;
            SceUID thid = sceKernelCreateThread("rcsvr_trophy_thread", (SceKernelThreadEntry)_trophy_thread, 0x10000100, 0x10000, 0, SCE_KERNEL_CPU_MASK_USER_1 | SCE_KERNEL_CPU_MASK_USER_2, NULL);
            if (thid >= 0)
                sceKernelStartThread(thid, 0, NULL);
            sceKernelWaitSema(trophySema, 1, NULL);
            break;
        }
        case 1: break;
        case 2: {
            for (int i = 0; i < trophy_count; ++i) {
                if (cb) cb(trophies[i].id, trophies[i].grade, trophies[i].hidden, trophies[i].unlocked, trophies[i].name, trophies[i].desc);
            }
            if (cb_end) cb_end(0);
            return;
        }
    }
}

void trophy_unlock(int id, int hidden, trophy_info_cb_t cb, trophy_unlock_cb_t cb2) {
    trophy_req.type = 1;
    trophy_req.id = id;
    trophy_req.hidden[0] = hidden ? 1 : 0;
    trophy_req.cb = cb;
    trophy_req.cb2 = cb2;
    SceUID thid = sceKernelCreateThread("rcsvr_trophy_thread", (SceKernelThreadEntry)_trophy_thread, 0x10000100, 0x10000, 0, SCE_KERNEL_CPU_MASK_USER_1 | SCE_KERNEL_CPU_MASK_USER_2, NULL);
    if (thid >= 0)
        sceKernelStartThread(thid, 0, NULL);
    sceKernelWaitSema(trophySema, 1, NULL);
}

void trophy_unlock_all(uint32_t *hidden, trophy_info_cb_t cb, trophy_unlock_cb_t cb2) {
    trophy_req.type = 2;
    sceClibMemcpy(trophy_req.hidden, hidden, 4 * sizeof(uint32_t));
    trophy_req.cb = cb;
    trophy_req.cb2 = cb2;
    SceUID thid = sceKernelCreateThread("rcsvr_trophy_thread", (SceKernelThreadEntry)_trophy_thread, 0x10000100, 0x10000, 0, SCE_KERNEL_CPU_MASK_USER_1 | SCE_KERNEL_CPU_MASK_USER_2, NULL);
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
