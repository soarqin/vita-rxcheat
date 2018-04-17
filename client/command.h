#pragma once

#include <cstdint>
#include <string>
#include <functional>

class UdpClient;

class Command {
public:
    enum :int {
        st_none = 0,
        st_u32 = 1,
        st_u16 = 2,
        st_u8 = 3,
        st_u64 = 4,
        st_i32 = 5,
        st_i16 = 6,
        st_i8 = 7,
        st_i64 = 8,
        st_float = 9,
        st_double = 10,
    };

public:
    inline Command(UdpClient &cl): client_(cl) {}
    void startSearch(int st, bool heap, void *data);
    void nextSearch(void *data);
    void startFuzzySearch(int st);
    void nextFuzzySearch(int direction);
    int getTypeSize(int type);
    void formatTypeData(char *output, int type, const void *data);

    void refreshTrophy();
    void unlockTrophy(int id, bool hidden);
    void unlockAllTrophy(uint32_t hidden[4]);

private:
    void sendCommand(int cmd, void *buf, int len);

private:
    UdpClient &client_;
    int searchType_ = st_none;
    std::function<void(int id, int grade, bool hidden, const std::string &name, const std::string &desc)> trophyCb_;
};
