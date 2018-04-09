#pragma once

#include <cstdint>
#include <vector>
#include <list>
#include <string>

extern "C" {
    typedef struct IKCPCB ikcpcb;
}

class UdpClient {
public:
    static void init();
    static void finish();
    UdpClient();
    ~UdpClient();
    bool connect(const std::string &addr, uint16_t port);
    void process();
    int send(const char *buf, int len);
    int recv(char *buf, int len);

private:
    int _send(const char *buf, int len);
    int _recv(char *buf, int len);
    int _sendAndRecv(const char *buf, int len, char *res, int reslen);

private:
    int fd_ = -1;
    uint32_t conv_ = 0;
    ikcpcb *kcp_ = NULL;
    std::list<std::string> packets_;
};
