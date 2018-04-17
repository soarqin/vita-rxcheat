#include "gui.h"

#include "net.h"
#include "command.h"

#include "imgui.h"
#include "imgui_impl_glfw_gl3.h"

#include <stdio.h>
#include <windows.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

enum:int {
    WIN_WIDTH = 640,
    WIN_HEIGHT = 640,
};

static void glfwErrorCallback(int error, const char* description) {
    fprintf(stderr, "Error %d: %s\n", error, description);
}

inline const char *getGradeName(int grade) {
    switch (grade) {
        case 1: return "Platinum"; break;
        case 2: return "Gold"; break;
        case 3: return "Silver"; break;
        case 4: return "Bronze"; break;
        default: return "Unknown"; break;
    }
}

Gui::Gui() {
    UdpClient::init();
    client_ = new UdpClient;
    cmd_ = new Command(*client_);

    client_->setOnRecv(std::bind(&Gui::handlePacket, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit())
        throw std::exception("Unable to initialize GLFW");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, 0);
    window_ = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "VITA Remote Cheat Client", NULL, NULL);
    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);
    gl3wInit2(glfwGetProcAddress);

    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplGlfwGL3_Init(window_, true);

    ImGui::StyleColorsDark();
    auto &style = ImGui::GetStyle();
    style.WindowRounding = 0.f;
    style.ItemSpacing = ImVec2(5.f, 5.f);

    ImFontAtlas *f = io.Fonts;
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x31FF, // Katakana Phonetic Extensions
        0x4e00, 0x9FAF, // CJK Ideograms
        0xFF00, 0xFFEF, // Half-width characters
        0,
    };
    if (!f->AddFontFromFileTTF("font.ttc", 18.0f, NULL, ranges)
#ifdef _WIN32
        && !f->AddFontFromFileTTF("C:/Windows/Fonts/msyh.ttc", 18.0f, NULL, ranges)
        && !f->AddFontFromFileTTF("C:/Windows/Fonts/simsun.ttc", 18.0f, NULL, ranges)
#endif
        )
        f->AddFontDefault();
}

Gui::~Gui() {
    delete cmd_;
    delete client_;

    ImGui_ImplGlfwGL3_Shutdown();
    glfwDestroyWindow(window_);
    ImGui::DestroyContext();
    glfwTerminate();

    UdpClient::finish();
}

int Gui::run() {
    while (!glfwWindowShouldClose(window_)) {
        client_->process();
        glfwPollEvents();

        glfwGetFramebufferSize(window_, &dispWidth_, &dispHeight_);

        ImGui_ImplGlfwGL3_NewFrame();
        char title[256];
        snprintf(title, 256, "%.1f FPS", ImGui::GetIO().Framerate);
        glfwSetWindowTitle(window_, title);

        if (ImGui::Begin("", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
            connectPanel();
            if (client_->isConnected()) {
                tabPanel();
                switch (tabIndex_) {
                    case 0:
                        searchPanel();
                        break;
                    case 1:
                        trophyPanel();
                        break;
                }
            }
            ImGui::End();
        }

        glViewport(0, 0, dispWidth_, dispHeight_);
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window_);
    }

    return 0;
}

void Gui::handlePacket(int op, const char *buf, int len) {
    if (op < 0x100) {
        searchResult_.clear();
        searchStatus_ = 1;
        searchResultType_ = op;
    } else {
        switch (op) {
            case 0x8000:
            {
                if (len < 240) break;
                int id = *(int*)buf;
                if (id >= (int)trophies_.size()) trophies_.resize(id + 1);
                TrophyInfo &ti = trophies_[id];
                ti.id = id;
                ti.grade = *(int*)(buf+4);
                ti.hidden = *(int*)(buf+8) != 0;
                ti.unlocked = *(int*)(buf+12) != 0;
                ti.name = buf + 16;
                ti.desc = buf + 80;
                if (ti.grade == 1) trophyPlat_ = id;
                break;
            }
            case 0x8001:
            {
                trophyStatus_ = 0;
                break;
            }
            case 0x8002:
            {
                trophyStatus_ = 2;
                break;
            }
            case 0x8100:
            {
                int idx = *(int*)buf;
                int platidx = *(int*)(buf + 4);
                if (idx >= 0 && idx < (int)trophies_.size()) {
                    trophies_[idx].unlocked = true;
                }
                if (platidx >= 0) {
                    trophies_[platidx].unlocked = true;
                }
                break;
            }
            case 0x8110:
            {
                break;
            }
            case 0x10000:
            {
                if (searchStatus_ != 1) break;
                searchResultIdx_ = -1;
                len /= 12;
                const uint32_t *res = (const uint32_t*)buf;
                for (int i = 0; i < len; ++i) {
                    char hex[16], value[64];
                    uint32_t addr = res[i * 3];
                    snprintf(hex, 16, "%08X", addr);
                    cmd_->formatTypeData(value, searchResultType_, &res[i * 3 + 1]);
                    SearchResult sr = {addr, hex, value};
                    searchResult_.push_back(sr);
                }
                break;
            }
            case 0x20000:
                searchStatus_ = 2;
                break;
            case 0x30000:
                searchResult_.clear();
                searchStatus_ = 3;
                break;
            case 0x30001:
                searchResult_.clear();
                searchStatus_ = 4;
                break;
        }
    }
}

inline void Gui::connectPanel() {
    ImGui::SetWindowPos(ImVec2(10.f, 10.f));
    ImGui::SetWindowSize(ImVec2(dispWidth_ - 20.f, dispHeight_ - 20.f));
    if (client_->isConnected()) {
        ImGui::Text(client_->titleId().c_str()); ImGui::SameLine(); ImGui::Text(client_->title().c_str());
        if (ImGui::Button("Disconnect", ImVec2(100.f, 0.f))) {
            client_->disconnect();
        }
    } else {
        ImGui::Text("Not connected");
        if (ImGui::Button("Connect", ImVec2(100.f, 0.f))) {
            client_->connect(ip_, 9527);
        }
    }
    ImGui::SameLine(125.f);
    ImGui::Text("IP Address:"); ImGui::SameLine();
    ImGui::PushItemWidth(200.f);
    ImGui::InputText("##IP", ip_, 256, client_->isConnected() ? ImGuiInputTextFlags_ReadOnly : 0);
    ImGui::PopItemWidth();
}

inline void Gui::tabPanel() {
    ImGui::RadioButton("Memory Searcher", &tabIndex_, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Trophy", &tabIndex_, 1);
}

inline void Gui::searchPanel() {
    const char* comboItems[] = {
        "int32", "uint32", "int16", "uint16",
        "int8", "uint8", "int64", "uint64",
        "float", "double",
    };

    const int comboItemType[] = {Command::st_i32, Command::st_u32,
        Command::st_i16, Command::st_u16, Command::st_i8, Command::st_u8,
        Command::st_i64, Command::st_u64, Command::st_float, Command::st_double,
    };

    if (searchStatus_ == 1) {
        ImGui::Button("Searching...", ImVec2(100.f, 0.f));
    } else {
        if (ImGui::Button("Search!", ImVec2(100.f, 0.f))) {
            uint64_t number = strtoull(searchVal_, NULL, hexSearch_ ? 16 : 10);
            cmd_->startSearch(Command::st_i32, heapSearch_, &number);
        }
    }
    ImGui::SameLine(125.f);
    ImGui::Text("Value:"); ImGui::SameLine();
    ImGui::PushItemWidth(200.f);
    ImGui::InputText("##Value", searchVal_, 31, hexSearch_ ? ImGuiInputTextFlags_CharsHexadecimal : ImGuiInputTextFlags_CharsDecimal);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::PushItemWidth(100.f);
    if (ImGui::BeginCombo("##Type", comboItems[comboIndex_], 0)) {
        for (int i = 0; i < 10; ++i) {
            bool selected = comboIndex_ == i;
            if (ImGui::Selectable(comboItems[i], selected)) {
                comboIndex_ = i;
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Checkbox("HEX", &hexSearch_)) {
        searchVal_[0] = 0;
    }
    ImGui::SameLine();
    ImGui::Checkbox("HEAP", &heapSearch_);

    switch (searchStatus_) {
        case 2:
        {
            ImGui::Text("Search result");
            ImGui::ListBoxHeader("##Result");
            ImGui::Columns(2, NULL, true);
            int sz = searchResult_.size();
            for (int i = 0; i < sz; ++i) {
                bool selected = searchResultIdx_ == i;
                if (ImGui::Selectable(searchResult_[i].hexaddr.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns))
                    searchResultIdx_ = i;
                if (selected) ImGui::SetItemDefaultFocus();
                ImGui::NextColumn();
                ImGui::Text(searchResult_[i].value.c_str());
                ImGui::NextColumn();
            }
            ImGui::ListBoxFooter();
            break;
        }
        case 3:
            ImGui::Text("Too many results, do more search please");
            break;
        case 4:
            ImGui::Text("Searching in progress, please do it later");
            break;
    }
}

inline void Gui::trophyPanel() {
    switch (trophyStatus_) {
        case 0:
        case 2:
            if (ImGui::Button("Refresh trophies")) {
                trophyStatus_ = 1;
                trophyIdx_ = -1;
                trophyPlat_ = -1;
                trophies_.clear();
                cmd_->refreshTrophy();
            }
            break;
        case 1:
            ImGui::Text("Refreshing");
            break;
    }
    int sz = trophies_.size();
    if (sz == 0) return;
    ImGui::ListBoxHeader("##Trophies", ImVec2(dispWidth_ - 30.f, 420.f));
    ImGui::Columns(4, NULL, true);
    ImGui::SetColumnWidth(0, dispWidth_ - 30.f - 230.f);
    ImGui::SetColumnWidth(1, 70.f);
    ImGui::SetColumnWidth(2, 70.f);
    ImGui::SetColumnWidth(3, 70.f);
    ImGui::Text("Name");
    ImGui::NextColumn();
    ImGui::Text("Grade");
    ImGui::NextColumn();
    ImGui::Text("Hidden");
    ImGui::NextColumn();
    ImGui::Text("Unlocked");
    ImGui::NextColumn();
    for (int i = 0; i < sz; ++i) {
        auto &t = trophies_[i];
        char hiddenname[32];
        if (ImGui::Selectable(t.name.empty() ? (snprintf(hiddenname, 32, "Hidden##%d", i), hiddenname) : t.name.c_str(), trophyIdx_ == i, ImGuiSelectableFlags_SpanAllColumns))
            trophyIdx_ = i;
        ImGui::NextColumn();
        ImGui::Text(getGradeName(t.grade));
        ImGui::NextColumn();
        if (t.hidden)
            ImGui::Text("YES");
        ImGui::NextColumn();
        if (t.unlocked)
            ImGui::Text("YES");
        ImGui::NextColumn();
    }
    ImGui::ListBoxFooter();
    if (trophyIdx_ >= 0 && trophyIdx_ < sz && trophies_[trophyIdx_].grade != 1 && !trophies_[trophyIdx_].unlocked) {
        if (ImGui::Button("Unlock")) {
            cmd_->unlockTrophy(trophies_[trophyIdx_].id, trophies_[trophyIdx_].hidden);
        }
    }
    if (trophyPlat_ >= 0 && !trophies_[trophyPlat_].unlocked) {
        if (ImGui::Button("Unlock All")) {
            uint32_t data[4];
            for (auto &t: trophies_) {
                if (t.hidden)
                    data[t.id >> 5] |= 1U << (t.id & 0x1F);
            }
            cmd_->unlockAllTrophy(data);
        }
    }
}
