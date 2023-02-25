#include "engine/game/app.h"

#include "engine/utils/log.h"
#include "engine/utils/gui.h"
#include "engine/core/renderer.h"
#include "engine/core/audio.h"

namespace eng {

    App::App(int windowWidth, int windowHeight, const char* windowName) {
        Log::Initialize();

        Window::Get().Initialize(windowWidth, windowHeight, windowName);
        Window::Get().SetResizeCallbackHandler(this);

        GUI::Initialize();
        Audio::Initialize();
        Renderer::Initialize();
        
        ENG_LOG_TRACE("[C] App");
    }

    App::~App() {
        Renderer::Release();
        Audio::Release();
        GUI::Release();

        ENG_LOG_TRACE("[D] App");
    }

    int App::Run() {
        Window& window = Window::Get();

        ENG_LOG_INFO("App::OnInit");
        OnInit();

        ENG_LOG_INFO("App::MainLoop");
        while(!window.ShouldClose()) {
            GUI::Begin();

            OnUpdate();

#ifdef ENGINE_ENABLE_GUI
            OnGUI();
#endif

            GUI::End();

            window.SwapAndPoll();
        }

        ENG_LOG_INFO("App::Terminating");
        return 0;
    }

}//namespace eng
