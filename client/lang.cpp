#include "lang.h"

#include "tinydir.h"
#include "yaml-cpp/yaml.h"
#ifdef _WIN32
#include <windows.h>
#include <shlwapi.h>
#endif
#include <unordered_map>

LangManager g_lang;

bool Lang::load(const std::string &name) {
#include "lang_default.inl"
    std::unordered_map<std::string, std::string> tempTable;
    YAML::Node node;
    try {
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
        name_ = node["name"].as<std::string>();
        code_ = node["code"].as<std::string>();
        for (auto n: node["fonts"]) {
            fonts_.push_back(n.as<std::string>());
        }
        for (auto n: node["strings"]) {
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
            C(MEM_VIEWER)
            C(MEM_TABLE)
            C(TROPHY)
            C(AUTOINT)
            C(AUTOUINT)
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
            C(EDIT_MEM)
            C(VIEW_MEMORY)
            C(ADD_TO_TABLE)
            C(TABLE_ADD)
            C(TABLE_DELETE)
            C(TABLE_EDIT)
            C(TABLE_SAVE)
            C(TABLE_LOAD)
            C(TABLE_ADDR)
            C(TABLE_COMMENT)
            C(TABLE_TYPE)
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
        id_ = name;
    } catch (...) {
        return false;
    }
    return true;
}

bool Lang::loadDefault() {
    return load(std::string());
}

LangManager::LangManager() {
    auto &lang = languages_[""];
    lang.loadDefault();
    currLang_ = &lang;
    loadLanguages();
}

bool LangManager::setLanguage(const std::string& name) {
    auto ite = languages_.find(name);
    if (ite == languages_.end()) return false;
    currLang_ = &ite->second;
    return true;
}

#include <algorithm>
bool LangManager::setLanguageByCode(const std::string &code) {
    for (auto &p: languages_) {
        if (p.second.code() == code) {
            currLang_ = &p.second;
            return true;
        }
    }
    return false;
}

void LangManager::loadLanguages() {
    tinydir_dir dir;
    tinydir_file file;
    auto &deflang = languages_[""];
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
        auto &l = languages_[filename];
        l = deflang;
        if (!l.load(filename)) {
            languages_.erase(filename);
        }
    } while (tinydir_next(&dir) >= 0);
#else
    tinydir_open(&dir, ".");
#endif
    tinydir_close(&dir);
}
