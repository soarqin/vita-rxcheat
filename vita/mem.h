#ifndef __MEM_H_
#define __MEM_H_

#include <stdint.h>

void mem_init();

void mem_search(void *data, int size);
void mem_set(uint32_t addr, const void *data, int size);

#endif
