#pragma once

#include <string>
#include <stdint.h>

struct MemoryRange {
    uint32_t start;
    uint32_t size;
    uint32_t index;
    uint32_t flag;    // 1-read only memory
};

void convertSetSource(const std::string &filename);
void convertStart(const MemoryRange *mr, int count);
