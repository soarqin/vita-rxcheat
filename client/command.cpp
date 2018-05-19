#define _CRT_SECURE_NO_WARNINGS

#include "command.h"

#include "net.h"

#include <string>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <cinttypes>

void Command::startSearch(uint32_t st, bool heap, void *data) {
    searchType_ = st & 0xFF;
    int size = getTypeSize(searchType_, data);
    if (size == 0) return;
    sendCommand(st | (heap ? 0x100 : 0), data, size);
}

void Command::nextSearch(void *data) {
    startSearch(searchType_ | 0x200, false, data);
}

void Command::startFuzzySearch(uint32_t st, bool heap) {
    searchType_ = st & 0xFF;
    sendCommand(0x1000 | (heap ? 0x100 : 0) | st, NULL, 0);
}

void Command::nextFuzzySearch(bool direction) {
    sendCommand(0x1200 | (direction ? 0x100 : 0) | searchType_, NULL, 0);
}

void Command::modifyMemory(uint8_t st, uint32_t offset, const void *data) {
    int size = getTypeSize(st, data);
    char cont[12];
    *(uint32_t*)cont = offset;
    memcpy(cont + 4, data, size);
    sendCommand(0x800 | st, cont, 4 + size);
}

int Command::getTypeSize(uint8_t type, const void* data) {
    switch (type) {
        case st_autoint:
        {
            int64_t val = *(int64_t*)data;
            if (val >= 0x80000000LL || val < -0x80000000LL) return 8;
            if (val >= 0x8000LL || val < -0x8000LL) return 4;
            if (val >= 0x80LL || val < -0x80LL) return 2;
            return 1;
        }
        case st_autouint:
        {
            int64_t val = *(int64_t*)data;
            if (val >= 0x100000000ULL) return 8;
            if (val >= 0x10000ULL) return 4;
            if (val >= 0x100ULL) return 2;
            return 1;
        }
        case st_u32:
        case st_i32:
        case st_float:
            return 4;
        case st_u16:
        case st_i16:
            return 2;
        case st_u8:
        case st_i8:
            return 1;
        case st_u64:
        case st_i64:
        case st_double:
            return 8;
    }
    return 0;
}

void Command::formatTypeData(char *output, uint8_t type, const void *data, bool hex) {
    switch (type) {
        case st_i8:
            sprintf(output, hex ? "%X": "%d", *(int8_t*)data);
            break;
        case st_i16:
            sprintf(output, hex ? "%X": "%d", *(int16_t*)data);
            break;
        case st_i32:
            sprintf(output, hex ? "%X": "%d", *(int32_t*)data);
            break;
        case st_autoint:
        case st_i64:
            sprintf(output, hex ? "%" PRIX64 : "%" PRId64, *(int64_t*)data);
            break;
        case st_u8:
            sprintf(output, hex ? "%X": "%u", *(uint8_t*)data);
            break;
        case st_u16:
            sprintf(output, hex ? "%X": "%u", *(uint16_t*)data);
            break;
        case st_u32:
            sprintf(output, hex ? "%X": "%u", *(uint32_t*)data);
            break;
        case st_autouint:
        case st_u64:
            sprintf(output, hex ? "%" PRIX64 : "%" PRIu64, *(uint64_t*)data);
            break;
        case st_float:
            sprintf(output, "%.10g", *(float*)data);
            break;
        case st_double:
            sprintf(output, "%.10g", *(double*)data);
            break;
        default:
            output[0] = 0;
    }
}

int Command::getRawData(void *dst, uint8_t type, const char *src, bool isHex) {
    switch (type) {
        case Command::st_autouint: case Command::st_u64:
        {
            uint64_t val = strtoull(src, NULL, isHex ? 16 : 10);
            memcpy(dst, &val, 8);
            return 8;
        }
        case Command::st_autoint: case Command::st_i64:
        {
            int64_t val = strtoll(src, NULL, isHex ? 16 : 10);
            memcpy(dst, &val, 8);
            return 8;
        }
        case Command::st_u32:
        {
            uint32_t val = (uint32_t)strtoul(src, NULL, isHex ? 16 : 10);
            memset(dst, 0, 8);
            memcpy(dst, &val, 4);
            return 4;
        }
        case Command::st_u16:
        {
            uint16_t val = (uint16_t)strtoul(src, NULL, isHex ? 16 : 10);
            memset(dst, 0, 8);
            memcpy(dst, &val, 2);
            return 2;
        }
        case Command::st_u8:
        {
            uint8_t val = (uint8_t)strtoul(src, NULL, isHex ? 16 : 10);
            memset(dst, 0, 8);
            memcpy(dst, &val, 1);
            return 1;
        }
        case Command::st_i32:
        {
            int32_t val = (int32_t)strtol(src, NULL, isHex ? 16 : 10);
            memset(dst, 0, 8);
            memcpy(dst, &val, 4);
            return 4;
        }
        case Command::st_i16:
        {
            int16_t val = (int16_t)strtol(src, NULL, isHex ? 16 : 10);
            memset(dst, 0, 8);
            memcpy(dst, &val, 2);
            return 2;
        }
        case Command::st_i8:
        {
            int8_t val = (int8_t)strtol(src, NULL, isHex ? 16 : 10);
            memset(dst, 0, 8);
            memcpy(dst, &val, 1);
            return 1;
        }
        case Command::st_float:
        {
            float val = strtof(src, NULL);
            memset(dst, 0, 8);
            memcpy(dst, &val, 4);
            return 4;
        }
        case Command::st_double:
        {
            double val = strtod(src, NULL);
            memcpy(dst, &val, 8);
            return 8;
        }
    }
    return 0;
}

void Command::readMem(uint32_t addr) {
    sendCommand(0x0C00, &addr, 4);
}

void Command::lockBegin() {
    sendCommand(0x2000, NULL, 0);
}

void Command::lock(uint32_t addr, uint8_t type, const char *data) {
    char buf[12];
    *(uint32_t*)buf = addr;
    memcpy(buf + 4, data, 8);
    sendCommand(0x2100 | type, buf, 12);
}

void Command::lockEnd() {
    sendCommand(0x2200, NULL, 0);
}

void Command::refreshTrophy() {
    sendCommand(0x8000, NULL, 0);
}

void Command::unlockTrophy(int id, bool hidden) {
    if (hidden) id |= 0x100;
    sendCommand(0x8100, &id, 4);
}

void Command::unlockAllTrophy(uint32_t hidden[4]) {
    sendCommand(0x8101, hidden, 16);
}

void Command::sendCommand(int cmd, const void *buf, int len) {
    if (len < 0) return;
    std::string n;
    n.resize(len + 8);
    *(int*)&n[0] = cmd;
    *(uint32_t*)&n[4] = (uint32_t)len;
    if (len > 0) memcpy(&n[8], buf, len);
    client_.send(n.c_str(), len + 8);
}
