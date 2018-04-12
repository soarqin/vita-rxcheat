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
    WIN_HEIGHT = 800,
};

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "Error %d: %s\n", error, description);
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
    bool connected = false;

    Command cmd(client);
    struct SearchResult  {
        uint32_t addr;
        std::string hexaddr;
        std::string value;
    };
    std::vector<SearchResult> result;
    int status = 0;
    int result_type = 0;
    int result_idx = -1;
    client.setOnRecv([&](int op, const char *buf, int len) {
        if (op < 0x100) {
            result.clear();
            status = 1;
            result_type = op;
        } else {
            switch (op) {
                case 0x10000: {
                    if (status != 1) break;
                    result_idx = -1;
                    len /= 12;
                    const uint32_t *res = (const uint32_t*)buf;
                    for (int i = 0; i < len; ++i) {
                        char hex[16], value[64];
                        uint32_t addr = res[i * 3];
                        snprintf(hex, 16, "%08X", addr);
                        cmd.formatTypeData(value, result_type, &res[i * 3 + 1]);
                        SearchResult sr = {addr, hex, value};
                        result.push_back(sr);
                    }
                    break;
                }
                case 0x20000:
                    status = 2;
                    break;
                case 0x30000:
                    result.clear();
                    status = 3;
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
    const char* combo_items[] = {"int32", "uint32",
        "int16", "uint16", "int8", "uint8",
        "int64", "uint64", "float", "double",
    };
    const int combo_item_type[] = {Command::st_i32, Command::st_u32,
        Command::st_i16, Command::st_u16, Command::st_i8, Command::st_u8,
        Command::st_i64, Command::st_u64, Command::st_float, Command::st_double,
    };

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
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
            if (connected) {
                if (ImGui::Button("Disconnect", ImVec2(100.f, 0.f))) {
                    client.disconnect();
                    connected = false;
                }
            } else {
                if (ImGui::Button("Connect", ImVec2(100.f, 0.f))) {
                    connected = client.connect(ip, 9527);
                }
            }
            ImGui::SameLine(125.f);
            ImGui::Text("IP Address:"); ImGui::SameLine();
            ImGui::PushItemWidth(200.f);
            ImGui::InputText("##IP", ip, 256, connected ? ImGuiInputTextFlags_ReadOnly : 0);
            ImGui::PopItemWidth();
            if (connected) {
                ImGui::Dummy(ImVec2(0.f, 50.f));
                if (ImGui::Button("Search!", ImVec2(100.f, 0.f))) {
                    uint32_t number = strtoull(searchVal, NULL, hexSearch ? 16 : 10);
                    cmd.startSearch((heapSearch ? 0x100 : 0) | Command::st_i32, &number);
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
                ImGui::Checkbox("HEAP", &heapSearch);
            }
            ImGui::Text("Search result");
            ImGui::ListBoxHeader("##Result");
            ImGui::Columns(2, NULL, true);
            if (status == 2) {
                int sz = result.size();
                for (int i = 0; i < sz; ++i) {
                    if (ImGui::Selectable(result[i].hexaddr.c_str(), result_idx == i, ImGuiSelectableFlags_SpanAllColumns))
                        result_idx = i;
                    ImGui::NextColumn();
                    ImGui::Text(result[i].value.c_str());
                    ImGui::NextColumn();
                }
            }
            ImGui::ListBoxFooter();
            ImGui::End();
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
