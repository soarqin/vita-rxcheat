#pragma once

#include <cstdint>
#include <vector>
#include <list>
#include <string>
#include <functional>

extern "C" {
    typedef struct IKCPCB ikcpcb;
}

class UdpClient {
public:
    static void init();
    static void finish();
    UdpClient();
    ~UdpClient();
    inline const std::string &titleId() { return titleid_; }
    inline const std::string &title() { return title_; }
    inline void setOnRecv(const std::function<void(int, const char *, int)>& onRecv) { onRecv_ = onRecv; }
    void autoconnect(uint16_t port);
    bool connect(const std::string &addr, uint16_t port);
    void disconnect();
    inline bool isConnecting() {
        return connecting_;
    }
    inline bool isConnected() {
        return kcp_ != NULL;
    }
    void process();
    int send(const char *buf, int len);

    void setOnConnected(const std::function<void()>& f) {
        onConnected_ = f;
    }

private:
    void _init();
    bool _startConnect(void *addr);
    int _send(const char *buf, int len);
    int _recv(char *buf, int len, void *addr);

private:
    int fd_ = -1;
    bool connecting_ = false;
    uint32_t conv_ = 0;
    ikcpcb *kcp_ = NULL;
    std::list<std::string> packets_;
    std::string recvBuf_;
    std::function<void(int, const char *, int)> onRecv_;
    std::string titleid_, title_;

    std::function<void()> onConnected_;
};
