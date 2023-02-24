#include <stdio.h>

#include <engine/engine.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace eng;

void DBG_GUI();

int main(int argc, char** argv) {

    Log::Initialize();
    Audio::Initialize();

    Window& window = Window::Get();
    Input& input = Input::Get();

    window.Initialize(640, 480, "test");
    GUI::Initialize();

    ShaderRef shader = std::make_shared<Shader>("res/shaders/test_shader");
    shader->InitTextureSlots(Renderer::TextureSlotsCount());

    FontRef font = std::make_shared<Font>("res/fonts/PermanentMarker-Regular.ttf", 48);

    TextureRef texture = std::make_shared<Texture>("res/textures/test2.png");

    //===================

    // Audio::PlayMusic("res/sounds/test_sound1.mp3");

    static glm::vec2 pos = glm::vec2(0.f);

    //TODO: test fine colors on linux (not sure about the ansi color codes)
    ENG_LOG_TRACE("=================");
    ENG_LOG_FINEST("TEST FINEST");
    ENG_LOG_FINER("TEST FINER");
    ENG_LOG_FINE("TEST FINE");
    ENG_LOG_TRACE("TEST TRACE");
    ENG_LOG_TRACE("=================");

    InputButton s, d, f;
    while(!window.ShouldClose()) {
        input.Update();
        if(window.GetKeyState(GLFW_KEY_Q))
            window.Close();

        s.Update(window.GetKeyState(GLFW_KEY_S));
        d.Update(window.GetKeyState(GLFW_KEY_D));
        f.Update(window.GetKeyState(GLFW_KEY_F));
        if(s.down()) Audio::Play("res/sounds/test_sound.mp3");
        if(d.down()) Audio::Play("res/sounds/test_sound.mp3", pos);
        if(f.down()) Audio::PlayMusic("res/sounds/test_sound.mp3");

        Renderer::StatsReset();

        GUI::Begin();
        Renderer::Begin(shader, true);

#ifdef ENGINE_ENABLE_GUI
        ImGui::Begin("Sound effect");
        ImGui::SliderFloat2("position", (float*)&pos, -10.f, 10.f);
        ImGui::End();
#endif

        Renderer::RenderQuad(Quad::FromCenter(glm::vec3(-0.5f, 0.f, 0.f), glm::vec2(0.2f, 0.2f), glm::vec4(1.f, 0.f, 0.f, 1.f)));
        Renderer::RenderQuad(Quad::FromCenter(glm::vec3( 0.5f, 0.f, 0.f), glm::vec2(0.2f, 0.2f), glm::vec4(1.f), texture));

        font->RenderText("Sample text.", glm::vec2(0.f, -0.75f), 1.f, glm::vec4(1.f));
        font->RenderTextCentered("Centered sample text.", glm::vec2(0.f, -0.5f), 1.f, glm::vec4(1.f, 0.f, 0.9f, 1.f));

        Renderer::End();

        DBG_GUI();
        GUI::End();

        window.SwapAndPoll();
    }

    Audio::Release();
    GUI::Release();
    return 0;
}

void DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Begin("General");
    ImGui::Text("FPS: %.1f", Input::Get().fps);
    ImGui::Text("Draw calls: %d | Total quads: %d (%d wasted)", Renderer::Stats().drawCalls, Renderer::Stats().totalQuads, Renderer::Stats().wastedQuads);
    ImGui::End();

    Audio::DBG_GUI();
#endif
}
