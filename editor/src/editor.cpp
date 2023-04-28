#include "editor.h"

using namespace eng;

Editor::Editor(int argc, char** argv) : App(1200, 900, "Editor") {}

constexpr float fontScale = 0.055f;

void Editor::OnResize(int width, int height) {
    LOG_INFO("Resize triggered ({}x{})", width, height);
    font->Resize(fontScale * height);
}

static std::string text = "";
#define BUF_LEN (1 << 20)
static char* buf = new char[BUF_LEN];

void Editor::OnInit() {
    try {
        shader = std::make_shared<Shader>("res/shaders/test_shader");
        shader->InitTextureSlots(Renderer::TextureSlotsCount());

        font = std::make_shared<Font>("res/fonts/PermanentMarker-Regular.ttf", int(fontScale * Window::Get().Height()));
    } catch(std::exception& e) {
        LOG_INFO("ERROR: {}", e.what());
        LOG_ERROR("Failed to load resources; Terminating...");
        throw e;
    }

    btn_toggleFullscreen = InputButton(GLFW_KEY_T);
    btn_toggleGUI = InputButton(GLFW_KEY_P);
}

void Editor::OnUpdate() {
    Window& window = Window::Get();
    Input& input = Input::Get();
    Camera& camera = Camera::Get();

    input.Update();
    camera.Update();

    Renderer::StatsReset();

    if(window.GetKeyState(GLFW_KEY_Q))
        window.Close();

#ifdef ENGINE_ENABLE_GUI
    btn_toggleGUI.Update();
    if(btn_toggleGUI.down())
        guiEnabled = !guiEnabled;
#endif

    btn_toggleFullscreen.Update();
    if(btn_toggleFullscreen.down()) {
        static bool fullscreen = false;
        fullscreen = !fullscreen;
        LOG_INFO("SETTING FULLSCREEN = {}", fullscreen);
        window.SetFullscreen(fullscreen);
    }

    Renderer::Begin(shader, true);
    
    Renderer::End();
}

void Editor::OnGUI() {
#ifdef ENGINE_ENABLE_GUI
    if(guiEnabled) {
        Audio::DBG_GUI();
        Camera::Get().DBG_GUI();

        ImGui::Begin("General");
        ImGui::Text("FPS: %.1f", Input::Get().fps);
        ImGui::Text("Draw calls: %d | Total quads: %d (%d wasted)", Renderer::Stats().drawCalls, Renderer::Stats().totalQuads, Renderer::Stats().wastedQuads);
        ImGui::Text("Textures: %d", Renderer::Stats().numTextures);
        ImGui::End();

        switch(fileMenu.Update()) {
            case FileMenuSignal::NEW:
                Terrain_SetupNew();
                break;
            case FileMenuSignal::LOAD:
            {
                int res = Terrain_Load();
                if(res == 0)
                    fileMenu.Reset();
                else
                    fileMenu.SignalError("File not found.");
                break;
            }
            case FileMenuSignal::SAVE:
            {
                int res = Terrain_Save();
                if(res == 0)
                    fileMenu.Reset();
                else
                    fileMenu.SignalError("No clue.");
                break;
            }
            case FileMenuSignal::QUIT:
                Window::Get().Close();
                break;
        }

        
    }
#endif
}

void Editor::Terrain_SetupNew() {
    //TODO:

    //init terrain based on values from FileMenu, reset the camera
}

int Editor::Terrain_Load() {
    //TODO:
    return 0;
}

int Editor::Terrain_Save() {
    //TODO:
    return 1;
}
