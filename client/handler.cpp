#include "handler.h"

#include "gui.h"

void Handler::process(int op, const char *buf, int len) {
    int cmd = op >> 8;
    int sub = op & 0xFF;
    switch(cmd) {
        case 0:
            gui_.searchResultStart(sub);
            break;
        case 0x8:
            gui_.updateMemory(*(uint32_t*)buf, sub, buf+4);
            break;
        case 0x0C:
            if (sub == 0)
                gui_.setMemViewData(*(uint32_t*)buf, buf + 4, len - 4);
            break;
        case 0x80:
            switch (sub) {
                case 0:
                {
                    if (len < 240) break;
                    gui_.trophyList(*(int*)buf, *(int*)(buf+4), *(int*)(buf+8) != 0, *(int*)(buf+12) != 0, buf + 16, buf + 80);
                    break;
                }
                case 1:
                {
                    gui_.trophyListEnd();
                    break;
                }
                case 2:
                {
                    gui_.trophyListErr();
                    break;
                }
            }
            break;
        case 0x81:
            switch (sub) {
                case 0:
                {
                    gui_.trophyUnlocked(*(int*)buf, *(int*)(buf + 4));
                    break;
                }
                case 0x10:
                {
                    gui_.trophyUnlockErr();
                    break;
                }
            }
            break;
        case 0x100:
            if (!gui_.inSearching()) break;
            gui_.searchResult((const Gui::SearchVal*)buf, len / 12);
            break;
        case 0x200:
            gui_.searchEnd(0);
            break;
        case 0x300:
            switch (sub) {
                case 0:
                    gui_.searchEnd(1);
                    break;
                case 1:
                    gui_.searchEnd(2);
                    break;
            }
            break;
    }
}
