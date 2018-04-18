#include "lang.h"

#include "tinydir.h"
#include "yaml-cpp/yaml.h"
#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#endif
#include <unordered_map>

Lang g_lang;

void Lang::listLanguages(std::vector<std::string> &names) {
    tinydir_dir dir;
    tinydir_file file;
#ifdef _WIN32
    TCHAR path[256];
    GetModuleFileName(NULL, path, 256);
    PathRemoveFileSpec(path);
    tinydir_open(&dir, path);
    do {
        if (tinydir_readfile(&dir, &file) < 0 || lstrcmpi(PathFindExtension(file.name), L".lng") != 0) {
            continue;
        }
        char filename[256];
        WideCharToMultiByte(CP_ACP, 0, file.name, -1, filename, 256, NULL, NULL);
        PathRemoveExtensionA(filename);
        names.push_back(filename);
    } while (tinydir_next(&dir) >= 0);
#else
    tinydir_open(&dir, ".");
#endif
    tinydir_close(&dir);
}

void Lang::loadDefault() {
    load(std::string());
}

void Lang::load(const std::string &name) {
#include "lang_default.inl"
    std::unordered_map<std::string, std::string> tempTable;
    YAML::Node node;
    if (name.empty())
        node = YAML::Load(defaultLang);
    else {
#ifdef _WIN32
        CHAR path[256];
        GetModuleFileNameA(NULL, path, 256);
        PathRemoveFileSpecA(path);
        PathAppendA(path, (name + ".lng").c_str());
        node = YAML::LoadFile(path);
#else
        node = YAML::LoadFile(name + ".lng");
#endif
    }
    for (auto &n: node["fonts"]) {
        fonts_.push_back(n.as<std::string>());
    }
    for (auto &n: node["strings"]) {
        tempTable[n.first.as<std::string>()] = n.second.as<std::string>();
    }
    for (auto &p: tempTable) {
#define C(N) else if(p.first == #N) { langstr_[LANG_##N] = p.second; }
        if (0) {}
        C(WINDOW_TITLE)
        C(TROPHY_GRADE_PLATINUM)
        C(TROPHY_GRADE_GOLD)
        C(TROPHY_GRADE_SILVER)
        C(TROPHY_GRADE_BRONZE)
        C(TROPHY_GRADE_UNKNOWN)
        C(CONNECT)
        C(AUTOCONNECT)
        C(DISCONNECT)
        C(NOT_CONNECTED)
        C(CONNECTING)
        C(IP_ADDR)
        C(MEM_SEARCHER)
        C(TROPHY)
        C(INT32)
        C(UINT32)
        C(INT16)
        C(UINT16)
        C(INT8)
        C(UINT8)
        C(INT64)
        C(UINT64)
        C(FLOAT)
        C(DOUBLE)
        C(NEW_SEARCH)
        C(NEXT_SEARCH)
        C(VALUE)
        C(HEX)
        C(HEAP)
        C(SEARCHING)
        C(SEARCH_RESULT)
        C(TOO_MANY_RESULTS)
        C(SEARCH_IN_PROGRESS)
        C(REFRESH_TROPHY)
        C(REFRESHING)
        C(TROPHY_NAME)
        C(TROPHY_GRADE)
        C(TROPHY_HIDDEN)
        C(TROPHY_UNLOCKED)
        C(TROPHY_TITLE_HIDDEN)
        C(YES)
        C(NO)
        C(TROPHY_UNLOCK)
        C(TROPHY_UNLOCK_ALL)
        C(POPUP_EDIT)
        C(EDIT_ADDR)
        C(OK)
        C(CANCEL)
#undef C
    }
}
