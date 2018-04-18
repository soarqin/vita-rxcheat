#pragma once

#include <vector>
#include <string.h>

class Lang {
public:
    enum:int {
        LANG_OK,
        LANG_CANCEL,
        LANG_MAX
    };
    void load();
    inline const std::vector<std::string> &fonts() { return fonts_; }
    inline const char *getString(int index) {
        return langstr_[index].c_str();
    }

private:
    std::vector<std::string> fonts_;
    std::string langstr_[LANG_MAX];
};

extern Lang g_lang;

#define LS(N) g_lang.getString(Lang::LANG_##N)
