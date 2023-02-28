#include "game.h"

#include "menu.h"

using namespace eng;

//TODO: add some precursor to main menu
//TODO: add OnDrag() to GUI detection - for map implementation (or some similar name, drag, down, hold, whatever)
//TODO: make logging initialization through singleton -> static objects cannot really use logging now since they get initialized before the logging

//TODO: remove SelectionHandler completely - use GUI hierarchy for GUI selection and cells for map selection
//TODO: think through how to handle resources - for example during menu initializations ... how to pass various things to correct places
//TODO: do input key handling through callbacks

//TODO: add some config file that will act as a persistent storage for options and stuff

//immediate future:
//TODO: add scroll menu to gui
//TODO: add basic play button with proper stage change & setup
//TODO: add cmdline args to launch in debug mode, skip menus, etc.

//TODO: figure out how to generate (nice) button textures - with borders and stuff
//TODO: add on hover and click button visuals
//TODO: font resizing to match current window size (mainly in buttons)

//TODO: stage switching, stage transitions (once there's ingame stage controller)

/*scroll menu impl:
    - there will be fixed number of buttons in it
    - buttons themselves won't move, scrolling will just move content among the generated buttons
        - buttons not moving -> can use regular mouse selection method
    - there will be 3 special buttons -> scroll slider (up,down & slider button)
        - can maybe wrap this in one single class
*/

Game::Game() : App(640, 480, "game") {}

void Game::OnResize(int width, int height) {
    LOG_INFO("Resize triggered.");
}

void Game::OnInit() {
    try {
        font = std::make_shared<Font>("res/fonts/PermanentMarker-Regular.ttf", 48);

        texture = std::make_shared<Texture>("res/textures/test2.png");
        btnTexture = std::make_shared<Texture>("res/textures/test_button.png");
        // backgroundTexture = std::make_shared<Texture>("res/textures/TitleMenu_BNE.png");

        ReloadShaders();
    } catch(std::exception& e) {
        LOG_INFO("ERROR: {}", e.what());
        LOG_ERROR("Failed to load resources; Terminating...");
        throw e;
    }

    stageController.Initialize({ 
        std::make_shared<MainMenuController>(font, btnTexture, backgroundTexture)
    });

    // Window::Get().SetFullscreen(true);

    // int width = 10;
    // int height = 10;
    // uint8_t* data = new uint8_t[width * height * 3];
    // btn = std::make_shared<Texture>(TextureParams::CustomData(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE), data);
    // delete[] data;
}

static InputButton t = InputButton(GLFW_KEY_T);

void Game::OnUpdate() {
    Window& window = Window::Get();
    Input& input = Input::Get();
    Camera& camera = Camera::Get();

    input.Update();
    camera.Update();

    Renderer::StatsReset();

    if(window.GetKeyState(GLFW_KEY_Q))
        window.Close();


    t.Update();
    if(t.down()) {
        static bool fullscreen = false;
        fullscreen = !fullscreen;
        LOG_INFO("SETTING FULLSCREEN = {}", fullscreen);
        window.SetFullscreen(fullscreen);
    }

    // window.UpdateViewport_full();
    // window.Clear();
    window.UpdateViewport();

    stageController.Update();

    Renderer::Begin(shader, true);
    stageController.Render();
    Renderer::End();
}

void Game::OnGUI() {
#ifdef ENGINE_ENABLE_GUI
    Audio::DBG_GUI();
    Camera::Get().DBG_GUI();

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
#endif
}

void Game::ReloadShaders() {
    shader = std::make_shared<Shader>("res/shaders/test_shader");
    shader->InitTextureSlots(Renderer::TextureSlotsCount());
}