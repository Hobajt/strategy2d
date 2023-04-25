#include "game.h"

#include "intro.h"
#include "menu.h"
#include "recap.h"
#include "ingame.h"

#include "resources.h"

using namespace eng;

//TODO: make logging initialization through singleton -> static objects cannot really use logging now since they get initialized before the logging
//TODO: consider remaking GUI's transform system - it's kinda unintuitive to define sizes relative to parent

//TODO: remove SelectionHandler completely - use GUI hierarchy for GUI selection and cells for map selection
//TODO: think through how to handle resources - for example during menu initializations ... how to pass various things to correct places
//TODO: do input key handling through callbacks

//TODO: add some config file that will act as a persistent storage for options and stuff

//less immediate future:
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
//TODO: fix font scaling - probably generate font at max required scale (act text size?) and then scale all other texts down

//TODO: GUI - rescale button hold offset when resizing (offset way too big in small screen)

//TODO: move shader->InitTextureSlots somewhere so that it's done automatically - spent an hour debugging malfunctioning textures only to find out i forgot to call this

//TODO: probably preload ingame assets on game start


Game::Game(int argc, char** argv) : App(640, 480, "game") {
#ifdef ENGINE_DEBUG
    try {
        for(int i = 1; i < argc-1; i++) {
            if(strncmp(argv[i], "--stage", 7) == 0) {
                dbg_stageIdx = GameStage::name2idx(std::string(argv[++i]));
            }
            else if (strncmp(argv[i], "--state", 7) == 0) {
                dbg_stageStateIdx = std::stoi(argv[++i]);
            }
        }
    } catch(std::exception&) {
        LOG_DEBUG("invalid cmdline args, skipping...");
        dbg_stageIdx = dbg_stageStateIdx = -1;
    }
    //stage = game stage idx, state = info for the stage controller 
    LOG_DEBUG("args: stage: {}, state: {}", dbg_stageIdx, dbg_stageStateIdx);
#endif
}

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

#ifdef ENGINE_DEBUG
    if(dbg_stageIdx != -1) {
        stageController.DBG_SetStage(dbg_stageIdx, dbg_stageStateIdx);
    }
#endif
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

        ImGui::Begin("font");
        Resources::DefaultFont()->GetTexture()->DBG_GUI();
        ImGui::End();
    }
#endif
}

void Game::ReloadShaders() {
    shader = Resources::LoadShader("test_shader", true);
}
