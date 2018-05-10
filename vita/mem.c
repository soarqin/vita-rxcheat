#include "mem.h"

#include "debug.h"
#include "util.h"

#include "kernel_api.h"

#include <vitasdk.h>
#include <stdlib.h>

typedef enum {
    st_none = 0,
    st_u32 = 1,
    st_u16 = 2,
    st_u8 = 3,
    st_u64 = 4,
    st_i32 = 5,
    st_i16 = 6,
    st_i8 = 7,
    st_i64 = 8,
    st_float = 9,
    st_double = 10,
    st_autouint = 21,
    st_autoint = 22,
} search_type;

#define LOCK_COUNT_MAX 0x80
#define MEM_RANGE_MAX (256)
#define STATIC_MEM_MAX (16)
#define STACK_MEM_MAX (32)
#define HEAP_MEM_MAX (MEM_RANGE_MAX-STATIC_MEM_MAX-STACK_MEM_MAX)

static memory_range mem_ranges[MEM_RANGE_MAX];
static memory_range *staticmem = mem_ranges, *stackmem = mem_ranges + STATIC_MEM_MAX, *heapmem = mem_ranges + STATIC_MEM_MAX + STACK_MEM_MAX;
static int static_cnt = 0, stack_cnt = 0, heap_cnt = 0;
static int mem_loaded = 0;
static int stype = 0, last_sidx = 0;
static SceUID searchMutex = -1, searchSema = -1;
static memlock_data lockdata[LOCK_COUNT_MAX];
static int lock_count = 0, mem_lock_ready = 0;

void mem_init() {
    sceIoMkdir("ux0:data/rcsvr/temp", 0777);
    searchMutex = sceKernelCreateMutex("rcsvr_search_mutex", 0, 0, 0);
    searchSema = sceKernelCreateSema("rcsvr_search_sema", 0, 0, 1, NULL);
}

void mem_finish() {
    if (searchSema >= 0) {
        sceKernelDeleteSema(searchSema);
        searchSema = -1;
    }
    if (searchMutex >= 0) {
        sceKernelDeleteMutex(searchMutex);
        searchMutex = -1;
    }
}

static int memory_range_compare(const void *a, const void *b) {
    uint32_t c1 = ((const memory_range*)a)->start;
    uint32_t c2 = ((const memory_range*)b)->start;
    if (c1 == c2) return 0;
    return c1 < c2 ? -1 : 1;
}

uint32_t mem_convert(uint32_t addr, int *read_only) {
    memory_range *mr = &mem_ranges[addr >> 24];
    uint32_t off = addr & 0xFFFFFFU;
    if (mr->start == 0 || off >= mr->size) return 0;
    if (read_only) *read_only = mr->flag & 1;
    return mr->start + off;
}

void mem_reload() {
    mem_loaded = 1;
    static_cnt = stack_cnt = 0;
    sceClibMemset(staticmem, 0, sizeof(memory_range) * STATIC_MEM_MAX);
    sceClibMemset(stackmem, 0, sizeof(memory_range) * STACK_MEM_MAX);
    SceUID modlist[256];
    int num_loaded = 256;
    int i, ret;
    ret = sceKernelGetModuleList(0xFF, modlist, &num_loaded);
    if (ret == 0) {
        for (i = 0; i < num_loaded && static_cnt < STATIC_MEM_MAX; ++i) {
            SceKernelModuleInfo info;
            if (sceKernelGetModuleInfo(modlist[i], &info) < 0) continue;
            if (sceClibStrncmp(info.path, "ux0:", 4) != 0
                && sceClibStrncmp(info.path, "app0:", 5) != 0
                && sceClibStrncmp(info.path, "gro0:", 5) != 0) continue;
            const char *rslash = strrchr(info.path, '/');
            if (rslash != NULL) {
                if (sceClibStrncmp(rslash, "/rcsvr.suprx", 12) == 0) continue;
                if (rslash > info.path + 10) {
                    if (sceClibStrncmp(rslash - 10, "sce_module/libfios2.suprx", 25) == 0) continue;
                    if (sceClibStrncmp(rslash - 10, "sce_module/libc.suprx", 21) == 0) continue;
                    if (sceClibStrncmp(rslash - 10, "sce_module/libult.suprx", 23) == 0) continue;
                }
            }
            log_trace("Module %s\n", info.path);
            int j;
            for (j = 0; j < 4; ++j) {
                if (info.segments[j].vaddr == 0 || (info.segments[j].perms & 4) != 4) continue;
                memory_range *mr = &staticmem[static_cnt++];
                mr->start = (uint32_t)info.segments[j].vaddr;
                mr->size = info.segments[j].memsz;
                mr->flag = (info.segments[j].perms & 2) == 0 ? 1 : 0;
                log_trace("    0x%08X 0x%08X 0x%08X 0x%08X\n", info.segments[j].vaddr, info.segments[j].memsz, info.segments[j].perms, info.segments[j].flags);
            }
        }
    }
    qsort(staticmem, static_cnt, sizeof(memory_range), memory_range_compare);
    for (i = 0; i < static_cnt; ++i) staticmem[i].index = i;
    SceKernelThreadInfo status;
    status.size = sizeof(SceKernelThreadInfo);
    SceUID thid = 0x40010001;
    SceUID curr = sceKernelGetThreadId();
    for(; thid <= 0x40010FFF && stack_cnt < STACK_MEM_MAX; ++thid) {
        if (thid == curr) continue;
        ret = sceKernelGetThreadInfo(thid, &status);
        if (ret < 0) continue;
        if (sceClibStrncmp(status.name, "rcsvr_", 6) == 0) continue;
        if (sceClibStrncmp(status.name, "SceFios", 7) == 0) continue;
        if (sceClibStrncmp(status.name, "SceGxm", 6) == 0) continue;
        if (sceClibStrncmp(status.name, "SceNp", 5) == 0) continue;
        if (sceClibStrncmp(status.name, "mempool_thread", 14) == 0) continue;
        if (sceClibStrncmp(status.name, "kuio_thread", 11) == 0) continue;
        log_trace("0x%08X %s 0x%08X 0x%08X\n", thid, status.name, status.stack, status.stackSize);
        memory_range *mr = &stackmem[stack_cnt++];
        mr->start = (uint32_t)status.stack;
        mr->size = status.stackSize;
    }
    qsort(stackmem, stack_cnt, sizeof(memory_range), memory_range_compare);
    for (i = 0; i < stack_cnt; ++i) stackmem[i].index = i + STATIC_MEM_MAX;
}

static void reload_heaps() {
    uint32_t addr = 0x80000000U;
    int i;
    heap_cnt = 0;
    sceClibMemset(heapmem, 0, sizeof(memory_range) * HEAP_MEM_MAX);
    while (addr < 0xA0000000U && heap_cnt < HEAP_MEM_MAX) {
        SceUID heap_memblock = sceKernelFindMemBlockByAddr((const void*)addr, 0);
        if (heap_memblock >= 0 && !util_is_allocated(heap_memblock)) {
            void* heap_addr;
            int ret = sceKernelGetMemBlockBase(heap_memblock, &heap_addr);
            if (ret >= 0) {
                SceKernelMemBlockInfo heap_info;
                heap_info.size = sizeof(SceKernelMemBlockInfo);
                ret = sceKernelGetMemBlockInfoByAddr(heap_addr, &heap_info);
                if (ret == 0) {
                    addr = (uint32_t)heap_info.mappedBase + heap_info.mappedSize;
                    if ((heap_info.type == SCE_KERNEL_MEMBLOCK_TYPE_USER_RW || heap_info.type == SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE) && (heap_info.access & 6) == 6) {
                        uint32_t base = (uint32_t)heap_info.mappedBase;
                        uint32_t size = heap_info.mappedSize;
                        log_trace("HEAP: %08X %08X %08X %08X %08X\n", base, size, heap_info.access, heap_info.memoryType, heap_info.type);
                        while (size > 0) {
                            memory_range *mr = &heapmem[heap_cnt++];
                            mr->start = (uint32_t)base;
                            if (size > 0x1000000U) {
                                mr->size = 0x1000000U;
                            } else {
                                mr->size = size;
                            }
                            base += mr->size;
                            size -= mr->size;
                        }
                    }
                    continue;
                }
            }
        }
        addr += 0x4000;
    }
    for (i = 0; i < heap_cnt; ++i) heapmem[i].index = i + STATIC_MEM_MAX + STACK_MEM_MAX;
}

static void single_search(SceUID outfile, memory_range *mr, const void *data, int size, void (*cb)(const uint32_t *addr, int count, int datalen)) {
    uint8_t *curr = (uint8_t*)mr->start;
    uint8_t *cend = curr + mr->size - size + 1;
    uint32_t addr[0x100];
    int addr_count = 0;
    log_debug("Searching from 0x%08X, size %d\n", curr, mr->size);
    for (; curr < cend; curr += size) {
        if (memcmp(data, (void*)curr, size) == 0) {
            log_debug("Found at %08X\n", curr);
            addr[addr_count++] = ((uint32_t)curr - mr->start) | (mr->index << 24);
            if (addr_count == 0x100) {
                cb(addr, addr_count, size);
                sceIoWrite(outfile, addr, 0x100 * 4);
                addr_count = 0;
            }
        }
    }
    if (addr_count > 0) {
        cb(addr, addr_count, size);
        sceIoWrite(outfile, addr, addr_count * 4);
    }
}

static void next_search(SceUID infile, SceUID outfile, const void *data, int size, void (*cb)(const uint32_t *addr, int count, int datalen)) {
    uint32_t old[0x100];
    uint32_t addr[0x100];
    int addr_count = 0;
    while(1) {
        SceSize i, n;
        n = sceIoRead(infile, old, 4 * 0x100);
        if (n < 0) break;
        n >>= 2;
        for (i = 0; i < n; ++i) {
            uint32_t raddr = mem_convert(old[i], NULL);
            log_debug("Check %08X\n", raddr);
            if ((old[i] >> 24) >= STATIC_MEM_MAX + STACK_MEM_MAX && sceKernelFindMemBlockByAddr((const void*)raddr, 0) < 0) continue;
            if (memcmp((void*)raddr, data, size) == 0) {
                log_debug("Found at %08X\n", raddr);
                addr[addr_count++] = old[i];
                if (addr_count == 0x100) {
                    cb(addr, addr_count, size);
                    sceIoWrite(outfile, addr, 0x100 * 4);
                    addr_count = 0;
                }
            }
        }
        if (n < 0x100) break;
    }
    if (addr_count > 0) {
        cb(addr, addr_count, size);
        sceIoWrite(outfile, addr, addr_count * 4);
    }
}

void mem_search(int type, int heap, const void *data, int len, void (*cb)(const uint32_t *addr, int count, int datalen)) {
    int size = mem_get_type_size(type, data);
    if (size > len) return;
    if (stype != type) {
        mem_reload();
        SceUID f = -1;
        char outfile[256];
        sceClibSnprintf(outfile, 256, "ux0:data/rcsvr/temp/%d", last_sidx);
        f = sceIoOpen(outfile, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
        int i;
        for (i = 0; i < static_cnt; ++i) {
            single_search(f, &staticmem[i], data, size, cb);
        }
        for (i = 0; i < stack_cnt; ++i) {
            single_search(f, &stackmem[i], data, size, cb);
        }
        if (heap) {
            reload_heaps();
            for (i = 0; i < heap_cnt; ++i) {
                single_search(f, &heapmem[i], data, size, cb);
            }
        }
        sceIoClose(f);
        stype = type;
    } else {
        SceUID f = -1;
        char infile[256];
        sceClibSnprintf(infile, 256, "ux0:data/rcsvr/temp/%d", last_sidx);
        f = sceIoOpen(infile, SCE_O_RDONLY, 0666);
        if (f < 0) {
            type = st_none;
            mem_search(type, heap, data, size, cb);
            return;
        }
        last_sidx ^= 1;
        SceUID of = -1;
        char outfile[256];
        sceClibSnprintf(outfile, 256, "ux0:data/rcsvr/temp/%d", last_sidx);
        of = sceIoOpen(outfile, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0666);
        next_search(f, of, data, size, cb);
        sceIoClose(of);
        sceIoClose(f);
        sceIoRemove(infile);
    }
}

void mem_search_reset() {
    sceIoRemove("ux0:data/rcsvr/temp/0");
    sceIoRemove("ux0:data/rcsvr/temp/1");
    last_sidx = 0;
    stype = st_none;
}

void mem_set(uint32_t addr, const void *data, int size) {
    int readonly;
    uint32_t raddr = mem_convert(addr, &readonly);
    if (raddr == 0) return;
    log_trace("Writing %d bytes to 0x%08X->0x%08X (%d)\n", size, addr, raddr, readonly);
    if (readonly)
        rcsvrMemcpy((void*)raddr, data, size);
    else
        sceClibMemcpy((void*)raddr, data, size);
}

typedef struct {
    int type;
    int heap;
    char buf[8];
    int len;
    void (*cb)(const uint32_t *addr, int count, int datalen);
    void (*cb_start)(int type);
    void (*cb_end)(int err);
} search_request;

volatile search_request search_req;

static int _search_thread(SceSize args, void *argp) {
    if (sceKernelTryLockMutex(searchMutex, 1) < 0) {
        search_req.cb_end(-1);
        return sceKernelExitDeleteThread(0);
    }
    int type = search_req.type;
    char buf[8];
    sceClibMemcpy(buf, (const char*)search_req.buf, 8);
    int len = search_req.len;
    int heap = search_req.heap;
    void (*cb)(const uint32_t *addr, int count, int datalen);
    void (*cb_start)(int type);
    void (*cb_end)();
    cb = search_req.cb;
    cb_start = search_req.cb_start;
    cb_end = search_req.cb_end;
    sceKernelSignalSema(searchSema, 1);
    cb_start(type);
    mem_search(type, heap, buf, len, cb);
    cb_end(0);
    sceKernelUnlockMutex(searchMutex, 1);
    return sceKernelExitDeleteThread(0);
}

void mem_start_search(int type, int heap, const char *buf, int len, void (*cb)(const uint32_t *addr, int count, int datalen), void (*cb_start)(int type), void (*cb_end)(int err)) {
    search_req.type = type;
    search_req.heap = heap;
    sceClibMemset((char*)search_req.buf, 0, 8);
    sceClibMemcpy((char*)search_req.buf, buf, len);
    search_req.len = len;
    search_req.cb = cb;
    search_req.cb_start = cb_start;
    search_req.cb_end = cb_end;
    SceUID thid = sceKernelCreateThread("rcsvr_search_thread", (SceKernelThreadEntry)_search_thread, 0x10000100, 0x10000, 0, 0, NULL);
    if (thid >= 0)
        sceKernelStartThread(thid, 0, NULL);
    sceKernelWaitSema(searchSema, 1, NULL);
}

int mem_read(uint32_t addr, void *data, int size) {
    if (!mem_loaded) {
        mem_reload();
    }
    uint32_t raddr = mem_convert(addr, NULL);
    if (raddr == 0) return -1;
    // SceUID heap_memblock = sceKernelFindMemBlockByAddr((const void*)addr, 0);
    // if (heap_memblock < 0) return -1;
    // void* heap_addr;
    // int ret = sceKernelGetMemBlockBase(heap_memblock, &heap_addr);
    // if (ret < 0) return -1;
    // SceKernelMemBlockInfo heap_info;
    // heap_info.size = sizeof(SceKernelMemBlockInfo);
    // ret = sceKernelGetMemBlockInfoByAddr(heap_addr, &heap_info);
    // if (ret < 0 || (heap_info.access & 6) != 6) return -1;
    // log_trace("HEAP: %08X %08X %08X %08X %08X\n", heap_info.mappedBase, heap_info.mappedSize, heap_info.access, heap_info.memoryType, heap_info.type);
    // uint32_t end = (uint32_t)heap_info.mappedBase + heap_info.mappedSize;
    // if (addr + size > end)
    //     size = end - addr;
    sceClibMemcpy(data, (void*)raddr, size);
    return size;
}

int mem_list(memory_range *range, int size, int heap) {
    int res = 0;
    int i;
    for (i = 0; i < static_cnt && res < size; ++i) {
        range[res++] = staticmem[i];
    }
    for (i = 0; i < stack_cnt && res < size; ++i) {
        range[res++] = stackmem[i];
    }
    if (!heap) return res;
    reload_heaps();
    for (i = 0; i < heap_cnt && res < size; ++i) {
        range[res++] = heapmem[i];
    }
    return res;
}

static inline int mem_is_valid(uint32_t addr) {
    int i;
    for (i = 0; i < static_cnt; ++i) {
        if (addr >= staticmem[i].start && addr < staticmem[i].start + staticmem[i].size)
            return 1;
    }
    for (i = 0; i < stack_cnt; ++i) {
        if (addr >= stackmem[i].start && addr < stackmem[i].start + stackmem[i].size)
            return 1;
    }
    if (sceKernelFindMemBlockByAddr((const void*)addr, 0) < 0)
        return 0;
    return 1;
}

void mem_lockdata_clear() {
    lock_count = 0;
}

void mem_lockdata_begin() {
    log_trace("lock begin\n");
    mem_lockdata_clear();
    mem_lock_ready = 0;
    if (!mem_loaded) {
        mem_reload();
    }
}

void mem_lockdata_add(uint32_t addr, uint8_t size, const char *data) {
    if (lock_count >= LOCK_COUNT_MAX) return;
    uint32_t raddr = mem_convert(addr, NULL);
    if (raddr == 0) return;
    memlock_data *ld = &lockdata[lock_count++];
    ld->address = raddr;
    ld->size = size;
    log_trace("lock add %08X->%08X %d\n", addr, raddr, size);
    sceClibMemcpy(ld->data, data, 8);
}

void mem_lockdata_end() {
    log_trace("lock end\n");
    mem_lock_ready = 1;
}

const memlock_data *mem_lockdata_query(int *count) {
    *count = lock_count;
    return lockdata;
}

void mem_lockdata_process() {
    if (!mem_lock_ready) return;
    int i;
    for (i = 0; i < lock_count; ++i) {
        memlock_data *ld = &lockdata[i];
        sceClibMemcpy((void*)ld->address, ld->data, ld->size);
    }
}

int mem_get_type_size(int type, const void *data) {
    switch(type) {
        case st_autoint: {
            int64_t val = *(int64_t*)data;
            if (val >= 0x80000000LL || val < -0x80000000LL) return 8;
            if (val >= 0x8000LL || val < -0x8000LL) return 4;
            if (val >= 0x80LL || val < -0x80LL) return 2;
            return 1;
        }
        case st_autouint: {
            int64_t val = *(int64_t*)data;
            if (val >= 0x100000000ULL) return 8;
            if (val >= 0x10000ULL) return 4;
            if (val >= 0x100ULL) return 2;
            return 1;
        }
        case st_u32: case st_i32: case st_float: return 4;
        case st_u16: case st_i16: return 2;
        case st_u8: case st_i8: return 1;
        case st_u64: case st_i64: case st_double: return 8;
    }
    return 0;
}
