#include "command.h"

#include "net.h"

#include <string>
#include <cstdlib>

void Command::startSearch(int st, void *data) {
    int len = 0;
    searchType_ = st & 0xFF;
    switch (searchType_) {
        case st_u32:
        case st_i32:
        case st_float:
            len = 4;
            break;
        case st_u16:
        case st_i16:
            len = 2;
            break;
        case st_u8:
        case st_i8:
            len = 1;
            break;
        case st_u64:
        case st_i64:
        case st_double:
            len = 8;
            break;
    }
    sendCommand(st, data, len);
}

void Command::nextSearch(void *data) {
    startSearch(searchType_ | 0x100, data);
}

void Command::startFuzzySearch(int st) {
    searchType_ = st & 0xFF;
    sendCommand(0x1000 | st, NULL, 0);
}

void Command::nextFuzzySearch(int direction) {
    startFuzzySearch(((direction + 3) << 8) | searchType_);
}

void Command::sendCommand(int cmd, void *buf, int len) {
    std::string n;
    n.resize(len + 4);
    *(int*)&n[0] = cmd;
    if (len > 0) memcpy(&n[4], buf, len);
    client_.send(n.c_str(), len + 4);
}
