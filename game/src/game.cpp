#include "game.h"

#include "intro.h"
#include "menu.h"
#include "recap.h"
#include "ingame.h"

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
    try {
        for(int i = 1; i < argc; i++) {
            if(strncmp(argv[i], "--fullscreen", 12) == 0) {
                fullscreen = true;
            }
#ifdef ENGINE_DEBUG
            else if(strncmp(argv[i], "--stage", 7) == 0 && i < argc-1) {
                dbg_stageIdx = GameStage::name2idx(std::string(argv[++i]));
            }
            else if (strncmp(argv[i], "--state", 7) == 0 && i < argc-1) {
                dbg_stageStateIdx = std::stoi(argv[++i]);
            }
#endif
        }
    } catch(std::exception&) {
        LOG_DEBUG("invalid cmdline args, skipping...");
#ifdef ENGINE_DEBUG
        dbg_stageIdx = dbg_stageStateIdx = -1;
#endif
    }
#ifdef ENGINE_DEBUG
    //stage = game stage idx, state = info for the stage controller 
    LOG_DEBUG("args: stage: {}, state: {}", dbg_stageIdx, dbg_stageStateIdx);
#endif
}

void Game::OnResize(int width, int height) {
    App::OnResize(width, height);
    LOG_INFO("Resize triggered ({}x{})", width, height);
}

void Game::OnInit() {
    try {
        //TODO: add resources method that generates textures & possibly preloads stuff
        //don't call it initialize, to distinct it from init methods in Renderer, Audio, etc.

        ReloadShaders();
        Resources::Preload();

        colorPalette = ColorPalette(true);
        colorPalette.UpdateShaderValues(shader);

    } catch(std::exception& e) {
        LOG_INFO("ERROR: {}", e.what());
        LOG_ERROR("Failed to load resources; Terminating...");
        throw e;
    }

    Window::Get().SetFullscreen(fullscreen);
    Resources::CursorIcons::SetIcon(CursorIconName::HAND_OC);
        

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

#ifdef ENGINE_DEBUG
static InputButton t = InputButton(GLFW_KEY_T);
static InputButton o = InputButton(GLFW_KEY_O);
#endif

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

#ifdef ENGINE_DEBUG
    if(window.GetKeyState(GLFW_KEY_Q))
        window.Close();
#endif

#ifdef ENGINE_ENABLE_GUI
    gui_btn.Update();
    if(gui_btn.down())
        gui_enabled = !gui_enabled;
#endif

    //TODO: remove these once the game is finished
#ifdef ENGINE_DEBUG
    t.Update();
    if(t.down()) {
        static bool fullscreen = false;
        fullscreen = !fullscreen;
        LOG_INFO("SETTING FULLSCREEN = {}", fullscreen);
        window.SetFullscreen(fullscreen);
    }

    o.Update();
    if(o.down()) {
        Config::ToggleCameraPanning();
        LOG_INFO("CAMERA PANNING = {}", Config::CameraPanning());
    }
#endif

    stageController.Update();

    Renderer::Begin(shader, true);

    colorPalette.Bind(shader);
    stageController.Render();

    Renderer::End();
}

void Game::OnGUI() {
#ifdef ENGINE_ENABLE_GUI
    if(gui_enabled) {
        Config::DBG_GUI();
        Audio::DBG_GUI();
        Camera::Get().DBG_GUI();
        stageController.DBG_GUI();

        ImGui::Begin("General");
        ImGui::Text("FPS: %.1f", Input::Get().fps);
        ImGui::Text("Draw calls: %d | Total quads: %d (%d wasted)", Renderer::Stats().drawCalls, Renderer::Stats().totalQuads, Renderer::Stats().wastedQuads);
        ImGui::Text("Textures: %d", Renderer::Stats().numTextures);
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
    shader = Resources::LoadShader("cycling_shader", true);
    shader->InitTextureSlots(Renderer::TextureSlotsCount());
    colorPalette.UpdateShaderValues(shader);
}
