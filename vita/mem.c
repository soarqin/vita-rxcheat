#include "mem.h"

#include "debug.h"

#include <vitasdk.h>
#include <stdlib.h>
#include <kuio.h>

typedef struct memory_range {
    uint8_t *start;
    size_t size;
} memory_range;

static memory_range staticmem[32], stackmem[8], blockmem[128];
static int static_sz = 0, stack_sz = 0, block_sz = 0;
static int mem_inited = 0;

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

static void single_search(memory_range *mr, void *data, int size) {
    uint8_t *curr = (uint8_t*)mr->start;
    uint8_t *cend = curr + mr->size - size + 1;
    for (; curr < cend; ++curr) {
        if (memcmp(data, (void*)curr, size) == 0) {
            log_debug("Found at %08X\n", curr);
        }
    }
}

void mem_search(void *data, int size) {
    if (!mem_inited) {
        mem_init();
    }
    int i;
    for (i = 0; i < static_sz; ++i) {
        single_search(&staticmem[i], data, size);
    }
    for (i = 0; i < stack_sz; ++i) {
        single_search(&stackmem[i], data, size);
    }
    /*
    reload_blocks();
    */
}

void mem_set(uint32_t addr, const void *data, int size) {
    memcpy((void *)addr, data, size);
}