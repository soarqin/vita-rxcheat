#define _CRT_SECURE_NO_WARNINGS

#include "gui.h"

#include <cstdio>
#include <Windows.h>

#ifdef _WIN32
int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
#ifndef NDEBUG
    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
#endif
#else
int main(int argc, char *argv[]) {
#endif
    Gui gui;
    return gui.run();
}
