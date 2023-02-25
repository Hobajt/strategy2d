#include <stdio.h>

#include <engine/engine.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace eng;

void DBG_GUI();

//TODO: add some precursor to main menu
//TODO: create more elaborate main(); add cmd arguments to launch in some kind of debug mode (skip menus, test map, etc.)
//TODO: setup a Game or App class
//TODO: add string methods for vector classes - for debug prints

int main(int argc, char** argv) {

    Log::Initialize();
    Audio::Initialize();

    Window& window = Window::Get();
    Input& input = Input::Get();
    Camera& camera = Camera::Get();

    window.Initialize(640, 480, "test");
    GUI::Initialize();

    ShaderRef shader = std::make_shared<Shader>("res/shaders/test_shader");
    shader->InitTextureSlots(Renderer::TextureSlotsCount());

    FontRef font = std::make_shared<Font>("res/fonts/PermanentMarker-Regular.ttf", 48);

    TextureRef texture = std::make_shared<Texture>("res/textures/test2.png");

    //===================

    // Audio::PlayMusic("res/sounds/test_sound1.mp3");

    static glm::vec2 pos = glm::vec2(0.f);

    // input.Update();

    InputButton k1,k2,k3;
    while(!window.ShouldClose()) {
        input.Update();
        camera.Update();
        if(window.GetKeyState(GLFW_KEY_Q))
            window.Close();

        k1.Update(window.GetKeyState(GLFW_KEY_C));
        k2.Update(window.GetKeyState(GLFW_KEY_V));
        k3.Update(window.GetKeyState(GLFW_KEY_B));
        if(k1.down()) Audio::Play("res/sounds/test_sound.mp3");
        if(k2.down()) Audio::Play("res/sounds/test_sound.mp3", pos);
        if(k3.down()) Audio::PlayMusic("res/sounds/test_sound.mp3");

        Renderer::StatsReset();

        GUI::Begin();
        Renderer::Begin(shader, true);

#ifdef ENGINE_ENABLE_GUI
        ImGui::Begin("Sound effect");
        ImGui::SliderFloat2("position", (float*)&pos, -10.f, 10.f);
        ImGui::End();
#endif

        Renderer::RenderQuad(Quad::FromCenter((glm::vec3(-0.5f, 0.f, 0.f) - camera.Position3d()) * camera.Mult3d(), glm::vec2(0.2f, 0.2f) * camera.Mult(), glm::vec4(1.f, 0.f, 0.f, 1.f)));
        Renderer::RenderQuad(Quad::FromCenter((glm::vec3( 0.5f, 0.f, 0.f) - camera.Position3d()) * camera.Mult3d(), glm::vec2(0.2f, 0.2f) * camera.Mult(), glm::vec4(1.f), texture));

        font->RenderText("Sample text.", glm::vec2(0.f, -0.75f), 1.f, glm::vec4(1.f));
        font->RenderTextCentered("Centered sample text.", glm::vec2(0.f, -0.5f), 1.f, glm::vec4(1.f, 0.f, 0.9f, 1.f));

        Renderer::End();

        DBG_GUI();
        GUI::End();

        window.SwapAndPoll();
    }

    Renderer::Release();
    Audio::Release();
    GUI::Release();

    ENG_LOG_INFO("Terminating...");
    return 0;
}

void DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Begin("General");
    ImGui::Text("FPS: %.1f", Input::Get().fps);
    ImGui::Text("Draw calls: %d | Total quads: %d (%d wasted)", Renderer::Stats().drawCalls, Renderer::Stats().totalQuads, Renderer::Stats().wastedQuads);
    ImGui::End();

    Audio::DBG_GUI();
    Camera::Get().DBG_GUI();
#endif
}
