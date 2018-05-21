#include "convert.h"

static std::string srcFilename;

void convertSetSource(const std::string &filename) {
    srcFilename = filename;
}

void convertStart(const MemoryRange *mr, int count) {
    for (int i = 0; i < count; ++i) {
        printf("Memory block %3d: %X %X %d\n", mr[i].index, mr[i].start, mr[i].size, mr[i].flag);
    }
}
