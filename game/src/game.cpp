#include "game.h"

#include "intro.h"
#include "menu.h"
#include "recap.h"
#include "ingame.h"

#include "resources.h"

using namespace eng;

//TODO: add some precursor to main menu
//TODO: make logging initialization through singleton -> static objects cannot really use logging now since they get initialized before the logging
//TODO: consider remaking GUI's transform system - it's kinda unintuitive to define sizes relative to parent

//TODO: remove SelectionHandler completely - use GUI hierarchy for GUI selection and cells for map selection
//TODO: think through how to handle resources - for example during menu initializations ... how to pass various things to correct places
//TODO: do input key handling through callbacks

//TODO: add some config file that will act as a persistent storage for options and stuff

//less immediate future:
//TODO: add cmdline args to launch in debug mode, skip menus, etc.
//TODO: figure out how to generate (nice) button textures - with borders and marble like (or whatever it is they have)
//TODO: start adding audio

//TODO: merge generated textures into a single atlas (to optimize render calls)
//TODO: maybe add text rendering method that works for bounding box - easier text alignment management
//TODO: implement GUI text alignment - might need different text rendering approach (bounding box)

//TODO: add basic play button with proper stage change & setup

//TODO: add some support to pass structs in between game stages (through transition) - try avoiding dynamic allocation for it

/*TODO: - introController
    - figure out proper way to play video files - for intro cinematic
    - conditionally skip the cinematic (based on value from configs - if it's not a first launch)
*/

//TODO: try replacing the void* in game stage transitions


Game::Game() : App(640, 480, "game") {}

Game::~Game() {
    Resources::Release();
}

void Game::OnResize(int width, int height) {
    LOG_INFO("Resize triggered ({}x{})", width, height);
    Resources::OnResize(width, height);
}

void Game::OnInit() {
    try {
        Resources::Initialize();
        ReloadShaders();

    } catch(std::exception& e) {
        LOG_INFO("ERROR: {}", e.what());
        LOG_ERROR("Failed to load resources; Terminating...");
        throw e;
    }

    // Window::Get().SetFullscreen(true);

    stageController.Initialize({ 
        std::make_shared<IntroController>(),
        std::make_shared<MainMenuController>(),
        std::make_shared<RecapController>(),
        std::make_shared<IngameController>(),
    });
}

static InputButton t = InputButton(GLFW_KEY_T);

#ifdef ENGINE_ENABLE_GUI
static bool gui_enabled = false;
static InputButton gui_btn = InputButton(GLFW_KEY_P);
#endif

void Game::OnUpdate() {
    Window& window = Window::Get();
    Input& input = Input::Get();
    Camera& camera = Camera::Get();

    input.Update();
    camera.Update();

    Renderer::StatsReset();

    if(window.GetKeyState(GLFW_KEY_Q))
        window.Close();

#ifdef ENGINE_ENABLE_GUI
    gui_btn.Update();
    if(gui_btn.down())
        gui_enabled = !gui_enabled;
#endif

    t.Update();
    if(t.down()) {
        static bool fullscreen = false;
        fullscreen = !fullscreen;
        LOG_INFO("SETTING FULLSCREEN = {}", fullscreen);
        window.SetFullscreen(fullscreen);
    }

    stageController.Update();

    Renderer::Begin(shader, true);
    stageController.Render();
    Renderer::End();
}

void Game::OnGUI() {
#ifdef ENGINE_ENABLE_GUI
    if(gui_enabled) {
        Audio::DBG_GUI();
        Camera::Get().DBG_GUI();
        stageController.DBG_GUI();

        ImGui::Begin("General");
        ImGui::Text("FPS: %.1f", Input::Get().fps);
        ImGui::Text("Draw calls: %d | Total quads: %d (%d wasted)", Renderer::Stats().drawCalls, Renderer::Stats().totalQuads, Renderer::Stats().wastedQuads);
        if(ImGui::Button("Reload shaders")) {
            try {
                ReloadShaders();
            }
            catch(std::exception& e) {
                LOG_ERROR("ReloadShaders failed...");
                //throw e;
            }
            LOG_INFO("Shaders reloaded.");
        }
        ImGui::End();
    }
#endif
}

void Game::ReloadShaders() {
    shader = Resources::LoadShader("test_shader", true);
}
