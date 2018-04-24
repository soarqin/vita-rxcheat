#pragma once

#include <vector>
#include <string>
#include <map>

class Lang {
public:
    enum:int {
        LANG_WINDOW_TITLE,
        LANG_TROPHY_GRADE_PLATINUM,
        LANG_TROPHY_GRADE_GOLD,
        LANG_TROPHY_GRADE_SILVER,
        LANG_TROPHY_GRADE_BRONZE,
        LANG_TROPHY_GRADE_UNKNOWN,
        LANG_CONNECT,
        LANG_AUTOCONNECT,
        LANG_DISCONNECT,
        LANG_NOT_CONNECTED,
        LANG_CONNECTING,
        LANG_IP_ADDR,
        LANG_MEM_SEARCHER,
        LANG_MEM_VIEWER,
        LANG_MEM_TABLE,
        LANG_TROPHY,
        LANG_AUTOINT,
        LANG_DATATYPE_FIRST = LANG_AUTOINT,
        LANG_AUTOUINT,
        LANG_INT32,
        LANG_UINT32,
        LANG_INT16,
        LANG_UINT16,
        LANG_INT8,
        LANG_UINT8,
        LANG_INT64,
        LANG_UINT64,
        LANG_FLOAT,
        LANG_DOUBLE,
        LANG_NEW_SEARCH,
        LANG_NEXT_SEARCH,
        LANG_VALUE,
        LANG_HEX,
        LANG_HEAP,
        LANG_SEARCHING,
        LANG_SEARCH_RESULT,
        LANG_TOO_MANY_RESULTS,
        LANG_SEARCH_IN_PROGRESS,
        LANG_EDIT_MEM,
        LANG_VIEW_MEMORY,
        LANG_REFRESH_TROPHY,
        LANG_REFRESHING,
        LANG_TROPHY_NAME,
        LANG_TROPHY_GRADE,
        LANG_TROPHY_HIDDEN,
        LANG_TROPHY_UNLOCKED,
        LANG_TROPHY_TITLE_HIDDEN,
        LANG_YES,
        LANG_NO,
        LANG_TROPHY_UNLOCK,
        LANG_TROPHY_UNLOCK_ALL,
        LANG_POPUP_EDIT,
        LANG_EDIT_ADDR,
        LANG_OK,
        LANG_CANCEL,
        LANG_MAX
    };
    bool loadDefault();
    bool load(const std::string &name);
    inline const std::vector<std::string> &fonts() const { return fonts_; }
    inline const char *getString(int index) const {
        return langstr_[index].c_str();
    }
    inline const std::string &id() const { return id_; }
    inline const std::string &name() const { return name_; }
    inline const std::string &code() const { return code_; }

private:
    std::string id_;
    std::string name_;
    std::string code_;
    std::vector<std::string> fonts_;
    std::string langstr_[LANG_MAX];
};

class LangManager {
public:
    LangManager();
    bool setLanguage(const std::string& name);
    bool setLanguageByCode(const std::string& code);

    inline Lang& currLang() { return *currLang_; }
    const std::map<std::string, Lang> &langs() { return languages_; }

private:
    void loadLanguages();

private:
    std::map<std::string, Lang> languages_;
    Lang *currLang_;
};

extern LangManager g_lang;

#define LS(N) (g_lang.currLang().getString(Lang::LANG_##N))
