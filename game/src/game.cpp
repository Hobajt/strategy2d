#include "game.h"

#include "menu.h"

using namespace eng;

//TODO: add some precursor to main menu
//TODO: create more elaborate main(); add cmd arguments to launch in some kind of debug mode (skip menus, test map, etc.)
//TODO: think through the screen size & resizing -> use square screen like the game?
//TODO: add scroll menu to gui
//TODO: add OnDrag() to GUI detection - for map implementation (or some similar name, drag, down, hold, whatever)
//TODO: make logging initialization through singleton -> static objects cannot really use logging now since they get initialized before the logging

//TODO: remove SelectionHandler completely - use GUI hierarchy for GUI selection and cells for map selection

Game::Game() : App(640, 640, "game") {}

void Game::OnResize(int width, int height) {
    LOG_INFO("Resize triggered.");
}

// static GUI::Element el = {};

void Game::OnInit() {
    try {
        font = std::make_shared<Font>("res/fonts/PermanentMarker-Regular.ttf", 48);

        texture = std::make_shared<Texture>("res/textures/test2.png");
        btnTexture = std::make_shared<Texture>("res/textures/test_button.png");

        ReloadShaders();
    } catch(std::exception& e) {
        LOG_INFO("ERROR: {}", e.what());
        LOG_ERROR("Failed to load resources; Terminating...");
        throw e;
    }

    stageController.Initialize({ 
        std::make_shared<MainMenuController>(font, btnTexture)
    });

    // el = GUI::Element(glm::vec2(0.f), glm::vec2(0.75f), 0.f, nullptr, glm::vec4(1.f), nullptr);
    // GUI::Element* c = &el;
    // c = c->AddChild(new GUI::Button(glm::vec2(-1.f), glm::vec2(2.f/3.f), 1.f, nullptr, glm::vec4(1.f, 0.f, 0.f, 1.f), nullptr, nullptr, 1));
    // c = c->AddChild(new GUI::Button(glm::vec2(1.f), glm::vec2(0.5f), 1.f, nullptr, glm::vec4(0.f, 0.f, 1.f, 1.f), nullptr, nullptr, 2));
}

static glm::vec2 pos = glm::vec2(0.f);

void Game::OnUpdate() {
    Window& window = Window::Get();
    Input& input = Input::Get();
    Camera& camera = Camera::Get();

    input.Update();
    camera.Update();

    Renderer::StatsReset();

    if(window.GetKeyState(GLFW_KEY_Q))
        window.Close();

    stageController.Update();

    // GUI::Element* e = el.ResolveMouseSelection(input.mousePos_n);
    // if(e != nullptr) {
    //     if(input.lmb.down()) e->OnClick();
    //     else e->OnHover();
    // }

    Renderer::Begin(shader, true);
    stageController.Render();
    // el.Render();
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