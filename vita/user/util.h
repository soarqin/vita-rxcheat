#ifndef __UTIL_H_
#define __UTIL_H_

#include <stdint.h>
#include <stdlib.h>

enum {
    SCE_KERNEL_CPU_MASK_SHIFT = 16,
    SCE_KERNEL_CPU_MASK_USER_0 = (0x01 << SCE_KERNEL_CPU_MASK_SHIFT),
    SCE_KERNEL_CPU_MASK_USER_1 = (0x01 << (SCE_KERNEL_CPU_MASK_SHIFT + 1)),
    SCE_KERNEL_CPU_MASK_USER_2 = (0x01 << (SCE_KERNEL_CPU_MASK_SHIFT + 2)),
    SCE_KERNEL_CPU_MASK_USER_ALL = SCE_KERNEL_CPU_MASK_USER_0 | SCE_KERNEL_CPU_MASK_USER_1 | SCE_KERNEL_CPU_MASK_USER_2,
};

#define CHAR_CROSS    "\xE2\x95\xB3"
#define CHAR_SQUARE   "\xE2\x96\xA1"
#define CHAR_TRIANGLE "\xE2\x96\xB3"
#define CHAR_CIRCLE   "\xE2\x97\x8B"

#define CHAR_LEFT  "\xE2\x86\x90"
#define CHAR_RIGHT "\xE2\x86\x92"
#define CHAR_UP    "\xE2\x86\x91"
#define CHAR_DOWN  "\xE2\x86\x93"

void util_init();
void util_finish();
void util_lock_power();
void util_unlock_power();
uint64_t util_gettick();
void util_pause_main_thread();
void util_resume_main_thread();
int util_is_allocated(int id);
int util_readfile_by_line(const char *filename, int (*cb)(const char *name, void *arg), void *arg);

extern void*(*my_alloc)(size_t);
extern void*(*my_realloc)(void*, size_t);
extern void*(*my_calloc)(size_t, size_t);
extern void (*my_free)(void*);

void util_set_alloc(void*(*a)(size_t), void*(*r)(void*, size_t), void*(*c)(size_t, size_t), void(*f)(void*));

#endif
