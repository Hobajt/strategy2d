#include "engine/game/app.h"

#include "engine/utils/log.h"
#include "engine/utils/dbg_gui.h"
#include "engine/core/renderer.h"
#include "engine/core/audio.h"

#include "engine/game/resources.h"

namespace eng {

    App::App(int windowWidth, int windowHeight, const char* windowName) {
        Log::Initialize();

        Window::Get().Initialize(windowWidth, windowHeight, windowName, float(windowWidth) / windowHeight);
        Window::Get().SetResizeCallbackHandler(this);

        DBG_GUI::Initialize();
        Audio::Initialize();
        Renderer::Initialize();
        
        ENG_LOG_TRACE("[C] App");
    }

    App::~App() {
        Resources::Release();
        Renderer::Release();
        Audio::Release();
        DBG_GUI::Release();

        ENG_LOG_TRACE("[D] App");
    }

    int App::Run() {
        Window& window = Window::Get();

        ENG_LOG_INFO("App::OnInit");
        OnInit();

        ENG_LOG_INFO("App::MainLoop");
        while(!window.ShouldClose()) {
            DBG_GUI::Begin();

            OnUpdate();

#ifdef ENGINE_ENABLE_GUI
            OnGUI();
#endif

            DBG_GUI::End();

            window.SwapAndPoll();
        }

        ENG_LOG_INFO("App::Terminating");
        return 0;
    }

    void App::OnResize(int width, int height) {
        Resources::OnResize(width, height);
    }

}//namespace eng
