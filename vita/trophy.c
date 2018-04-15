#include "trophy.h"

#include "debug.h"

#include <vitasdk.h>
#include <taihen.h>

#define HOOKS_NUM 2

static SceUID hooks[HOOKS_NUM];
static tai_hook_ref_t ref[HOOKS_NUM];

typedef SceUID SceNpTrophyHandle;
typedef SceUID SceNpTrophyContext;
typedef SceUID SceNpTrophyId;

static SceNpTrophyContext context = -1;
static SceNpTrophyHandle handle = -1;

#define SCE_NP_TROPHY_NAME_MAX_SIZE  128
#define SCE_NP_TROPHY_DESCR_MAX_SIZE 1024

typedef enum SceNpTrophyGrade {
    SCE_NP_TROPHY_GRADE_UNKNOWN        = 0,
    SCE_NP_TROPHY_GRADE_PLATINUM       = 1,
    SCE_NP_TROPHY_GRADE_GOLD           = 2,
    SCE_NP_TROPHY_GRADE_SILVER         = 3,
    SCE_NP_TROPHY_GRADE_BRONZE         = 4,
} SceNpTrophyGrade;

typedef struct SceNpTrophyDetails {
    int           size;
    SceNpTrophyId trophyId;
    int           trophyGrade;
    int           unk0;
    int           hidden;
    char          name[SCE_NP_TROPHY_NAME_MAX_SIZE];
    char          description[SCE_NP_TROPHY_DESCR_MAX_SIZE];
} SceNpTrophyDetails;

typedef struct SceNpTrophyData {
    int           size;
    SceNpTrophyId trophyId;
    int           unlocked;
    int           unk0;
    SceRtcTick    timestamp;
} SceNpTrophyData;

extern int sceNpTrophyUnlockTrophy(SceNpTrophyContext context, SceNpTrophyHandle handle, SceNpTrophyId trophyId, int *platinumId);
extern int sceNpTrophyCreateHandle(SceNpTrophyHandle *handle);
extern int sceNpTrophyGetTrophyInfo(SceNpTrophyContext context, SceNpTrophyHandle handle, SceNpTrophyId trophyId, SceNpTrophyDetails *details, SceNpTrophyData *data);

int sceNpTrophyCreateContext_patched(SceNpTrophyContext *c, void *commID, void *commSign, uint64_t options) {
    int ret = TAI_CONTINUE(SceUID, ref[0], c, commID, commSign, options);
    if (ret >= 0)
        context = *c;
    log_debug("sceNpTrophyCreateContext %d %d\n", ret, *c);
    return ret;
}

int sceNpTrophyInit_patched(uint32_t poolAddr, uint32_t poolSize, uint32_t containerId, uint64_t options) {
    int ret = TAI_CONTINUE(SceUID, ref[1], poolAddr, poolSize, containerId, options);
    return ret;
}

void trophy_init() {
    log_debug("trophy_init\n");
    hooks[0] = taiHookFunctionImport(
        &ref[0],
        TAI_MAIN_MODULE,
        TAI_ANY_LIBRARY,
        0xC49FD33F,
        sceNpTrophyCreateContext_patched);
    hooks[1] = taiHookFunctionImport(
        &ref[1],
        TAI_MAIN_MODULE,
        TAI_ANY_LIBRARY,
        0x34516838,
        sceNpTrophyInit_patched);
}

void trophy_finish() {
    int i;
    for (i = 0; i < HOOKS_NUM; i++)
        taiHookRelease(hooks[i], ref[i]);
}

void trophy_test() {
    if (handle < 0) {
        int ret = sceNpTrophyCreateHandle(&handle);
        if (ret < 0)
            log_debug("sceNpTrophyCreateHandle: %d\n", ret);
    }
    SceNpTrophyDetails detail;
    detail.size = sizeof(SceNpTrophyDetails);
    SceNpTrophyData data;
    data.size = sizeof(SceNpTrophyData);
    int id;
    for (id = 0; id < 256; ++id) {
        int ret = sceNpTrophyGetTrophyInfo(context, handle, id, &detail, &data);
        if (ret < 0 && ret != 0x8055160f) break;
        log_debug("sceNpTrophyGetTrophyInfo: %08X %d %d %d %d\n", ret, detail.trophyId, detail.trophyGrade, detail.spec, detail.hidden);
    }
}
