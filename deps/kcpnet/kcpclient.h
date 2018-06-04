#pragma once

#include <stdint.h>
#include <string>

struct ClientContext;

class KcpClient {
public:
	KcpClient();
	virtual ~KcpClient();

	void sendData(int op, const void *data, int len);

	void start(const char *addr, uint16_t port);
	void stop();

    void loop();
    void runOnce();

protected:
	virtual void onConnected() {}
	virtual void onKcpData(int op, const void *buf, int len) {}
	virtual void onError() {}

private:
	void doSend(const void *data, int len, const void *data2 = NULL, int len2 = 0);
	void onUdpRecv(const void *buf, int len);
    void checkKcpUpdate();

private:
	ClientContext *context_;
};
