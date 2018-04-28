#include "util.h"

#include <vitasdk.h>
#include <stdint.h>

static uint64_t start_tick = 0ULL;
static uint64_t resolution_of_tick = 0ULL;

void util_init() {
    resolution_of_tick = sceRtcGetTickResolution() / 1000U;
    SceRtcTick tick;
    sceRtcGetCurrentTick(&tick);
    start_tick = tick.tick;
}

uint64_t util_gettick() {
    SceRtcTick tick;
    sceRtcGetCurrentTick(&tick);
    return (tick.tick - start_tick) / resolution_of_tick;
}

void*(*my_alloc)(size_t);
void*(*my_realloc)(void*, size_t);
void*(*my_calloc)(size_t, size_t);
void (*my_free)(void*);

void util_set_alloc(void*(*a)(size_t), void*(*r)(void*, size_t), void*(*c)(size_t, size_t), void(*f)(void*)) {
    my_alloc = a; my_realloc = r; my_calloc = c; my_free = f;
}
