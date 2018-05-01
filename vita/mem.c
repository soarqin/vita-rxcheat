#include "mem.h"

#include "debug.h"

#include <vitasdk.h>
#include "kio.h"
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

static memory_range staticmem[64], stackmem[32], blockmem[1024];
static int static_sz = 0, stack_sz = 0, block_sz = 0;
static int mem_loaded = 0;
static int stype = 0, last_sidx = 0;
static SceUID searchMutex = -1, searchSema = -1;
static memlock_data lockdata[LOCK_COUNT_MAX];
static int lock_count = 0;

void mem_init() {
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

static void mem_load() {
    mem_loaded = 1;
    static_sz = stack_sz = 0;
    SceUID modlist[256];
    int num_loaded = 256;
    int ret;
    ret = sceKernelGetModuleList(0xFF, modlist, &num_loaded);
    if (ret == 0) {
        int i;
        for (i = 0; i < num_loaded && static_sz < 64; ++i) {
            SceKernelModuleInfo info;
            if (sceKernelGetModuleInfo(modlist[i], &info) < 0) continue;
            if (strncmp(info.path, "ux0:", 4) != 0 && strncmp(info.path, "app0:", 5) != 0 && strncmp(info.path, "gro0:", 5) != 0) continue;
            const char *rslash = strrchr(info.path, '/');
            if (rslash != NULL && strcmp(rslash, "/rcsvr.suprx") == 0) continue;
            log_debug("Module %s\n", info.path);
            int j;
            for (j = 0; j < 4; ++j) {
                if (info.segments[j].vaddr == 0 || (info.segments[j].perms & 6) != 6) continue;
                log_debug("    0x%08X 0x%08X 0x%08X 0x%08X\n", info.segments[j].vaddr, info.segments[j].memsz, info.segments[j].perms, info.segments[j].flags);
                memory_range *mr = &staticmem[static_sz++];
                mr->start = (uint32_t)info.segments[j].vaddr;
                mr->size = info.segments[j].memsz;
            }
        }
    }
    SceKernelThreadInfo status;
    status.size = sizeof(SceKernelThreadInfo);
    SceUID thid = 0x40010001;
    SceUID curr = sceKernelGetThreadId();
    for(; thid <= 0x40010FFF && stack_sz < 32; ++thid) {
        if (thid == curr) continue;
        ret = sceKernelGetThreadInfo(thid, &status);
        if (ret < 0 || strncmp(status.name, "rcsvr_", 6) == 0) continue;
        log_debug("0x%08X %s 0x%08X 0x%08X\n", thid, status.name, status.stack, status.stackSize);
        memory_range *mr = &stackmem[stack_sz++];
        mr->start = (uint32_t)status.stack;
        mr->size = status.stackSize;
    }
}

static void reload_blocks() {
    block_sz = 0;
    uint32_t addr = 0x80000000U;
    while(addr < 0xA0000000U && block_sz < 1024) {
        SceUID heap_memblock = sceKernelFindMemBlockByAddr((const void*)addr, 0);
        if (heap_memblock >= 0) {
            void* heap_addr;
            int ret = sceKernelGetMemBlockBase(heap_memblock, &heap_addr);
            if (ret >= 0){
                SceKernelMemBlockInfo heap_info;
                heap_info.size = sizeof(SceKernelMemBlockInfo);
                ret = sceKernelGetMemBlockInfoByAddr(heap_addr, &heap_info);
                if (ret == 0 && (heap_info.access & 6) == 6) {
                    log_debug("%08X %08X %08X %08X %08X\n", heap_info.mappedBase, heap_info.mappedSize, heap_info.access, heap_info.memoryType, heap_info.type);
                    addr = (uint32_t)heap_info.mappedBase + heap_info.mappedSize;
                    memory_range *mr = &blockmem[block_sz++];
                    mr->start = (uint32_t)heap_info.mappedBase & ~0x80000000U;
                    mr->size = heap_info.mappedSize;
                    continue;
                }
            }
        }
        addr += 0x8000;
    }
}

static void single_search(SceUID outfile, memory_range *mr, const void *data, int size, void (*cb)(const uint32_t *addr, int count, int datalen)) {
    int is_heap = !(mr->start & 0x80000000U);
    uint8_t *curr = (uint8_t*)(mr->start | 0x80000000U);
    uint8_t *cend = curr + mr->size - size + 1;
    uint32_t addr[0x100];
    int addr_count = 0;
    log_debug("Searching from 0x%08X, size %d\n", curr, mr->size);
    for (; curr < cend; curr += size) {
        if (memcmp(data, (void*)curr, size) == 0) {
            log_debug("Found at %08X\n", curr);
            addr[addr_count++] = is_heap ? ((uint32_t)curr & ~0x80000000U) : (uint32_t)curr;
            if (addr_count == 0x100) {
                cb(addr, addr_count, size);
                kIoWrite(outfile, addr, 0x100 * 4, NULL);
                addr_count = 0;
            }
        }
    }
    if (addr_count > 0) {
        cb(addr, addr_count, size);
        kIoWrite(outfile, addr, addr_count * 4, NULL);
    }
}

static void next_search(SceUID infile, SceUID outfile, const void *data, int size, void (*cb)(const uint32_t *addr, int count, int datalen)) {
    uint32_t old[0x100];
    uint32_t addr[0x100];
    int addr_count = 0;
    while(1) {
        SceSize i, n;
        kIoRead(infile, old, 4 * 0x100, &n);
        n >>= 2;
        for (i = 0; i < n; ++i) {
            int is_heap = !(old[i] & 0x80000000U);
            uint32_t raddr = old[i] | 0x80000000U;
            log_debug("Check %08X\n", raddr);
            if (is_heap && sceKernelFindMemBlockByAddr((const void*)raddr, 0) < 0) continue;
            if (memcmp((void*)raddr, data, size) == 0) {
                log_debug("Found at %08X\n", raddr);
                addr[addr_count++] = old[i];
                if (addr_count == 0x100) {
                    cb(addr, addr_count, size);
                    kIoWrite(outfile, addr, 0x100 * 4, NULL);
                    addr_count = 0;
                }
            }
        }
        if (n < 0x100) break;
    }
    if (addr_count > 0) {
        cb(addr, addr_count, size);
        kIoWrite(outfile, addr, addr_count * 4, NULL);
    }
}

void mem_search(int type, int heap, const void *data, int len, void (*cb)(const uint32_t *addr, int count, int datalen)) {
    int size = mem_get_type_size(type, data);
    if (size > len) return;
    if (!mem_loaded) {
        mem_load();
    }
    if (stype != type) {
        SceUID f = -1;
        char outfile[256];
        sceClibSnprintf(outfile, 256, "ux0:/data/rcsvr_%d.tmp", last_sidx);
        kIoOpen(outfile, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, &f);
        int i;
        for (i = 0; i < static_sz; ++i) {
            single_search(f, &staticmem[i], data, size, cb);
        }
        for (i = 0; i < stack_sz; ++i) {
            single_search(f, &stackmem[i], data, size, cb);
        }
        if (heap) {
            reload_blocks();
            for (i = 0; i < block_sz; ++i) {
                single_search(f, &blockmem[i], data, size, cb);
            }
        }
        kIoClose(f);
        // reload_blocks();
        stype = type;
    } else {
        SceUID f = -1;
        char infile[256];
        sceClibSnprintf(infile, 256, "ux0:/data/rcsvr_%d.tmp", last_sidx);
        if (kIoOpen(infile, SCE_O_RDONLY, &f) < 0) {
            type = st_none;
            mem_search(type, heap, data, size, cb);
            return;
        }
        last_sidx ^= 1;
        SceUID of = -1;
        char outfile[256];
        sceClibSnprintf(outfile, 256, "ux0:/data/rcsvr_%d.tmp", last_sidx);
        kIoOpen(outfile, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, &of);
        next_search(f, of, data, size, cb);
        kIoClose(of);
        kIoClose(f);
        kIoRemove(infile);
    }
}

void mem_search_reset() {
    kIoRemove("ux0:/data/rcsvr_0.tmp");
    kIoRemove("ux0:/data/rcsvr_1.tmp");
    last_sidx = 0;
    stype = st_none;
}

void mem_set(uint32_t addr, const void *data, int size) {
    log_debug("Writing %d bytes to 0x%08X\n", size, addr);
    memcpy((void *)(addr | 0x80000000U), data, size);
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
    memcpy(buf, (const char*)search_req.buf, 8);
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
    memset((char*)search_req.buf, 0, 8);
    memcpy((char*)search_req.buf, buf, len);
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
        mem_load();
    }
    int i;
    for (i = 0; i < static_sz; ++i) {
        if (addr >= staticmem[i].start && addr < staticmem[i].start + staticmem[i].size) {
            uint32_t end = staticmem[i].start + staticmem[i].size;
            if (addr + size > end) size = end - addr;
            memcpy(data, (void*)addr, size);
            return size;
        }
    }
    for (i = 0; i < stack_sz; ++i) {
        if (addr >= stackmem[i].start && addr < stackmem[i].start + stackmem[i].size) {
            uint32_t end = stackmem[i].start + stackmem[i].size;
            if (addr + size > end) size = end - addr;
            memcpy(data, (void*)addr, size);
            return size;
        }
    }
    SceUID heap_memblock = sceKernelFindMemBlockByAddr((const void*)addr, 0);
    if (heap_memblock < 0) return -1;
    void* heap_addr;
    int ret = sceKernelGetMemBlockBase(heap_memblock, &heap_addr);
    if (ret < 0) return -1;
    SceKernelMemBlockInfo heap_info;
    heap_info.size = sizeof(SceKernelMemBlockInfo);
    ret = sceKernelGetMemBlockInfoByAddr(heap_addr, &heap_info);
    if (ret < 0 || (heap_info.access & 6) != 6) return -1;
    log_debug("%08X %08X %08X %08X %08X\n", heap_info.mappedBase, heap_info.mappedSize, heap_info.access, heap_info.memoryType, heap_info.type);
    uint32_t end = (uint32_t)heap_info.mappedBase + heap_info.mappedSize;
    if (addr + size > end)
        size = end - addr;
    memcpy(data, (void*)addr, size);
    return size;
}

int mem_list(memory_range *range, int size, int heap) {
    int res = 0;
    int i;
    for (i = 0; i < static_sz && res < size; ++i) {
        range[res++] = staticmem[i];
    }
    for (i = 0; i < stack_sz && res < size; ++i) {
        range[res++] = stackmem[i];
    }
    if (!heap) return res;
    reload_blocks();
    for (i = 0; i < block_sz && res < size; ++i) {
        range[res++] = blockmem[i];
    }
    return res;
}

void mem_lockdata_clear() {
    lock_count = 0;
}

void mem_lockdata_begin() {
    mem_lockdata_clear();
}

void mem_lockdata_add(uint32_t addr, uint8_t size, char *data) {
    if (lock_count >= LOCK_COUNT_MAX) return;
    memlock_data *ld = &lockdata[lock_count++];
    ld->address = addr;
    memcpy(ld->data, data, 8);
    ld->size = size;
}

void mem_lockdata_end() {
}

const memlock_data *mem_lockdata_query(int *count) {
    *count = lock_count;
    return lockdata;
}

void mem_process_lock() {
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
