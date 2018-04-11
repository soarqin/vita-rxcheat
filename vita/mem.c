#include "mem.h"

#include "debug.h"

#include <vitasdk.h>
#include "kio.h"
#include <stdlib.h>

typedef struct memory_range {
    uint8_t *start;
    size_t size;
} memory_range;

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
} search_type;

static memory_range staticmem[32], stackmem[8]/*, blockmem[128]*/;
static int static_sz = 0, stack_sz = 0/*, block_sz = 0*/;
static int mem_inited = 0;
static int stype = 0, last_sidx = 0;

void mem_init() {
    mem_inited = 1;
    static_sz = stack_sz = 0;
    SceUID modlist[256];
    int num_loaded = 256;
    int ret;
    ret = sceKernelGetModuleList(0xFF, modlist, &num_loaded);
    if (ret == 0) {
        int i;
        for (i = 0; i < num_loaded; ++i) {
            SceKernelModuleInfo info;
            if (sceKernelGetModuleInfo(modlist[i], &info) < 0) continue;
            if (strncmp(info.path, "ux0:", 4) != 0 && strncmp(info.path, "app0:", 5) != 0 && strncmp(info.path, "gro0:", 5) != 0) continue;
            log_debug("Module %s\n", info.path);
            int j;
            for (j = 0; j < 4; ++j) {
                if (info.segments[j].vaddr == 0 || (info.segments[j].perms & 6) != 6) continue;
                log_debug("    0x%08X 0x%08X 0x%08X 0x%08X\n", info.segments[j].vaddr, info.segments[j].memsz, info.segments[j].perms, info.segments[j].flags);
                memory_range *mr = &staticmem[static_sz++];
                mr->start = info.segments[j].vaddr;
                mr->size = info.segments[j].memsz;
            }
        }
    }
    SceKernelThreadInfo status;
    status.size = sizeof(SceKernelThreadInfo);
    SceUID thid = 0x40010001;
    for(; thid <= 0x40010050; ++thid) {
        ret = sceKernelGetThreadInfo(thid, &status);
        if (ret < 0) continue;
        log_debug("0x%08X %s 0x%08X 0x%08X\n", thid, status.name, status.stack, status.stackSize);
        memory_range *mr = &stackmem[stack_sz++];
        mr->start = status.stack;
        mr->size = status.stackSize;
    }
}

/*
static void reload_blocks() {
    block_sz = 0;
    uint32_t addr = 0x80000000;
    while(addr < 0xA0000000) {
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
                    mr->start = heap_info.mappedBase;
                    mr->size = heap_info.mappedSize;
                    continue;
                }
            }
        } 
        addr += 0x8000;
    }
}
*/

static void single_search(SceUID tmpfile, memory_range *mr, const void *data, int size) {
    uint8_t *curr = (uint8_t*)mr->start;
    uint8_t *cend = curr + mr->size - size + 1;
    for (; curr < cend; curr += size) {
        if (memcmp(data, (void*)curr, size) == 0) {
            log_debug("Found at %08X\n", curr);
            kIoWrite(tmpfile, &curr, 4, NULL);
        }
    }
}

static void next_search(SceUID infile, SceUID outfile, const void *data, int size) {
    uint32_t old[0x200];
    while(1) {
        SceSize i, n;
        kIoRead(infile, old, 4 * 0x200, &n);
        n >>= 2;
        for (i = 0; i < n; ++i) {
            if (memcmp((void*)old[i], data, size) == 0) {
                log_debug("Found at %08X\n", old[i]);
                kIoWrite(outfile, &old[i], 4, NULL);
            }
        }
        if (n < 0x200) break;
    }
}

void mem_search(int type, const void *data, int len) {
    int size = 0;
    switch(type) {
        case st_u32: case st_i32: case st_float: size = 4; break;
        case st_u16: case st_i16: size = 2; break;
        case st_u8: case st_i8: size = 1; break;
        case st_u64: case st_i64: case st_double: size = 8; break;
        default: return;
    }
    if (size > len) return;
    if (!mem_inited) {
        mem_init();
    }
    if (stype != type) {
        SceUID f = -1;
        char outfile[256];
        sceClibSnprintf(outfile, 256, "ux0:/data/rcsvr_%d.tmp", last_sidx);
        kIoOpen(outfile, SCE_O_WRONLY | SCE_O_CREAT, &f);
        int i;
        for (i = 0; i < static_sz; ++i) {
            single_search(f, &staticmem[i], data, size);
        }
        for (i = 0; i < stack_sz; ++i) {
            single_search(f, &stackmem[i], data, size);
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
            mem_search(type, data, len);
            return;
        }
        last_sidx ^= 1;
        SceUID of = -1;
        char outfile[256];
        sceClibSnprintf(outfile, 256, "ux0:/data/rcsvr_%d.tmp", last_sidx);
        kIoOpen(outfile, SCE_O_WRONLY | SCE_O_CREAT, &of);
        next_search(f, of, data, size);
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
    memcpy((void *)addr, data, size);
}
