#include <vitasdk.h>
#include <stdint.h>

#include "util.h"

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
