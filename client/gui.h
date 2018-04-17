#pragma once

#include <cstdint>
#include <string>
#include <vector>

class UdpClient;
class Command;

extern "C" typedef struct GLFWwindow GLFWwindow;

class Gui {
public:
    Gui();
    ~Gui();

    int run();

private:
    void handlePacket(int op, const char *buf, int len);
    void connectPanel();
    void tabPanel();
    void searchPanel();
    void trophyPanel();
    void editPopup();

private:
    UdpClient *client_;
    Command *cmd_;

    GLFWwindow* window_ = NULL;
    int dispWidth_ = 0, dispHeight_ = 0;

    char ip_[256] = "172.27.15.216";
    int tabIndex_ = 0;

    struct SearchResult {
        uint32_t addr;
        std::string hexaddr;
        std::string value;
    };
    struct TrophyInfo {
        int id = -1;
        int grade = 0;
        bool hidden = false;
        bool unlocked = false;
        std::string name;
        std::string desc;
    };
    std::vector<SearchResult> searchResult_;
    int searchStatus_ = 0;
    int searchResultType_ = 0;
    char searchVal_[32] = "";
    int typeComboIndex_ = 0;
    bool hexSearch_ = false;
    bool heapSearch_ = false;
    int searchResultIdx_ = -1;
    char editVal_[32] = "";
    int trophyStatus_ = 0;
    int trophyIdx_ = -1;
    int trophyPlat_ = -1;
    std::vector<TrophyInfo> trophies_;
};
