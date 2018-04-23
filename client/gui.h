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
    void searchResultStart(int type);
    void searchResult(const SearchVal *vals, int count);
    void searchEnd(int ret);

    void trophyList(int id, int grade, bool hidden, bool unlocked, const char *name, const char *desc);
    void trophyListEnd();
    void trophyListErr();
    void trophyUnlocked(int idx, int platidx);
    void trophyUnlockErr();

    void updateMemory(uint32_t addr, int type, const void *data);
    void setMemViewData(uint32_t addr, const void *data, int len);

private:
    void connectPanel();
    void tabPanel();
    void searchPanel();
    void searchPopup();
    void memoryPanel();
    void memoryPopup();
    void tablePanel();
    void tablePopup();
    void trophyPanel();

    void saveData();
    void loadData();

private:
    UdpClient *client_;
    Command *cmd_;
    Handler *handler_;

    GLFWwindow* window_ = NULL;
    int dispWidth_ = 0, dispHeight_ = 0;

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
        int type;
        std::string hexaddr;
        std::string value;
        std::string name;
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
    std::vector<MemoryItem> memTable_;
    int searchStatus_ = 0;
    int searchResultType_ = 0;
    char searchVal_[32] = "";
    int typeComboIndex_ = 0;
    bool hexSearch_ = false;
    bool heapSearch_ = false;
    int searchResultIdx_ = -1;
    bool searchEditing_ = false;
    char searchEditVal_[32] = "";
    char memoryAddr_[9] = "";
    uint32_t memAddr_ = 0U;
    int memViewIndex_ = -1;
    std::vector<uint8_t> memViewData_;
    bool memoryEditing_ = false;
    uint32_t memoryEditingAddr_ = 0;
    char memoryEditVal_[32] = "";
    bool tableEditing_ = false;
    char tableEditVal_[32] = "";
    int trophyStatus_ = 0;
    int trophyIdx_ = -1;
    int trophyPlat_ = -1;
    std::vector<TrophyInfo> trophies_;
};
