#pragma once

class Gui;

class Handler {
public:
    inline Handler(Gui &g): gui_(g) {}
    void process(int op, const char *buf, int len);

private:
    Gui &gui_;
};
