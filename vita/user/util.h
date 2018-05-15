#ifndef __UTIL_H_
#define __UTIL_H_

#include <stdint.h>
#include <stdlib.h>

void util_init();
void util_finish();
uint64_t util_gettick();
void util_pause_main_thread();
void util_resume_main_thread();
int util_is_allocated(int id);

extern void*(*my_alloc)(size_t);
extern void*(*my_realloc)(void*, size_t);
extern void*(*my_calloc)(size_t, size_t);
extern void (*my_free)(void*);

void util_set_alloc(void*(*a)(size_t), void*(*r)(void*, size_t), void*(*c)(size_t, size_t), void(*f)(void*));

#endif
