#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/kernel/modulemgr.h>
#include <taihen.h>

static tai_hook_ref_t start_preloaded_modules_hook;

static SceUID hooks[1];

static SceUID start_preloaded_modules_patched(SceUID pid) {
  char titleid[32];
  int ret = TAI_CONTINUE(int, start_preloaded_modules_hook, pid);
  if (pid < 0) return ret;
  int result = 0;
  ksceKernelGetProcessTitleId(pid, titleid, 32);
  if (titleid[0] == 'P' && titleid[1] == 'C')
    ksceKernelLoadStartModuleForPid(pid, "ux0:/tai/rcsvr.suprx", 0, NULL, 0, NULL, &result);
  return pid;
}

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
  hooks[0] = taiHookFunctionExportForKernel(KERNEL_PID, 
                                              &start_preloaded_modules_hook, 
                                              "SceKernelModulemgr", 
                                              0xC445FA63, // SceModulemgrForKernel
                                              0x432DCC7A,
                                              start_preloaded_modules_patched);
  if (hooks[0] < 0) {
    hooks[0] = taiHookFunctionExportForKernel(KERNEL_PID, 
                                                &start_preloaded_modules_hook, 
                                                "SceKernelModulemgr", 
                                                0x92C9FFC2, // SceModulemgrForKernel
                                                0x998C7AE9,
                                                start_preloaded_modules_patched);
  }
  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
  if (hooks[0] >= 0)
    taiHookReleaseForKernel(hooks[0], start_preloaded_modules_hook);
  return SCE_KERNEL_STOP_SUCCESS;
}
