#pragma once

#include <cstdint>
#include <string>
#include <vector>

class UdpClient;
class Command;
class Handler;

extern "C" typedef struct GLFWwindow GLFWwindow;

class Gui {
public:
    Gui();
    ~Gui();

    int run();

#pragma pack(push, 1)
    struct SearchVal {
        uint32_t addr;
        uint64_t val;
    }
#ifdef __GNUC__
    __attribute__((packed))
#endif
    ;
#pragma pack(pop)


    inline bool inSearching() { return searchStatus_ == 1; }
    void searchResultStart(uint8_t type);
    void searchResult(const SearchVal *vals, int count);
    void searchEnd(int ret);

    void trophyList(int id, int grade, bool hidden, bool unlocked, const char *name, const char *desc);
    void trophyListEnd();
    void trophyListErr();
    void trophyUnlocked(int idx, int platidx);
    void trophyUnlockErr();

    void updateMemory(uint32_t addr, uint8_t type, const void *data);
    void setMemViewData(uint32_t addr, const void *data, int len);

private:
    void connectPanel();
    void langPanel();
    void tabPanel();
    void searchPanel();
    void searchPopup();
    void memoryPanel();
    void memoryPopup();
    void enterTableEdit();
    void tablePanel();
    void tablePopup();
    void trophyPanel();

    void saveData();
    void loadData();

    void saveTable(const char *name);
    bool loadTable(const char *name);

    void exportCheatCodes(const char *name);

    void reloadFonts();

private:
    UdpClient *client_;
    Command *cmd_;
    Handler *handler_;

    GLFWwindow* window_ = NULL;
    int dispWidth_ = 0, dispHeight_ = 0;

    bool reloadLang_ = false;

    char ip_[256] =
#ifdef NDEBUG
        ""
#else
        "172.27.15.216"
#endif
        ;
    int tabIndex_ = 0;

    struct MemoryItem {
        uint32_t addr;
        uint8_t type;
        bool locked;
        std::string hexaddr;
        std::string value;
        std::string comment;
        char memval[8];
        void formatValue(bool hex);
    };
    struct TrophyInfo {
        int id = -1;
        int grade = 0;
        bool hidden = false;
        bool unlocked = false;
        std::string name;
        std::string desc;
    };
    std::vector<MemoryItem> searchResults_;
    int searchStatus_ = 0;
    uint8_t searchResultType_ = 0;
    char searchVal_[32] = "";
    int typeComboIndex_ = 0;
    bool searchHex_ = false;
    bool heapSearch_ = false;
    int searchResultIdx_ = -1;
    char searchEditVal_[32] = "";
    bool searchEditHex_ = false;
    char memoryAddr_[9] = "";
    uint32_t memAddr_ = 0U;
    int memViewIndex_ = -1;
    std::vector<uint8_t> memViewData_;
    bool memoryEditing_ = false;
    uint32_t memoryEditingAddr_ = 0;
    char memoryEditVal_[3] = "";
    std::vector<MemoryItem> memTable_;
    int memTableIdx_ = -1;
    bool tableEditAdding_ = false;
    char tableEditAddr_[16] = "";
    char tableEditComment_[64] = "";
    char tableEditVal_[32] = "";
    int tableTypeComboIdx_ = -1;
    bool tableHex_ = false;
    int trophyStatus_ = 0;
    int trophyIdx_ = -1;
    int trophyPlat_ = -1;
    std::vector<TrophyInfo> trophies_;
};
