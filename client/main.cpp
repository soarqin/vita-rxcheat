#define _CRT_SECURE_NO_WARNINGS

#include "net.h"
#include "command.h"

#include "imgui.h"
#include "imgui_impl_glfw_gl3.h"

#include <stdio.h>
#include <windows.h>
#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

enum :int {
    WIN_WIDTH = 640,
    WIN_HEIGHT = 640,
};

static void glfw_error_callback(int error, const char* description) {
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

int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    UdpClient::init();
    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_RESIZABLE, 0);
    GLFWwindow* window = glfwCreateWindow(WIN_WIDTH, WIN_HEIGHT, "VITA Remote Cheat Client", NULL, NULL);
//     const GLFWvidmode *mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
//     glfwSetWindowPos(window, (mode->width - WIN_WIDTH) / 2, (mode->height - WIN_HEIGHT) / 2);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    gl3wInit2(glfwGetProcAddress);

    // Setup ImGui binding
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplGlfwGL3_Init(window, true);

    ImGui::StyleColorsDark();
    auto &style = ImGui::GetStyle();
    style.WindowRounding = 0.f;
    style.ItemSpacing = ImVec2(5.f, 5.f);

    //io.Fonts->AddFontDefault();
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

    UdpClient client;

    Command cmd(client);
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
    std::vector<SearchResult> search_result;
    int search_status = 0;
    int search_result_type = 0;
    int search_result_idx = -1;
    int trophy_status = 0;
    int trophy_idx = -1;
    int trophy_plat = -1;
    std::vector<TrophyInfo> trophies;
    client.setOnRecv([&](int op, const char *buf, int len) {
        if (op < 0x100) {
            search_result.clear();
            search_status = 1;
            search_result_type = op;
        } else {
            switch (op) {
                case 0x8000: {
                    if (len < 240) break;
                    int id = *(int*)buf;
                    if (id >= (int)trophies.size()) trophies.resize(id + 1);
                    TrophyInfo &ti = trophies[id];
                    ti.id = id;
                    ti.grade = *(int*)(buf+4);
                    ti.hidden = *(int*)(buf+8) != 0;
                    ti.unlocked = *(int*)(buf+12) != 0;
                    ti.name = buf + 16;
                    ti.desc = buf + 80;
                    if (ti.grade == 1) trophy_plat = id;
                    break;
                }
                case 0x8001: {
                    trophy_status = 0;
                    break;
                }
                case 0x8002: {
                    trophy_status = 2;
                    break;
                }
                case 0x8100: {
                    int idx = *(int*)buf;
                    int platidx = *(int*)(buf + 4);
                    if (idx >= 0 && idx < (int)trophies.size()) {
                        trophies[idx].unlocked = true;
                    }
                    if (platidx >= 0) {
                        trophies[platidx].unlocked = true;
                    }
                    break;
                }
                case 0x8110: {
                    break;
                }
                case 0x10000: {
                    if (search_status != 1) break;
                    search_result_idx = -1;
                    len /= 12;
                    const uint32_t *res = (const uint32_t*)buf;
                    for (int i = 0; i < len; ++i) {
                        char hex[16], value[64];
                        uint32_t addr = res[i * 3];
                        snprintf(hex, 16, "%08X", addr);
                        cmd.formatTypeData(value, search_result_type, &res[i * 3 + 1]);
                        SearchResult sr = {addr, hex, value};
                        search_result.push_back(sr);
                    }
                    break;
                }
                case 0x20000:
                    search_status = 2;
                    break;
                case 0x30000:
                    search_result.clear();
                    search_status = 3;
                    break;
                case 0x30001:
                    search_result.clear();
                    search_status = 4;
                    break;
            }
        }
    });
    char ip[256] = "172.27.15.216";
    char searchVal[32] = "";
    bool hexSearch = false;
    bool heapSearch = false;
    bool s1 = true, s2 = false;
    int combo_index = 0;
    int tab_index = 0;
    const char* combo_items[] = {"int32", "uint32",
        "int16", "uint16", "int8", "uint8",
        "int64", "uint64", "float", "double",
    };
    const int combo_item_type[] = {Command::st_i32, Command::st_u32,
        Command::st_i16, Command::st_u16, Command::st_i8, Command::st_u8,
        Command::st_i64, Command::st_u64, Command::st_float, Command::st_double,
    };

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        client.process();
        glfwPollEvents();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        ImGui_ImplGlfwGL3_NewFrame();
        char title[256];
        snprintf(title, 256, "%.1f FPS", io.Framerate);
        glfwSetWindowTitle(window, title);

        {
            ImGui::Begin("", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
            ImGui::SetWindowPos(ImVec2(10.f, 10.f));
            ImGui::SetWindowSize(ImVec2(display_w - 20.f, display_h - 20.f));
            if (client.isConnected()) {
                ImGui::Text(client.titleId().c_str()); ImGui::SameLine(); ImGui::Text(client.title().c_str());
                if (ImGui::Button("Disconnect", ImVec2(100.f, 0.f))) {
                    client.disconnect();
                }
            } else {
                ImGui::Text("Not connected");
                if (ImGui::Button("Connect", ImVec2(100.f, 0.f))) {
                    client.connect(ip, 9527);
                }
            }
            ImGui::SameLine(125.f);
            ImGui::Text("IP Address:"); ImGui::SameLine();
            ImGui::PushItemWidth(200.f);
            ImGui::InputText("##IP", ip, 256, client.isConnected() ? ImGuiInputTextFlags_ReadOnly : 0);
            ImGui::PopItemWidth();
            if (client.isConnected()) {
                ImGui::RadioButton("Memory Searcher", &tab_index, 0);
                ImGui::SameLine();
                ImGui::RadioButton("Trophy", &tab_index, 1);
                switch (tab_index) {
                    case 0: {
                        if (search_status == 1) {
                            ImGui::Button("Searching...", ImVec2(100.f, 0.f));
                        } else {
                            if (ImGui::Button("Search!", ImVec2(100.f, 0.f))) {
                                uint32_t number = strtoull(searchVal, NULL, hexSearch ? 16 : 10);
                                cmd.startSearch(Command::st_i32, heapSearch, &number);
                            }
                        }
                        ImGui::SameLine(125.f);
                        ImGui::Text("Value:"); ImGui::SameLine();
                        ImGui::PushItemWidth(200.f);
                        ImGui::InputText("##Value", searchVal, 31, hexSearch ? ImGuiInputTextFlags_CharsHexadecimal : ImGuiInputTextFlags_CharsDecimal);
                        ImGui::PopItemWidth();
                        ImGui::SameLine();
                        ImGui::PushItemWidth(100.f);
                        if (ImGui::BeginCombo("##Type", combo_items[combo_index], 0)) {
                            for (int i = 0; i < 10; ++i) {
                                bool selected = combo_index == i;
                                if (ImGui::Selectable(combo_items[i], selected)) {
                                    combo_index = i;
                                }
                                if (selected) {
                                    ImGui::SetItemDefaultFocus();
                                }
                            }
                            ImGui::EndCombo();
                        }
                        ImGui::PopItemWidth();
                        ImGui::SameLine();
                        if (ImGui::Checkbox("HEX", &hexSearch)) {
                            searchVal[0] = 0;
                        }
                        ImGui::SameLine();
                        ImGui::Checkbox("HEAP", &heapSearch);

                        switch(search_status) {
                            case 2: {
                                ImGui::Text("Search result");
                                ImGui::ListBoxHeader("##Result");
                                ImGui::Columns(2, NULL, true);
                                int sz = search_result.size();
                                for (int i = 0; i < sz; ++i) {
                                    if (ImGui::Selectable(search_result[i].hexaddr.c_str(), search_result_idx == i, ImGuiSelectableFlags_SpanAllColumns))
                                        search_result_idx = i;
                                    ImGui::NextColumn();
                                    ImGui::Text(search_result[i].value.c_str());
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
                        break;
                    }
                    case 1: {
                        switch (trophy_status) {
                            case 0:
                            case 2:
                                if (ImGui::Button("Refresh trophies")) {
                                    trophy_status = 1;
                                    trophy_idx = -1;
                                    trophy_plat = -1;
                                    trophies.clear();
                                    cmd.refreshTrophy();
                                }
                                break;
                            case 1:
                                ImGui::Text("Refreshing");
                                break;
                        }
                        int sz = trophies.size();
                        if (sz == 0) break;
                        ImGui::ListBoxHeader("##Trophies", ImVec2(display_w - 30.f, 420.f));
                        ImGui::Columns(4, NULL, true);
                        ImGui::SetColumnWidth(0, display_w - 30.f - 230.f);
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
                            auto &t = trophies[i];
                            char hiddenname[32];
                            if (ImGui::Selectable(t.name.empty() ? (sprintf(hiddenname, "Hidden##%d", i), hiddenname) : t.name.c_str(), trophy_idx == i, ImGuiSelectableFlags_SpanAllColumns))
                                trophy_idx = i;
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
                        if (trophy_idx >= 0 && trophy_idx < sz && trophies[trophy_idx].grade != 1 && !trophies[trophy_idx].unlocked) {
                            if (ImGui::Button("Unlock")) {
                                cmd.unlockTrophy(trophies[trophy_idx].id);
                            }
                        }
                        if (trophy_plat >= 0 && !trophies[trophy_plat].unlocked) {
                            if (ImGui::Button("Unlock All")) {
                                cmd.unlockAllTrophy();
                            }
                        }
                        break;
                    }
                }
                ImGui::End();
            }
        }

        // Rendering
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();

    UdpClient::finish();
    return 0;
}
