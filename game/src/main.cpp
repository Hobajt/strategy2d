#include <stdio.h>

#include <engine/engine.h>

int main(int argc, char** argv) {
    eng::Log::Initialize();

    eng::Window& window = eng::Window::Get();
    eng::Input& input = eng::Input::Get();

    window.Initialize(640, 480, "test");
    eng::GUI::Initialize();

    while(!window.ShouldClose()) {
        input.Update();

        LOG_INFO("Mouse pos: ({}, {})", input.mousePos.x, input.mousePos.y);

        eng::GUI::Begin();
#ifdef ENGINE_ENABLE_GUI
        static bool show_demo_window = true;
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
#endif
        eng::GUI::End();

        window.SwapAndPoll();
    }

    eng::GUI::Release();
    
    return 0;
}
