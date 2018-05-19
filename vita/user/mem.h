#ifndef __MEM_H_
#define __MEM_H_

#include <stdint.h>

typedef void (*mem_search_cb)(const uint32_t *addr, int count, int datalen);
typedef void (*mem_search_start_cb)(int type);
typedef void (*mem_search_end_cb)(int err);

void mem_init();
void mem_finish();

void mem_force_reload();
void mem_check_reload();
uint32_t mem_convert(uint32_t addr, int *read_only);
void mem_reload();

void mem_search(int type, int heap, const void *data, int len, mem_search_cb cb);
void mem_search_reset();
void mem_set(uint32_t addr, const void *data, int size);
void mem_start_search(int type, int heap, const char *buf, int len, mem_search_cb cb, mem_search_start_cb cb_start, mem_search_end_cb cb_end);
void mem_start_fuzzy_search(int type, int heap, mem_search_start_cb cb_start, mem_search_end_cb cb_end);
void mem_next_fuzzy_search(int direction, mem_search_cb cb, mem_search_start_cb cb_start, mem_search_end_cb cb_end);
int mem_read(uint32_t addr, void *data, int size);

typedef struct memory_range {
    uint32_t start;
    uint32_t size;
    uint32_t index;
    uint32_t flag;    // 1-read only memory
} memory_range;
typedef struct memlock_data {
    uint32_t address;
    char data[8];
    uint8_t size;
} memlock_data;

int mem_list(memory_range *range, int size, int heap);
void mem_dump();
int mem_is_dumping();

void mem_lockdata_clear();
void mem_lockdata_begin();
void mem_lockdata_add(uint32_t addr, uint8_t size, const char *data);
void mem_lockdata_end();
const memlock_data *mem_lockdata_query(int *count);
void mem_lockdata_process();

int mem_get_type_size(int type, const void *data);

#endif
