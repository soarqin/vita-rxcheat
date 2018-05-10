#include <vitasdkkern.h>
#include <taihen.h>

#include <libk/string.h>

#include "kernel_debug.h"

static int kernel_api_inited = 0;

int module_get_export_func(SceUID pid, const char *modname, uint32_t libnid, uint32_t funcnid, uintptr_t *func);
int (*sceKernelRxMemcpyKernelToUserForPidForKernel)(SceUID pid, uintptr_t dst, const void *src, size_t len);
void (*sceKernelCpuDcacheWritebackInvalidateRangeForKernel)(void *addr, unsigned int size);
void (*sceKernelCpuIcacheAndL2WritebackInvalidateRangeForKernel)(const void *ptr, size_t len);

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
    cache_flush(dst_pid, (uintptr_t)dst, size);
    return ret;
}

void rcsvrMemcpy(void *addr, const void *data, int data_len) {
    module_get_export_func(0, 0, 0, 0, 0);
    uint32_t state;
    ENTER_SYSCALL(state);
    SceUID pid = ksceKernelGetProcessId();
    char buf[0x40];
    int len = data_len;
    uintptr_t src = (uintptr_t)data;
    uintptr_t dst = (uintptr_t)addr;
    while (len > 0) {
        if (len > 0x40) {
            ksceKernelMemcpyUserToKernel(buf, src, 0x40);
            tai_force_memcpy(pid, (void*)dst, buf, 0x40);
            len -= 0x40;
            src += 0x40;
            dst += 0x40;
        } else {
            ksceKernelMemcpyUserToKernel(buf, src, len);
            tai_force_memcpy(pid, (void*)dst, buf, len);
            len = 0;
        }
    }
    EXIT_SYSCALL(state);
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
