#include "net.h"

#include "convert.h"

#include <stdio.h>

class MyClient: public Client {
protected:
    virtual void onConnected() {
        printf("Connected to game.\n");
        sendData(0xA00, nullptr, 0);
    }

    virtual void onKcpData(int op, const void *buf, int len) {
        switch (op) {
            case 0xA00:
                printf("Got memory map from game, parsing...\n");
                stop();
                convertStart((const MemoryRange*)buf, len / sizeof(MemoryRange));
                break;
            case 0xA01:
                printf("Failed to get memory map from game, exiting...\n");
                stop();
                break;
        }
    }
};

int main(int argc, char *argv[]) {
    MyClient c;
    c.connect("192.168.137.80", 9527);
    convertSetSource("test.psv");
    c.run();
    return 0;
}
