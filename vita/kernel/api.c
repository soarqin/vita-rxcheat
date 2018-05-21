#include "api.h"

#include "debug.h"

#include <vitasdkkern.h>
#include <taihen.h>

#include <libk/string.h>

static int kernel_api_inited = 0;

int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);
int (*sceKernelRxMemcpyKernelToUserForPidForKernel)(SceUID pid, uintptr_t dst, const void *src, size_t len);
void (*sceKernelCpuDcacheWritebackInvalidateRangeForKernel)(void *addr, unsigned int size);
void (*sceKernelCpuIcacheAndL2WritebackInvalidateRangeForKernel)(const void *ptr, size_t len);

int ksceKernelMemcpyUserToUserForPid(SceUID pid, void *dst, const void *src, SceSize size);

/*
  Codes from taiHEN - patch.c
  Used to force write memory to user space for readonly blocks
*/
void cache_flush(SceUID pid, uintptr_t vma, size_t len) {
    uintptr_t vma_align;
    int flags;
    int my_context[3];
    int ret;
    int *other_context;
    int dacr;

    if (pid == KERNEL_PID) return;

    vma_align = vma & ~0x1F;
    len = ((vma + len + 0x1F) & ~0x1F) - vma_align;

    // TODO: Take care of SHARED_PID
    flags = ksceKernelCpuDisableInterrupts();
    ksceKernelCpuSaveContext(my_context);
    ret = ksceKernelGetPidContext(pid, (SceKernelProcessContext **)&other_context);
    if (ret >= 0) {
        ksceKernelCpuRestoreContext(other_context);
        asm volatile ("mrc p15, 0, %0, c3, c0, 0" : "=r" (dacr));
        asm volatile ("mcr p15, 0, %0, c3, c0, 0" :: "r" (0x15450FC3));
        sceKernelCpuDcacheWritebackInvalidateRangeForKernel((void *)vma_align, len);
        sceKernelCpuIcacheAndL2WritebackInvalidateRangeForKernel((void *)vma_align, len);
        asm volatile ("mcr p15, 0, %0, c3, c0, 0" :: "r" (dacr));
    }
    ksceKernelCpuRestoreContext(my_context);
    ksceKernelCpuEnableInterrupts(flags);
    asm volatile ("isb" ::: "memory");
}

static int tai_force_memcpy(SceUID dst_pid, void *dst, const void *src, size_t size) {
    int ret;
    if (dst_pid == KERNEL_PID)
        return -1;
    ret = sceKernelRxMemcpyKernelToUserForPidForKernel(dst_pid, (uintptr_t)dst, src, size);
    if (ret < 0) return ret;
    return ret;
}

int rcsvrMemcpyForce(void *dstp, const void *srcp, int size, int flush) {
    uint32_t state;
    int ret = 0;
    ENTER_SYSCALL(state);
    SceUID pid = ksceKernelGetProcessId();
    char buf[0x100];
    int len = size;
    uintptr_t src = (uintptr_t)srcp;
    uintptr_t dst = (uintptr_t)dstp;
    while (len > 0) {
        if (len > 0x100) {
            if (ksceKernelMemcpyUserToKernel(buf, src, 0x100) < 0) { ret = -1; break; }
            if (tai_force_memcpy(pid, (void*)dst, buf, 0x100) < 0) { ret = -1; break; }
            if (flush) cache_flush(pid, dst, 0x100);
            len -= 0x100;
            src += 0x100;
            dst += 0x100;
        } else {
            if (ksceKernelMemcpyUserToKernel(buf, src, len) < 0) { ret = -1; break; }
            if (tai_force_memcpy(pid, (void*)dst, buf, len) < 0) { ret = -1; break; }
            if (flush) cache_flush(pid, dst, len);
            len = 0;
        }
    }
    EXIT_SYSCALL(state);
    return ret;
}

int rcsvrMemcpy(void *dst, const void *src, int size) {
    uint32_t state;
    ENTER_SYSCALL(state);
    int ret = ksceKernelMemcpyUserToUserForPid(ksceKernelGetProcessId(), dst, src, size);
    EXIT_SYSCALL(state);
    return ret;
}

SceUID rcsvrIoDopen(const char *dirname) {
    uint32_t state;
    ENTER_SYSCALL(state);
    char path[128];
    ksceKernelStrncpyUserToKernel(path, (uintptr_t)dirname, sizeof(path));
    SceUID ret = ksceIoDopen(path);
    EXIT_SYSCALL(state);
    return ret;
}

int rcsvrIoDread(SceUID fd, SceIoDirent *dir) {
    uint32_t state;
    SceIoDirent out;
    ENTER_SYSCALL(state);
    int ret = ksceIoDread(fd, &out);
    if (ret >= 0)
        ksceKernelMemcpyKernelToUser((uintptr_t)dir, &out, sizeof(SceIoDirent));
    EXIT_SYSCALL(state);
    return ret;
}

int rcsvrIoDclose(SceUID fd) {
    uint32_t state;
    ENTER_SYSCALL(state);
    int ret = ksceIoDclose(fd);
    EXIT_SYSCALL(state);
    return ret;
}

void kernel_api_init() {
    int ret;
    if (kernel_api_inited) return;

    ret = module_get_export_func(KERNEL_PID, "SceSysmem", 0x63A519E5, 0x30931572, (uintptr_t*)&sceKernelRxMemcpyKernelToUserForPidForKernel);
    if (ret < 0) { // 3.65/3.67
        ret = module_get_export_func(KERNEL_PID, "SceSysmem", 0x02451F0F, 0x2995558D, (uintptr_t*)&sceKernelRxMemcpyKernelToUserForPidForKernel);
        if (ret < 0) return;
        ret = module_get_export_func(KERNEL_PID, "SceSysmem", 0xA5195D20, 0x4F442396, (uintptr_t*)&sceKernelCpuDcacheWritebackInvalidateRangeForKernel);
        if (ret < 0) return;
        ret = module_get_export_func(KERNEL_PID, "SceSysmem", 0xA5195D20, 0x73E895EA, (uintptr_t*)&sceKernelCpuIcacheAndL2WritebackInvalidateRangeForKernel);
        if (ret < 0) return;
    } else { // 3.60
        ret = module_get_export_func(KERNEL_PID, "SceSysmem", 0x54BF2BAB, 0x6BA2E51C, (uintptr_t*)&sceKernelCpuDcacheWritebackInvalidateRangeForKernel);
        if (ret < 0) return;
        ret = module_get_export_func(KERNEL_PID, "SceSysmem", 0x54BF2BAB, 0x19F17BD0, (uintptr_t*)&sceKernelCpuIcacheAndL2WritebackInvalidateRangeForKernel);
        if (ret < 0) return;
    }
    log_trace("sceKernelRxMemcpyKernelToUserForPidForKernel: %08X\n", sceKernelRxMemcpyKernelToUserForPidForKernel);
    log_trace("sceKernelCpuDcacheWritebackInvalidateRangeForKernel: %08X\n", sceKernelCpuDcacheWritebackInvalidateRangeForKernel);
    log_trace("sceKernelCpuIcacheAndL2WritebackInvalidateRangeForKernel: %08X\n", sceKernelCpuIcacheAndL2WritebackInvalidateRangeForKernel);

    kernel_api_inited = 1;
}
