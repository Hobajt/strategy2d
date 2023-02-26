#include "game.h"

using namespace eng;

//TODO: add some precursor to main menu
//TODO: create more elaborate main(); add cmd arguments to launch in some kind of debug mode (skip menus, test map, etc.)
//TODO: think through the screen size & resizing -> use square screen like the game?

Game::Game() : App(640, 640, "game") {}

void Game::OnResize(int width, int height) {
    LOG_INFO("Resize triggered.");
}

void Game::OnInit() {
    k1 = InputButton(GLFW_KEY_C);
    k2 = InputButton(GLFW_KEY_V);
    k3 = InputButton(GLFW_KEY_B);

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
}

static glm::vec2 pos = glm::vec2(0.f);

void Game::OnUpdate() {
    Window& window = Window::Get();
    Input& input = Input::Get();
    Camera& camera = Camera::Get();

    input.Update();
    camera.Update();

    
    switch(state) {
        case GameState::INTRO:
            //TODO: do intro
            state = GameState::MAIN_MENU;
            break;
        case GameState::MAIN_MENU:
            // menu.
            break;
        case GameState::INGAME:
            break;
    }

    static GUI::Menu tstMenu = GUI::Menu(glm::vec2(0.f), glm::vec2(0.5f), 0.f, std::vector<GUI::Button>{
        GUI::Button(glm::vec2(0.f, 0.31f * 2.f), glm::vec2(0.9f, 0.3f), 1.f, btnTexture, glm::vec4(0.3f, 0.3f, 0.3f, 1.f), font, "button 1", glm::vec4(1.f, 1.f, 1.f, 1.f)),
        GUI::Button(glm::vec2(0.f, 0.00f * 2.f), glm::vec2(0.9f, 0.3f), 1.f, btnTexture, glm::vec4(0.7f, 0.7f, 0.7f, 1.f), font, "button 2", glm::vec4(1.f, 0.f, .9f, 1.f)),
        GUI::Button(glm::vec2(0.f,-0.31f * 2.f), glm::vec2(0.9f, 0.3f), 1.f, btnTexture, glm::vec4(0.9f, 0.0f, 0.0f, 1.f), font, "button 3", glm::vec4(0.f, 0.f, 1.f, 1.f))
        // GUI::Button(glm::vec2(0.5f, 0.f), glm::vec2(0.5f, 0.3f), 1.f, nullptr, glm::vec4(0.7f, 0.7f, 0.7f, 1.f), font, "button 2", glm::vec4(1.f, 0.f, .9f, 1.f)),
    });

    ImGui::Begin("test gui");
    ImGui::SliderFloat2("menu size", (float*)&tstMenu.size, -1.f, 1.f);
    ImGui::SliderFloat2("btn size", (float*)&tstMenu.buttons[1].size, -1.f, 1.f);
    ImGui::SliderFloat2("btn offset", (float*)&tstMenu.buttons[1].offset, -1.f, 1.f);
    ImGui::End();

    Renderer::Begin(shader, true);
    tstMenu.Render();
    Renderer::End();

    // //game state transition logic
    // if(transition.IsHappening()) {
    //     float t = transition....;
    //     Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f), glm::vec2(1.f), glm::vec4(0.f, 0.f, 0.f, t)));
    //     if(transition.IsDone()) {

    //     }
    // }

    //============================================== old stuff

    if(window.GetKeyState(GLFW_KEY_Q))
        window.Close();
    
    // k1.Update();
    // k2.Update();
    // k3.Update();
    // if(k1.down()) Audio::Play("res/sounds/test_sound.mp3");
    // if(k2.down()) Audio::Play("res/sounds/test_sound.mp3", pos);
    // if(k3.down()) Audio::PlayMusic("res/sounds/test_sound.mp3");

    // Renderer::StatsReset();

    // Renderer::Begin(shader, true);

    // Renderer::RenderQuad(Quad::FromCenter((glm::vec3(-0.5f, 0.f, 0.f) - camera.Position3d()) * camera.Mult3d(), glm::vec2(0.2f, 0.2f) * camera.Mult(), glm::vec4(1.f, 0.f, 0.f, 1.f)));
    // Renderer::RenderQuad(Quad::FromCenter((glm::vec3( 0.5f, 0.f, 0.f) - camera.Position3d()) * camera.Mult3d(), glm::vec2(0.2f, 0.2f) * camera.Mult(), glm::vec4(1.f), texture));

    // font->RenderText("Sample text.", glm::vec2(0.f, -0.75f), 1.f, glm::vec4(1.f));
    // font->RenderTextCentered("Centered sample text.", glm::vec2(0.f, -0.5f), 1.f, glm::vec4(1.f, 0.f, 0.9f, 1.f));

    // Renderer::End();
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