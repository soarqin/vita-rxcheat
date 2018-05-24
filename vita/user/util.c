#include "util.h"

#include "liballoc.h"
#include "debug.h"

#include <vitasdk.h>
#include <taihen.h>

enum {
    MEMPOOL_MAX = 8,
};

static SceUID main_thread_id = 0;
static int main_thread_priority = 0;
static int main_thread_cpu_affinity = 0;
static volatile int main_thread_paused = 0;
static SceUID mempool_sema = 0;
static SceUID mempool_id[MEMPOOL_MAX];
static void *mempool_start[MEMPOOL_MAX];
static int mempool_count = 0;

int liballoc_lock() {
    return sceKernelWaitSema(mempool_sema, 1, NULL);
}

int liballoc_unlock() {
    return sceKernelSignalSema(mempool_sema, 1);
}

void* liballoc_alloc(size_t *sz) {
    static int seq = 0;
    char name[16];
    SceUID pool_id;
    void *pool_addr = NULL;
    int type[2], alloc_sz, i;
    if (mempool_count >= MEMPOOL_MAX) return NULL;
    sceClibSnprintf(name, 16, "rcsvr_mem_%d", seq);
    SceKernelFreeMemorySizeInfo fmsi;
    fmsi.size = sizeof(fmsi);
    sceKernelGetFreeMemorySize(&fmsi);
#ifdef RCSVR_DEBUG
    log_trace("Free memory: %X %X %X\n", fmsi.size_user, fmsi.size_phycont, fmsi.size_cdram);
#endif
    alloc_sz = *sz * 256 * 1024;
    type[0] = SCE_KERNEL_MEMBLOCK_TYPE_USER_RW;
    type[1] = SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW;
    if (fmsi.size_user < alloc_sz) {
        type[0] = SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW;
        type[1] = SCE_KERNEL_MEMBLOCK_TYPE_USER_RW;
    }
    for (i = 0; i < 2; ++i) {
        size_t new_sz = *sz;
        if (type[i] == SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW) {
            alloc_sz = (alloc_sz + 0xFFFFFU) & ~0xFFFFFU;
            new_sz = alloc_sz / (256 * 1024);
        }
        pool_id = sceKernelAllocMemBlock(name, type[i], alloc_sz, NULL);
        log_trace("sceKernelAllocMemBlock(%s): %s %08X %08X\n", type[i] == SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW ? "PHYCONT" : "USER", name, alloc_sz, pool_id);
        if (pool_id < 0) continue;
        if (sceKernelGetMemBlockBase(pool_id, &pool_addr) != SCE_OK) {
            sceKernelFreeMemBlock(pool_id);
            continue;
        }
        if (new_sz != *sz) *sz = new_sz;
        break;
    }
    if (pool_addr == NULL) return NULL;
    log_trace("sceKernelGetMemBlockBase: %08X\n", pool_addr);
    seq = (seq + 1) & 0xFFFF;
    mempool_id[mempool_count] = pool_id;
    mempool_start[mempool_count++] = pool_addr;
    return pool_addr;
}

int liballoc_free(void *ptr, size_t sz) {
    for (int i = 0; i < mempool_count; ++i) {
        if (mempool_start[i] == ptr && mempool_id[i] >= 0) {
            sceKernelFreeMemBlock(mempool_id[i]);
            --mempool_count;
            if (i < mempool_count) {
                mempool_id[i] = mempool_id[mempool_count];
                mempool_start[i] = mempool_start[mempool_count];
            }
            break;
        }
    }
    return 0;
}

void util_init() {
    // init memory pool
    mempool_sema = sceKernelCreateSema("rcsvr_mempool_sema", 0, 1, 1, 0);
    util_set_alloc(kmalloc, krealloc, kcalloc, kfree);

    sceShellUtilInitEvents(0);
}

void util_finish() {
    int count = mempool_count;
    mempool_count = 0;
    for (int i = 0; i < count; ++i) {
        if (mempool_id[i] >= 0) {
            sceKernelFreeMemBlock(mempool_id[i]);
        }
    }
}

static int power_lock_count = 0;

void util_lock_power() {
    if (power_lock_count++ == 0)
        sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
}

void util_unlock_power() {
    if (power_lock_count <= 0) return;
    if (--power_lock_count == 0)
        sceShellUtilUnlock(SCE_SHELL_UTIL_LOCK_TYPE_PS_BTN);
}

uint64_t util_gettick() {
    return sceKernelGetProcessTimeWide() / 1000ULL;
}

static int pause_blocking_thread(SceSize args, void *argv) {
    while (1) {
        if (!main_thread_paused) break;
    }
    return sceKernelExitDeleteThread(0);
}

int sceKernelChangeThreadCpuAffinityMask(SceUID threadId, int cpuAffinityMask);

void util_pause_main_thread() {
    SceKernelThreadInfo info;
    info.size = sizeof(SceKernelThreadInfo);
    if (main_thread_id == 0) {
        // find main thread
        SceUID thid = 0x40010001;
        SceUID curr = sceKernelGetThreadId();
        for(; thid <= 0x40010FFF; ++thid) {
            if (thid == curr) continue;
            int ret = sceKernelGetThreadInfo(thid, &info);
            if (ret < 0) continue;
            main_thread_id = thid;
            main_thread_priority = info.currentPriority;
            main_thread_cpu_affinity = info.currentCpuAffinityMask;
            break;
        }
        if (main_thread_id == 0) return;
    } else {
        int ret = sceKernelGetThreadInfo(main_thread_id, &info);
        if (ret >= 0) {
            main_thread_priority = info.currentPriority;
            main_thread_cpu_affinity = info.currentCpuAffinityMask;
        } else {
            main_thread_priority = 0x10000100;
            main_thread_cpu_affinity = 0;
        }
    }
    sceKernelChangeThreadPriority(main_thread_id, 0xBF);
    sceKernelChangeThreadCpuAffinityMask(main_thread_id, SCE_KERNEL_CPU_MASK_USER_0);
    log_trace("pause_main_thread: %X %X %X\n", main_thread_id, main_thread_priority, main_thread_cpu_affinity);
    while(1) {
        main_thread_paused = 1;
        SceUID thid = sceKernelCreateThread("rcsvr_pause_blocking_thread", pause_blocking_thread, 0x40, 0x4000, 0, SCE_KERNEL_CPU_MASK_USER_0, NULL);
        if (thid >= 0) sceKernelStartThread(thid, 0, NULL);
        sceKernelGetThreadInfo(main_thread_id, &info);
        if (info.status != SCE_THREAD_RUNNING) break;
        main_thread_paused = 0;
        sceKernelDelayThread(1000);
    }
}

void util_resume_main_thread() {
    if (main_thread_id == 0) return;
    if (!main_thread_paused) return;
    main_thread_paused = 0;
    log_trace("resume_main_thread: %X %X %X\n", main_thread_id, main_thread_priority, main_thread_cpu_affinity);
    sceKernelChangeThreadPriority(main_thread_id, main_thread_priority);
    sceKernelChangeThreadCpuAffinityMask(main_thread_id, main_thread_cpu_affinity);
}

int util_is_allocated(int id) {
    int i;
    for (i = 0; i < mempool_count; ++i) {
        if (mempool_id[i] >= 0 && mempool_id[i] == id) return 1;
    }
    return 0;
}

int util_readfile_by_line(const char *filename, int (*cb)(const char *name, void *arg), void *arg) {
    const int BUF_SIZE = 256;
    char buf[BUF_SIZE];
    SceUID f = sceIoOpen(filename, SCE_O_RDONLY, 0666);
    if (f < 0) return f;
    int pos = 0;
    while(1) {
        int i = pos, n = sceIoRead(f, buf + pos, BUF_SIZE - pos), s = 0;
        if (n <= 0) {
            if (pos > s) {
                buf[pos] = 0;
                cb(buf + s, arg);
            }
            break;
        }
        n += pos;
        while(1) {
            while(i < n && buf[i] != '\r' && buf[i] != '\n' && buf[i] != 0) ++i;
            if (i >= n) {
                if (s == 0) {
                    buf[n - 1] = 0;
                    if (cb(buf, arg) < 0)
                        goto closefile;
                    pos = 0;
                } else {
                    memmove(buf, buf + s, n - s);
                    pos = n - s;
                }
                break;
            }
            buf[i] = 0;
            if (i > s) {
                if (cb(buf + s, arg) < 0)
                    goto closefile;
            }
            ++i;
            while(i < n && (buf[i] == '\r' || buf[i] == '\n')) ++i;
            if (i >= n) {
                pos = 0;
                break;
            }
            pos = s = i;
        }
    }
closefile:
    sceIoClose(f);
    return 0;
}

void*(*my_alloc)(size_t);
void*(*my_realloc)(void*, size_t);
void*(*my_calloc)(size_t, size_t);
void (*my_free)(void*);

void util_set_alloc(void*(*a)(size_t), void*(*r)(void*, size_t), void*(*c)(size_t, size_t), void(*f)(void*)) {
    my_alloc = a; my_realloc = r; my_calloc = c; my_free = f;
}
