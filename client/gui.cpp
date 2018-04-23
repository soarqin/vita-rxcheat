#define _CRT_SECURE_NO_WARNINGS

#include "gui.h"

#include "lang.h"
#include "net.h"
#include "command.h"
#include "handler.h"
#include "../version.h"

#include "imgui.h"
#include "imgui_impl_glfw_gl3.h"

#include "yaml-cpp/yaml.h"

#ifdef _WIN32
#include "resource.h"

#include <windows.h>
#include <Shlwapi.h>
#endif
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#endif
#include <cstdio>
#include <fstream>
#include <stdexcept>

enum:int {
    WIN_WIDTH = 640,
    WIN_HEIGHT = 640,
};

static void glfwErrorCallback(int error, const char* description) {
    fprintf(stderr, "Error %d: %s\n", error, description);
}

inline const char *getGradeName(int grade) {
    switch (grade) {
        case 1: return LS(TROPHY_GRADE_PLATINUM); break;
        case 2: return LS(TROPHY_GRADE_GOLD); break;
        case 3: return LS(TROPHY_GRADE_SILVER); break;
        case 4: return LS(TROPHY_GRADE_BRONZE); break;
        default: return LS(TROPHY_GRADE_UNKNOWN); break;
    }
}

Gui::Gui() {
    std::vector<std::string> langs;
    g_lang.listLanguages(langs);
    g_lang.loadDefault();
    try {
        g_lang.load("chs");
    } catch(...) {}
    UdpClient::init();
    client_ = new UdpClient;
    client_->setOnConnected([&](const char *addr) {
        strncpy(ip_, addr, 256);
    });
    cmd_ = new Command(*client_);
    handler_ = new Handler(*this);

    loadData();

    client_->setOnRecv(std::bind(&Handler::process, handler_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit())
        throw std::runtime_error("Unable to initialize GLFW");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, 0);
    char title[256];
    snprintf(title, 256, "%s v" VERSION_STR, LS(WINDOW_TITLE));
    window_ = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, title, NULL, NULL);
#ifdef _WIN32
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAINICON));
    SendMessage(glfwGetWin32Window(window_), WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    SendMessage(glfwGetWin32Window(window_), WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
#endif
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

    io.IniFilename = NULL;
    ImFontAtlas *f = io.Fonts;
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x2000, 0x31FF, // Katakana Phonetic Extensions
        0x4E00, 0x9FAF, // CJK Ideograms
        0xFF00, 0xFFEF, // Half-width characters
        0,
    };
    if (!f->AddFontFromFileTTF("font.ttc", 18.0f, NULL, ranges)) {
        bool found = false;
        for (auto &p: g_lang.fonts()) {
            if (f->AddFontFromFileTTF(p.c_str(), 18.0f, NULL, ranges)) {
                found = true;  break;
            }
        }
        if (!found) f->AddFontDefault();
    }
}

Gui::~Gui() {
    saveData();
    delete handler_;
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
#ifndef NDEBUG
        char title[256];
        snprintf(title, 256, "%s v" VERSION_STR " - %.1f FPS", LS(WINDOW_TITLE), ImGui::GetIO().Framerate);
        glfwSetWindowTitle(window_, title);
#endif

        if (ImGui::Begin("", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize)) {
            connectPanel();
            if (client_->isConnected()) {
                tabPanel();
                switch (tabIndex_) {
                    case 0:
                        searchPanel();
                        searchPopup();
                        break;
                    case 2:
                        tablePanel();
                        tablePopup();
                        break;
                    case 3:
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

void Gui::searchResultStart(int type) {
    searchResults_.clear();
    searchStatus_ = 1;
    searchResultType_ = type;
}

void Gui::searchResult(const SearchVal *vals, int count) {
    for (int i = 0; i < count; ++i) {
        char hex[16], value[64];
        snprintf(hex, 16, "%08X", vals[i].addr);
        cmd_->formatTypeData(value, searchResultType_, &vals[i].val);
        MemoryItem sr = {vals[i].addr, searchResultType_, hex, value};
        searchResults_.push_back(sr);
    }
}

void Gui::searchEnd(int ret) {
    if (ret == 0 && searchResults_.empty())
        searchStatus_ = 0;
    else
        searchStatus_ = 2 + ret;
}

void Gui::trophyList(int id, int grade, bool hidden, bool unlocked, const char *name, const char *desc) {
    if (id >= (int)trophies_.size()) trophies_.resize(id + 1);
    TrophyInfo &ti = trophies_[id];
    ti.id = id;
    ti.grade = grade;
    ti.hidden = hidden;
    ti.unlocked = unlocked;
    ti.name = name;
    ti.desc = desc;
    if (ti.grade == 1) trophyPlat_ = id;
}

void Gui::trophyListEnd() {
    trophyStatus_ = 0;
}

void Gui::trophyListErr() {
    trophyStatus_ = 2;
}

void Gui::trophyUnlocked(int idx, int platidx) {
    if (idx >= 0 && idx < (int)trophies_.size()) {
        trophies_[idx].unlocked = true;
    }
    if (platidx >= 0) {
        trophies_[platidx].unlocked = true;
    }
}

void Gui::trophyUnlockErr() {
}

void Gui::updateMemory(uint32_t addr, int type, const void *data) {
    for (auto &p: searchResults_) {
        if (p.addr == addr) {
            char value[64];
            cmd_->formatTypeData(value, type, data);
            p.type = type;
            p.value = value;
            break;
        }
    }
    for (auto &p: memTable_) {
        if (p.addr == addr) {
            char value[64];
            cmd_->formatTypeData(value, type, data);
            p.type = type;
            p.value = value;
            break;
        }
    }
}

inline void Gui::connectPanel() {
    ImGui::SetWindowPos(ImVec2(10.f, 10.f));
    ImGui::SetWindowSize(ImVec2(dispWidth_ - 20.f, dispHeight_ - 20.f));
    if (client_->isConnected()) {
        ImGui::Text("%s - %s", client_->titleId().c_str(), client_->title().c_str());
        if (ImGui::Button(LS(DISCONNECT), ImVec2(100.f, 0.f))) {
            searchResults_.clear();
            searchStatus_ = 0;
            trophies_.clear();
            trophyStatus_ = 0;
            client_->disconnect();
        }
    } else {
        if (client_->isConnecting()) {
            ImGui::Text(LS(CONNECTING));
            if (ImGui::Button(LS(DISCONNECT), ImVec2(100.f, 0.f))) {
                client_->disconnect();
            }
            ImGui::SameLine();
            ImGui::InvisibleButton(LS(AUTOCONNECT), ImVec2(100.f, 0.f));
        } else {
            ImGui::Text(LS(NOT_CONNECTED));
            if (ImGui::Button(LS(CONNECT), ImVec2(100.f, 0.f))) {
                client_->connect(ip_, 9527);
            }
            ImGui::SameLine();
            if (ImGui::Button(LS(AUTOCONNECT), ImVec2(100.f, 0.f))) {
                client_->autoconnect(9527);
            }
        }
    }
    ImGui::SameLine();
    ImGui::Text(LS(IP_ADDR)); ImGui::SameLine();
    ImGui::PushItemWidth(200.f);
    ImGui::InputText("##IP", ip_, 256, client_->isConnected() ? ImGuiInputTextFlags_ReadOnly : 0);
    ImGui::PopItemWidth();
}

inline void Gui::tabPanel() {
    ImGui::RadioButton(LS(MEM_SEARCHER), &tabIndex_, 0);
    /* TODO: memory table
    ImGui::SameLine();
    ImGui::RadioButton(LS(MEM_VIEWER), &tabIndex_, 1);
    ImGui::SameLine();
    ImGui::RadioButton(LS(MEM_TABLE), &tabIndex_, 2);
    */
    ImGui::SameLine();
    ImGui::RadioButton(LS(TROPHY), &tabIndex_, 3);
}

inline void formatData(int type, const char *src, bool isHex, void *dst) {
    switch (type) {
        case Command::st_autouint: case Command::st_u64:
        {
            uint64_t val = strtoull(src, NULL, isHex ? 16 : 10);
            memcpy(dst, &val, 8);
            break;
        }
        case Command::st_autoint: case Command::st_i64:
        {
            int64_t val = strtoll(src, NULL, isHex ? 16 : 10);
            memcpy(dst, &val, 8);
            break;
        }
        case Command::st_u32: case Command::st_u16: case Command::st_u8:
        {
            uint32_t val = strtoul(src, NULL, isHex ? 16 : 10);
            memcpy(dst, &val, 4);
            break;
        }
        case Command::st_i32: case Command::st_i16: case Command::st_i8:
        {
            int32_t val = strtol(src, NULL, isHex ? 16 : 10);
            memcpy(dst, &val, 4);
            break;
        }
        case Command::st_float:
        {
            float val = strtof(src, NULL);
            memcpy(dst, &val, 4);
            break;
        }
        case Command::st_double:
        {
            double val = strtod(src, NULL);
            memcpy(dst, &val, 8);
            break;
        }
    }
}

inline void Gui::searchPanel() {
    const int comboItemType[] = {Command::st_autoint, Command::st_autouint, Command::st_i32, Command::st_u32,
        Command::st_i16, Command::st_u16, Command::st_i8, Command::st_u8,
        Command::st_i64, Command::st_u64, Command::st_float, Command::st_double,
    };

    if (searchStatus_ == 1) {
        ImGui::Button(LS(SEARCHING), ImVec2(100.f, 0.f));
    } else {
        if (ImGui::Button(LS(NEW_SEARCH), ImVec2(100.f, 0.f)) && typeComboIndex_ >= 0) {
            char output[8];
            formatData(comboItemType[typeComboIndex_], searchVal_, hexSearch_, output);
            cmd_->startSearch(comboItemType[typeComboIndex_], heapSearch_, output);
        }
        if ((searchStatus_ == 2 || searchStatus_ == 3) && (ImGui::SameLine(), ImGui::Button(LS(NEXT_SEARCH), ImVec2(70.f, 0.f)) && typeComboIndex_ >= 0)) {
            char output[8];
            formatData(searchResultType_, searchVal_, hexSearch_, output);
            cmd_->nextSearch(output);
        }
    }
    ImGui::SameLine(); ImGui::Text(LS(VALUE)); ImGui::SameLine();
    ImGui::PushItemWidth(120.f);
    ImGui::InputText("##Value", searchVal_, 31, hexSearch_ ? ImGuiInputTextFlags_CharsHexadecimal : ImGuiInputTextFlags_CharsDecimal);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::PushItemWidth(100.f);
    if (ImGui::BeginCombo("##Type", LS(DATATYPE_FIRST + typeComboIndex_), 0)) {
        for (int i = 0; i < 10; ++i) {
            bool selected = typeComboIndex_ == i;
            if (ImGui::Selectable(LS(DATATYPE_FIRST + i), selected)) {
                if (typeComboIndex_ != i) {
                    typeComboIndex_ = i;
                    if (searchResultType_ != comboItemType[i] || searchResults_.empty()) {
                        searchStatus_ = 0;
                    } else {
                        searchStatus_ = 2;
                    }
                }
            }
            if (selected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Checkbox(LS(HEX), &hexSearch_)) {
        searchVal_[0] = 0;
    }
    ImGui::SameLine();
    ImGui::Checkbox(LS(HEAP), &heapSearch_);

    switch (searchStatus_) {
        case 2:
        {
            ImGui::Text(LS(SEARCH_RESULT));
            ImGui::ListBoxHeader("##Result");
            ImGui::Columns(2, NULL, true);
            int sz = searchResults_.size();
            for (int i = 0; i < sz; ++i) {
                bool selected = searchResultIdx_ == i;
                if (ImGui::Selectable(searchResults_[i].hexaddr.c_str(), selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowDoubleClick)) {
                    searchResultIdx_ = i;
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        strcpy(memEditVal_, searchResults_[i].value.c_str());
                        memEditing_ = true;
                    }
                }
                if (selected) ImGui::SetItemDefaultFocus();
                ImGui::NextColumn();
                ImGui::Text(searchResults_[i].value.c_str());
                ImGui::NextColumn();
            }
            ImGui::ListBoxFooter();
            if (searchResultIdx_ >= 0 && (ImGui::SameLine(), ImGui::Button(LS(EDIT_MEM)))) {
                strcpy(memEditVal_, searchResults_[searchResultIdx_].value.c_str());
                memEditing_ = true;
            }
            break;
        }
        case 3:
            ImGui::Text(LS(TOO_MANY_RESULTS));
            break;
        case 4:
            ImGui::Text(LS(SEARCH_IN_PROGRESS));
            break;
    }
}

void Gui::searchPopup() {
    if (!memEditing_) return;
    ImGui::OpenPopup(LS(POPUP_EDIT));
    if (ImGui::BeginPopupModal(LS(POPUP_EDIT), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        auto &mi = searchResults_[searchResultIdx_];
        ImGui::Text("%s: %s", LS(EDIT_ADDR), mi.hexaddr.c_str());
        ImGui::InputText("##EditValue", memEditVal_, 31, hexSearch_ ? ImGuiInputTextFlags_CharsHexadecimal : ImGuiInputTextFlags_CharsDecimal);
        ImGui::SameLine();
        if (ImGui::Checkbox(LS(HEX), &hexSearch_)) {
            searchVal_[0] = 0;
            memEditVal_[0] = 0;
        }
        if (ImGui::Button(LS(OK))) {
            memEditing_ = false;
            ImGui::CloseCurrentPopup();
            char output[8];
            formatData(mi.type, memEditVal_, hexSearch_, output);
            cmd_->modifyMemory(mi.type, mi.addr, output);
            memEditVal_[0] = 0;
        }
        ImGui::SameLine();
        if (ImGui::Button(LS(CANCEL))) {
            memEditing_ = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void Gui::tablePanel() {
}

void Gui::tablePopup() {
}

inline void Gui::trophyPanel() {
    switch (trophyStatus_) {
        case 0:
        case 2:
            if (ImGui::Button(LS(REFRESH_TROPHY))) {
                trophyStatus_ = 1;
                trophyIdx_ = -1;
                trophyPlat_ = -1;
                trophies_.clear();
                cmd_->refreshTrophy();
            }
            break;
        case 1:
            ImGui::Text(LS(REFRESHING));
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
    ImGui::Text(LS(TROPHY_NAME));
    ImGui::NextColumn();
    ImGui::Text(LS(TROPHY_GRADE));
    ImGui::NextColumn();
    ImGui::Text(LS(TROPHY_HIDDEN));
    ImGui::NextColumn();
    ImGui::Text(LS(TROPHY_UNLOCKED));
    ImGui::NextColumn();
    for (int i = 0; i < sz; ++i) {
        auto &t = trophies_[i];
        char hiddenname[32];
        if (ImGui::Selectable(t.name.empty() ? (snprintf(hiddenname, 32, LS(TROPHY_TITLE_HIDDEN), i), hiddenname) : t.name.c_str(), trophyIdx_ == i, ImGuiSelectableFlags_SpanAllColumns))
            trophyIdx_ = i;
        if (ImGui::IsItemHovered() && !t.desc.empty())
            ImGui::SetTooltip("%s", t.desc.c_str());
        ImGui::NextColumn();
        ImGui::Text(getGradeName(t.grade));
        ImGui::NextColumn();
        if (t.hidden)
            ImGui::Text(LS(YES));
        ImGui::NextColumn();
        if (t.unlocked)
            ImGui::Text(LS(YES));
        ImGui::NextColumn();
    }
    ImGui::ListBoxFooter();
    if (trophyIdx_ >= 0 && trophyIdx_ < sz && trophies_[trophyIdx_].grade != 1 && !trophies_[trophyIdx_].unlocked) {
        if (ImGui::Button(LS(TROPHY_UNLOCK))) {
            cmd_->unlockTrophy(trophies_[trophyIdx_].id, trophies_[trophyIdx_].hidden);
        }
    }
    if (trophyPlat_ >= 0 && !trophies_[trophyPlat_].unlocked) {
        if (ImGui::Button(LS(TROPHY_UNLOCK_ALL))) {
            uint32_t data[4];
            for (auto &t: trophies_) {
                if (t.hidden)
                    data[t.id >> 5] |= 1U << (t.id & 0x1F);
            }
            cmd_->unlockAllTrophy(data);
        }
    }
}

inline void getConfigFilePath(char *path) {
#ifdef _WIN32
    GetModuleFileNameA(NULL, path, 256);
    PathRemoveFileSpecA(path);
    PathAppendA(path, "rcsvr.yml");
#else
    strcpy(path, "rcsvr.yml");
#endif
}

void Gui::saveData() {
    char path[256];
    getConfigFilePath(path);
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "IPAddr" << YAML::Value << ip_;
    out << YAML::Key << "HEX" << YAML::Value << hexSearch_;
    out << YAML::Key << "HEAP" << YAML::Value << heapSearch_;
    out << YAML::EndMap;
    std::ofstream f(path);
    f << out.c_str();
    f.close();
}

void Gui::loadData() {
    char path[256];
    getConfigFilePath(path);
    YAML::Node node;
    try {
        node = YAML::LoadFile(path);
        strncpy(ip_, node["IPAddr"].as<std::string>().c_str(), 256);
        hexSearch_ = node["HEX"].as<bool>();
        heapSearch_ = node["HEAP"].as<bool>();
    } catch (...) { return; }
}
