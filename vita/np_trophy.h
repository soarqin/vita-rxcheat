#ifndef __NP_TROPHY_H_
#define __NP_TROPHY_H_

#include <psp2/types.h>
#include <psp2/rtc.h>

typedef SceUID SceNpTrophyHandle;
typedef SceUID SceNpTrophyContext;
typedef int    SceNpTrophyId;

enum {
    SCE_NP_TROPHY_TITLE_MAX_SIZE      = 128,
    SCE_NP_TROPHY_GAME_DESCR_MAX_SIZE = 1024,
    SCE_NP_TROPHY_NAME_MAX_SIZE       = 128,
    SCE_NP_TROPHY_DESCR_MAX_SIZE      = 1024,
    SCE_NP_TROPHY_FLAG_SETSIZE        = 128,
    SCE_NP_TROPHY_FLAG_BITS_SHIFT     = 5,
};

typedef enum SceNpTrophyGrade {
    SCE_NP_TROPHY_GRADE_UNKNOWN        = 0,
    SCE_NP_TROPHY_GRADE_PLATINUM       = 1,
    SCE_NP_TROPHY_GRADE_GOLD           = 2,
    SCE_NP_TROPHY_GRADE_SILVER         = 3,
    SCE_NP_TROPHY_GRADE_BRONZE         = 4,
} SceNpTrophyGrade;

typedef struct SceNpTrophyGameDetails {
    int  size;
    int  platinumId;
    int  numTrophies;
    int  numPlatinum;
    int  numGold;
    int  numSilver;
    int  numBronze;
    char title[SCE_NP_TROPHY_TITLE_MAX_SIZE];
    char description[SCE_NP_TROPHY_GAME_DESCR_MAX_SIZE];
} SceNpTrophyGameDetails;

typedef struct SceNpTrophyGameData {
    int size;
    int unlockedTrophies;
    int unlockedPlatinum;
    int unlockedGold;
    int unlockedSilver;
    int unlockedBronze;
    int unk0;
} SceNpTrophyGameData;

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

typedef struct SceNpTrophyFlagArray {
    SceUInt32 flag_bits[SCE_NP_TROPHY_FLAG_SETSIZE >> SCE_NP_TROPHY_FLAG_BITS_SHIFT];
} SceNpTrophyFlagArray;

extern int sceNpTrophyCreateHandle(SceNpTrophyHandle *handle);
extern int sceNpTrophyGetGameInfo(SceNpTrophyContext context, SceNpTrophyHandle handle, SceNpTrophyGameDetails *details, SceNpTrophyGameData *data);
extern int sceNpTrophyGetTrophyInfo(SceNpTrophyContext context, SceNpTrophyHandle handle, SceNpTrophyId trophyId, SceNpTrophyDetails *details, SceNpTrophyData *data);
extern int sceNpTrophyGetTrophyUnlockState(SceNpTrophyContext context, SceNpTrophyHandle handle, SceNpTrophyFlagArray *flags, int *count);
extern int sceNpTrophyUnlockTrophy(SceNpTrophyContext context, SceNpTrophyHandle handle, SceNpTrophyId trophyId, int *platinumId);

#endif
