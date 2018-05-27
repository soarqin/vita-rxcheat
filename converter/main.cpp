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
    if (argc > 2) {
        c.connect(argv[1], 9527);
        convertSetSource(argv[2]);
        c.run();
    } else {
        MemoryRange mr[1];
        mr[0].start = 0x81000000U;
        mr[0].size = 0xA0000000U - 0x81000000U;
        mr[0].flag = 1;
        mr[0].index = 0;
        convertSetSource(argv[1]);
        convertStart(mr, 1);
    }
    return 0;
}
