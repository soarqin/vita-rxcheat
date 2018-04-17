#define _CRT_SECURE_NO_WARNINGS

#include "gui.h"

#include <cstdio>
#include <Windows.h>

int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#ifndef NDEBUG
    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
#endif
    Gui gui;
    return gui.run();

    return 0;
}
