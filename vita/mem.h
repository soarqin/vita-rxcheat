#ifndef __MEM_H_
#define __MEM_H_

#include <stdint.h>

void mem_init();
void mem_finish();

void mem_search(int type, int heap, const void *data, int len, void (*cb)(const uint32_t *addr, int count, int datalen));
void mem_search_reset();
void mem_set(uint32_t addr, const void *data, int size);
void mem_start_search(int type, int heap, const char *buf, int len, void (*cb)(const uint32_t *addr, int count, int datalen), void (*cb_start)(int type), void (*cb_end)(int err));

int mem_get_type_size(int type);

#endif
