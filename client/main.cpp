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
    WIN_WIDTH = 1280,
    WIN_HEIGHT = 720,
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

    //io.Fonts->AddFontDefault();
    ImFontAtlas *f = io.Fonts;
    if (!f->AddFontFromFileTTF("C:/Windows/Fonts/msyh.ttc", 16.0f, NULL, f->GetGlyphRangesChinese())
        && !f->AddFontFromFileTTF("C:/Windows/Fonts/simsun.ttc", 16.0f, NULL, f->GetGlyphRangesChinese()))
        f->AddFontDefault();

    UdpClient client;
    bool connected = false;

    int searchNum = 0;
    Command cmd(client);
    char ip[256] = "172.27.15.216";

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        client.process();
        glfwPollEvents();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        ImGui_ImplGlfwGL3_NewFrame();

        {
            ImGui::Begin("", NULL, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);
            ImGui::SetWindowPos(ImVec2(5.f, 5.f));
            ImGui::SetWindowSize(ImVec2(display_w - 10.f, display_h - 10.f));
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::InputText("IP Address", ip, 256, connected ? ImGuiInputTextFlags_ReadOnly : 0);
            if (connected) {
                if (ImGui::InputInt("Search", &searchNum, 1, 100, ImGuiInputTextFlags_EnterReturnsTrue)) {
                    cmd.startSearch(Command::st_i32, &searchNum);
                }
            } else {
                ImGui::SameLine();
                if (ImGui::Button("Connect")) {
                    connected = client.connect(ip, 9527);
                }
            }
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
