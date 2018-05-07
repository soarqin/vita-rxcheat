#include <vitasdkkern.h>
#include <taihen.h>

#define HOOKS_NUM 5
static SceUID hooks[HOOKS_NUM];
static tai_hook_ref_t refs[HOOKS_NUM];

// Allow ux0:data/rcsvr* and ux0:temp/rcsvr*
static inline int is_valid_filename(const char *filename) {
    if (filename[0] == 'u' && filename[1] == 'x' && filename[2] == '0' && filename[3] == ':'
        && ((filename[4] == 'd' && filename[5] == 'a' && filename[6] == 't' && filename[7] == 'a') || (filename[4] == 't' && filename[5] == 'e' && filename[6] == 'm' && filename[7] == 'p'))
        && filename[8] == '/' && filename[9] == 'r' && filename[10] == 'c' && filename[11] == 's' && filename[12] == 'v' && filename[13] == 'r'
        )
        return 1;
    return 0;
}

static SceUID start_preloaded_modules_patched(SceUID pid) {
    int ret = TAI_CONTINUE(int, refs[0], pid);
    if (ret >= 0) {
        char titleid[32];
        int result = 0;
        ksceKernelGetProcessTitleId(pid, titleid, 32);
        if (titleid[0] == 'P' && titleid[1] == 'C')
            ksceKernelLoadStartModuleForPid(pid, "ux0:/tai/rcsvr.suprx", 0, NULL, 0, NULL, &result);
    }
    return ret;
}

static int sceIoOpenForPidForDriver_patched(SceUID pid, const char *filename, int flag, SceIoMode mode) {
    int ret, state;
    ENTER_SYSCALL(state);
    if ((ret = TAI_CONTINUE(int, refs[1], pid, filename, flag, mode)) < 0 && is_valid_filename(filename)) {
        ret = ksceIoOpen(filename, flag, mode);
    }
    EXIT_SYSCALL(state);
    return ret;
}

typedef struct sceIoMkdirOpt {
  uint32_t unk_0;
  uint32_t unk_4;
} sceIoMkdirOpt;

static int _sceIoMkdir_patched(const char *dirname, SceIoMode mode, sceIoMkdirOpt opt) {
    int ret, state;
    ENTER_SYSCALL(state);
    if ((ret = TAI_CONTINUE(int, refs[2], dirname, mode, opt)) < 0) {
        char path[128];
        ksceKernelStrncpyUserToKernel(&path, (uintptr_t)dirname, sizeof(path));
        if (is_valid_filename(path))
            ret = ksceIoMkdir(path, mode);
    }
    EXIT_SYSCALL(state);
    return ret;
}

typedef struct sceIoRmdirOpt {
  uint32_t unk_0;
  uint32_t unk_4;
} sceIoRmdirOpt;

static int _sceIoRmdir_patched(const char *dirname, sceIoRmdirOpt* opt) {
    int ret, state;
    ENTER_SYSCALL(state);
    if ((ret = TAI_CONTINUE(int, refs[3], dirname, opt)) < 0) {
        char path[128];
        ksceKernelStrncpyUserToKernel(&path, (uintptr_t)dirname, sizeof(path));
        if (is_valid_filename(path))
            ret = ksceIoRmdir(path);
    }
    EXIT_SYSCALL(state);
    return ret;
}

typedef struct sceIoRemoveOpt {
  uint32_t unk_0;
  uint32_t unk_4;
} sceIoRemoveOpt;

static int _sceIoRemove_patched(const char *filename, sceIoRemoveOpt* opt) {
    int ret, state;
    ENTER_SYSCALL(state);
    if ((ret = TAI_CONTINUE(int, refs[4], filename, opt)) < 0) {
        char path[128];
        ksceKernelStrncpyUserToKernel(&path, (uintptr_t)filename, sizeof(path));
        if (is_valid_filename(path))
            ret = ksceIoRemove(path);
    }
    EXIT_SYSCALL(state);
    return ret;
}

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
    hooks[0] = taiHookFunctionExportForKernel(KERNEL_PID, 
        &refs[0], 
        "SceKernelModulemgr", 
        0xC445FA63, // SceModulemgrForKernel
        0x432DCC7A,
        start_preloaded_modules_patched);
    if (hooks[0] < 0) {
        hooks[0] = taiHookFunctionExportForKernel(KERNEL_PID, 
            &refs[0], 
            "SceKernelModulemgr", 
            0x92C9FFC2, // SceModulemgrForKernel
            0x998C7AE9,
            start_preloaded_modules_patched);
    }
    hooks[1] = taiHookFunctionExportForKernel(KERNEL_PID,
        &refs[1],
        "SceIofilemgr", // SceIofilemgrForKernel
        0x40FD29C7,
        0xC3D34965,
        sceIoOpenForPidForDriver_patched);
    hooks[2] = taiHookFunctionExportForKernel(KERNEL_PID,
        &refs[2],
        "SceIofilemgr", // SceIofilemgr
        0xF2FF276E,
        0x8F1ACC32,
        _sceIoMkdir_patched);
    hooks[3] = taiHookFunctionExportForKernel(KERNEL_PID,
        &refs[3],
        "SceIofilemgr", // SceIofilemgr
        0xF2FF276E,
        0xFFFB4D76,
        _sceIoRmdir_patched);
    hooks[4] = taiHookFunctionExportForKernel(KERNEL_PID,
        &refs[4],
        "SceIofilemgr", // SceIofilemgr
        0xF2FF276E,
        0x78955C65,
        _sceIoRemove_patched);
    return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
    int i;
    for (i = 0; i < HOOKS_NUM; ++i) {
        if (hooks[i] >= 0)
            taiHookReleaseForKernel(hooks[i], refs[i]);
    }
    return SCE_KERNEL_STOP_SUCCESS;
}
