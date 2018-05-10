#include "util.h"

#include "liballoc.h"
#include "debug.h"

#include <vitasdk.h>
#include <stdint.h>

static SceUID mempool_sema = 0;
static SceUID mempool_id[16];
static void *mempool_start[16];
static int mempool_count = 0;
static uint64_t start_tick = 0ULL;
static uint64_t resolution_of_tick = 0ULL;

int liballoc_lock() {
    return sceKernelWaitSema(mempool_sema, 1, NULL);
}

int liballoc_unlock() {
    return sceKernelSignalSema(mempool_sema, 1);
}

void* liballoc_alloc(size_t sz) {
    static int seq = 0;
    char name[16];
    SceUID pool_id;
    void *pool_addr;
    int type[2], alloc_sz, i;
    if (mempool_count >= 16) return NULL;
    sceClibSnprintf(name, 16, "rcsvr_mem_%d", seq);
    SceKernelFreeMemorySizeInfo fmsi;
    fmsi.size = sizeof(fmsi);
    sceKernelGetFreeMemorySize(&fmsi);
    alloc_sz = sz * 1024 * 1024;
    type[0] = SCE_KERNEL_MEMBLOCK_TYPE_USER_RW;
    type[1] = SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW;
    if (fmsi.size_user < alloc_sz) {
        type[0] = SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW;
        type[1] = SCE_KERNEL_MEMBLOCK_TYPE_USER_RW;
    }
    for (i = 0; i < 2; ++i) {
        pool_id = sceKernelAllocMemBlock(name, type[i], alloc_sz, NULL);
        log_trace("sceKernelAllocMemBlock(%s): %s %08X %08X\n", type[i] == SCE_KERNEL_MEMBLOCK_TYPE_USER_MAIN_PHYCONT_RW ? "PHYCONT" : "USER", name, sz, pool_id);
        if (pool_id < 0) continue;
        if (sceKernelGetMemBlockBase(pool_id, &pool_addr) != SCE_OK) {
            sceKernelFreeMemBlock(pool_id);
            continue;
        }
        break;
    }
    log_trace("sceKernelGetMemBlockBase: %08X\n", pool_addr);
    seq = (seq + 1) & 0xFFFF;
    mempool_id[mempool_count] = pool_id;
    mempool_start[mempool_count++] = pool_addr;
    return pool_addr;
}

int liballoc_free(void *ptr, size_t sz) {
    int i;
    for (i = 0; i < mempool_count; ++i) {
        if (mempool_start[i] == ptr) {
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

    // init time tick
    resolution_of_tick = sceRtcGetTickResolution() / 1000U;
    SceRtcTick tick;
    sceRtcGetCurrentTick(&tick);
    start_tick = tick.tick;
}

uint64_t util_gettick() {
    SceRtcTick tick;
    sceRtcGetCurrentTick(&tick);
    return (tick.tick - start_tick) / resolution_of_tick;
}

int util_is_allocated(int id) {
    int i;
    for (i = 0; i < mempool_count; ++i) {
        if (mempool_id[i] == id) return 1;
    }
    return 0;
}

void*(*my_alloc)(size_t);
void*(*my_realloc)(void*, size_t);
void*(*my_calloc)(size_t, size_t);
void (*my_free)(void*);

void util_set_alloc(void*(*a)(size_t), void*(*r)(void*, size_t), void*(*c)(size_t, size_t), void(*f)(void*)) {
    my_alloc = a; my_realloc = r; my_calloc = c; my_free = f;
}
