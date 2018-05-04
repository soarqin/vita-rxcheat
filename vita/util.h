#ifndef __UTIL_H_
#define __UTIL_H_

#include <stdint.h>
#include <stdlib.h>

void util_init();
uint32_t util_crc32(const unsigned char *buf, int len, uint32_t init);
uint64_t util_gettick();

extern void*(*my_alloc)(size_t);
extern void*(*my_realloc)(void*, size_t);
extern void*(*my_calloc)(size_t, size_t);
extern void (*my_free)(void*);

void util_set_alloc(void*(*a)(size_t), void*(*r)(void*, size_t), void*(*c)(size_t, size_t), void(*f)(void*));

#endif
