#ifndef __MEM_H_
#define __MEM_H_

#include <stdint.h>

void mem_init();

void mem_search(int type, const void *data, int len, void (*cb)(const uint32_t *addr, int count, int datalen), int hep);
void mem_search_reset();
void mem_set(uint32_t addr, const void *data, int size);

#endif
