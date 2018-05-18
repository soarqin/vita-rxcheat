#ifndef __NP_TROPHY_H_
#define __NP_TROPHY_H_

#include <psp2/types.h>
#include <psp2/rtc.h>
#include <psp2/common_dialog.h>
#include <psp2/kernel/clib.h>

typedef SceInt32 SceNpTrophyHandle;
typedef SceInt32 SceNpTrophyContext;
typedef SceInt32 SceNpTrophyId;

enum {
    SCE_NP_COMMUNICATION_PASSPHRASE_SIZE= 128,
    SCE_NP_COMMUNICATION_SIGNATURE_SIZE = 160,
    SCE_NP_TROPHY_GAME_TITLE_MAX_SIZE   = 128,
    SCE_NP_TROPHY_GAME_DESCR_MAX_SIZE   = 1024,
    SCE_NP_TROPHY_GROUP_TITLE_MAX_SIZE  = 128,
    SCE_NP_TROPHY_GROUP_DESCR_MAX_SIZE  = 1024,
    SCE_NP_TROPHY_NAME_MAX_SIZE         = 128,
    SCE_NP_TROPHY_DESCR_MAX_SIZE        = 1024,
    SCE_NP_TROPHY_FLAG_SETSIZE          = 128,
    SCE_NP_TROPHY_FLAG_BITS_SHIFT       = 5,
};

/* SceNpCommunicationId is defined in VitaSDK already */
typedef struct SceNpCommunicationId SceNpCommunicationId;
typedef struct SceNpCommunicationPassphrase {
	SceUChar8 data[SCE_NP_COMMUNICATION_PASSPHRASE_SIZE];
} SceNpCommunicationPassphrase;
typedef struct SceNpCommunicationSignature {
    SceUChar8 data[SCE_NP_COMMUNICATION_SIGNATURE_SIZE];
} SceNpCommunicationSignature;
typedef struct SceNpCommunicationConfig {
    const SceNpCommunicationId         *commId;
    const SceNpCommunicationPassphrase *commPassphrase;
    const SceNpCommunicationSignature  *commSignature;
} SceNpCommunicationConfig;

typedef enum SceNpTrophyGrade {
    SCE_NP_TROPHY_GRADE_UNKNOWN        = 0,
    SCE_NP_TROPHY_GRADE_PLATINUM       = 1,
    SCE_NP_TROPHY_GRADE_GOLD           = 2,
    SCE_NP_TROPHY_GRADE_SILVER         = 3,
    SCE_NP_TROPHY_GRADE_BRONZE         = 4,
} SceNpTrophyGrade;

typedef struct SceNpTrophyGameDetails {
    SceSize   size;
    SceUInt32 numGroups;
    SceUInt32 numTrophies;
    SceUInt32 numPlatinum;
    SceUInt32 numGold;
    SceUInt32 numSilver;
    SceUInt32 numBronze;
    char      title[SCE_NP_TROPHY_GAME_TITLE_MAX_SIZE];
    char      description[SCE_NP_TROPHY_GAME_DESCR_MAX_SIZE];
} SceNpTrophyGameDetails;

typedef struct SceNpTrophyGameData {
    SceSize   size;
    SceUInt32 unlockedTrophies;
    SceUInt32 unlockedPlatinum;
    SceUInt32 unlockedGold;
    SceUInt32 unlockedSilver;
    SceUInt32 unlockedBronze;
    SceUInt32 progressPercentage;
} SceNpTrophyGameData;

typedef struct SceNpTrophyGroupDetails {
    SceSize   size;
    SceInt32  groupId;
    SceUInt32 numTrophies;
    SceUInt32 numPlatinum;
    SceUInt32 numGold;
    SceUInt32 numSilver;
    SceUInt32 numBronze;
    char      title[SCE_NP_TROPHY_GROUP_TITLE_MAX_SIZE];
    char      description[SCE_NP_TROPHY_GROUP_DESCR_MAX_SIZE];
} SceNpTrophyGroupDetails;

typedef struct SceNpTrophyGroupData {
    SceSize   size;
    SceInt32  groupId;
    SceUInt32 unlockedTrophies;
    SceUInt32 unlockedPlatinum;
    SceUInt32 unlockedGold;
    SceUInt32 unlockedSilver;
    SceUInt32 unlockedBronze;
    SceUInt32 progressPercentage;
} SceNpTrophyGroupData;

typedef struct SceNpTrophyDetails {
    SceSize       size;
    SceNpTrophyId trophyId;
    SceInt32      trophyGrade;
    SceInt32      groupId;
    SceInt32      hidden;
    char          name[SCE_NP_TROPHY_NAME_MAX_SIZE];
    char          description[SCE_NP_TROPHY_DESCR_MAX_SIZE];
} SceNpTrophyDetails;

typedef struct SceNpTrophyData {
    SceSize       size;
    SceNpTrophyId trophyId;
    SceInt32      unlocked;
    SceInt32      unk0;
    SceRtcTick    timestamp;
} SceNpTrophyData;

typedef struct SceNpTrophyFlagArray {
    SceUInt32 flag_bits[SCE_NP_TROPHY_FLAG_SETSIZE >> SCE_NP_TROPHY_FLAG_BITS_SHIFT];
} SceNpTrophyFlagArray;

extern int sceNpInit(const SceNpCommunicationConfig *commConf, void *opt);
extern int sceNpTrophyInit(void *opt);
extern int sceNpTrophyTerm(void);
extern int sceNpTrophyCreateHandle(SceNpTrophyHandle *handle);
extern int sceNpTrophyDestroyHandle(SceNpTrophyHandle handle);
extern int sceNpTrophyAbortHandle(SceNpTrophyHandle handle);
extern int sceNpTrophyCreateContext(SceNpTrophyContext *context, const SceNpCommunicationId *commId, const SceNpCommunicationSignature *commSign, SceUInt64 options);
extern int sceNpTrophyDestroyContext(SceNpTrophyContext context);
extern int sceNpTrophyUnlockTrophy(SceNpTrophyContext context, SceNpTrophyHandle handle, SceNpTrophyId trophyId, int *platinumId);
extern int sceNpTrophyGetTrophyUnlockState(SceNpTrophyContext context, SceNpTrophyHandle handle, SceNpTrophyFlagArray *flags, int *count);
extern int sceNpTrophyGetGameInfo(SceNpTrophyContext context, SceNpTrophyHandle handle, SceNpTrophyGameDetails *details, SceNpTrophyGameData *data);
extern int sceNpTrophyGetGroupInfo(SceNpTrophyContext context, SceNpTrophyHandle handle, SceInt32 groupId, SceNpTrophyGroupDetails *details, SceNpTrophyGroupData *data);
extern int sceNpTrophyGetTrophyInfo(SceNpTrophyContext context, SceNpTrophyHandle handle, SceNpTrophyId trophyId, SceNpTrophyDetails *details, SceNpTrophyData *data);
extern int sceNpTrophyGetGameIcon(SceNpTrophyContext context, SceNpTrophyHandle handle, void *buffer, SceSize *size);
extern int sceNpTrophyGetGroupIcon(SceNpTrophyContext context, SceNpTrophyHandle handle, SceInt32 groupId, void *buffer, SceSize *size);
extern int sceNpTrophyGetTrophyIcon(SceNpTrophyContext context, SceNpTrophyHandle handle, SceNpTrophyId trophyId, void *buffer, SceSize *size);

typedef struct SceNpTrophySetupDialogParam {
    SceUInt32 sdkVersion;
    SceCommonDialogParam commonParam;
    SceNpTrophyContext context;
    SceUInt32 options;
    SceUInt8 reserved[128];
} SceNpTrophySetupDialogParam;
typedef struct SceNpTrophySetupDialogResult {
    int result;
    SceUInt8 reserved[128];
} SceNpTrophySetupDialogResult;

extern int sceNpTrophySetupDialogInit(SceNpTrophySetupDialogParam *param);
extern SceCommonDialogStatus sceNpTrophySetupDialogGetStatus(void);
extern int sceNpTrophySetupDialogGetResult(SceNpTrophySetupDialogResult *result);

#endif
