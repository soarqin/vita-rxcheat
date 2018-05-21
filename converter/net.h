#pragma once

#include <stdint.h>
#include <string>

struct ClientContext;

class Client {
public:
    Client();
    virtual ~Client();

    void connect(const char *addr, uint16_t port);
    void sendData(int op, const void *data, int len);

    void run();
    void stop();

protected:
    virtual void onConnected() {}
    virtual void onKcpData(int op, const void *buf, int len) {}
    virtual void onError() {}

private:
    void doSend(const void *data, int len, const void *data2 = NULL, int len2 = 0);
    void onUdpRecv(const void *buf, int len);

private:
    ClientContext *context_;
};
