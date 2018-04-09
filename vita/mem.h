#ifndef __MEM_H_
#define __MEM_H_

#include <stdint.h>

void mem_init();

void mem_search(int type, void *data);
void mem_set(uint32_t addr, const void *data, int size);

#endif
