#include "engine/utils/gui.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#ifdef ENGINE_ENABLE_GUI
    #include <imgui.h>
    #include <imgui_impl_glfw.h>
    #include <imgui_impl_opengl3.h>
    #include <imgui_internal.h>
#endif

#include "engine/core/window.h"

namespace eng::GUI {

    void Initialize() {
#ifdef ENGINE_ENABLE_GUI
        Window& window = Window::Get();
        if(!window.IsInitialized()) {
            ENG_LOG_ERROR("GUI::Init() must be called after window initialization.");
            throw std::exception();
        }

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
        //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        //ImGui::StyleColorsClassic();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window.Handle(), true);
        ImGui_ImplOpenGL3_Init(GLSL_VERSION_STRING);
#else
        Window& window = Window::Get();
        if(!window.IsInitialized()) {
            ENG_LOG_ERROR("GUI::Init() must be called after window initialization.");
            throw std::exception();
        }
        ENG_LOG_INFO("GUI::Init() - Debugging GUI features are not enabled (compile with ENGINE_ENABLE_GUI).");
#endif
    }

    void Release() {
#ifdef ENGINE_ENABLE_GUI
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
#endif
    }

    void Begin() {
#ifdef ENGINE_ENABLE_GUI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
#endif
    }

    void End() {
#ifdef ENGINE_ENABLE_GUI
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(Window::Get().Handle(), &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        // glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        // glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // Update and Render additional Platform Windows
        // (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
        //  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
#endif
    }

}//namespace eng::GUI
