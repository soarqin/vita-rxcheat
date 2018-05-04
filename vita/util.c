#include "util.h"

#include <vitasdk.h>
#include <stdint.h>

static uint64_t start_tick = 0ULL;
static uint64_t resolution_of_tick = 0ULL;
static int crc32_table[0x100];

void util_init() {
    uint32_t i, j;
    uint32_t c;
    for (i = 0; i < 0x100; i++) {
        for (j = 8, c = i << 24; j > 0; --j)
            c = (c & 0x80000000) ? ((c << 1) ^ 0x04c11db7) : (c << 1);
        crc32_table[i] = c;
    }
    resolution_of_tick = sceRtcGetTickResolution() / 1000U;
    SceRtcTick tick;
    sceRtcGetCurrentTick(&tick);
    start_tick = tick.tick;
}

uint32_t util_crc32(const unsigned char *buf, int len, uint32_t init) {
    uint32_t crc = init;
    while (len--) {
        crc = (crc << 8) ^ crc32_table[((crc >> 24) ^ *buf++) & 0xFF];
    }
    return crc;
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
