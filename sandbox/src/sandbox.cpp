#include "sandbox.h"

#include <sstream>

using namespace eng;

Sandbox::Sandbox(int argc, char** argv) : App(1200, 900, "sandbox") {}

constexpr float fontScale = 0.055f;

void Sandbox::OnResize(int width, int height) {
    LOG_INFO("Resize triggered ({}x{})", width, height);
    font->Resize(fontScale * height);
}

static std::string text = "";
#define BUF_LEN (1 << 20)
static char* buf = new char[BUF_LEN];

void Sandbox::OnInit() {
    try {
        ReloadShaders();
        // font = std::make_shared<Font>("res/fonts/PermanentMarker-Regular.ttf", int(fontScale * Window::Get().Height()));
        font = std::make_shared<Font>("res/fonts/OpenSans-Regular.ttf", int(fontScale * Window::Get().Height()));

        TilesetRef tileset = Resources::LoadTileset("summer");
        map = Map(glm::ivec2(10), tileset);
        Camera::Get().SetBounds(map.Size());

        texture = std::make_shared<Texture>("res/textures/troll.png", TextureParams(GL_NEAREST, GL_CLAMP_TO_EDGE));
        // texture = std::make_shared<Texture>("res/textures/cross.png", TextureParams(GL_NEAREST, GL_CLAMP_TO_EDGE));

        colorPalette = ColorPalette(true);
        colorPalette.UpdateShaderValues(shader);

    } catch(std::exception& e) {
        LOG_INFO("ERROR: {}", e.what());
        LOG_ERROR("Failed to load resources; Terminating...");
        throw e;
    }
}

static InputButton t = InputButton(GLFW_KEY_T);

#ifdef ENGINE_ENABLE_GUI
static bool gui_enabled = true;
static InputButton gui_btn = InputButton(GLFW_KEY_P);
#endif

void Sandbox::OnUpdate() {
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

    Renderer::Begin(shader, true);

    //======================

    colorPalette.Bind(shader);

    // map.Render();
    glm::vec2 size = glm::vec2(texture->Size()) / float(texture->Size().y);
    Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f), size, glm::vec4(1.f), texture).SetPaletteIdx((float)paletteIndex));

    if(whiteBackground)
        Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f, 0.f, 0.1f), glm::vec2(1.f), glm::vec4(1.f), nullptr));

    //======================
    
    Renderer::End();
}

void Sandbox::OnGUI() {
#ifdef ENGINE_ENABLE_GUI
    if(gui_enabled) {
        Audio::DBG_GUI();
        Camera::Get().DBG_GUI();

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

        ImGui::Begin("Sandbox stuff");

        ImGui::Checkbox("white background", &whiteBackground);
        ImGui::SliderInt("Palette index", &paletteIndex, 0, colorPalette.Size().y);

        ImGui::End();

        ImGui::Begin("Palettes");
        colorPalette.GetTexture()->DBG_GUI();
        ImGui::End();
    }
#endif
}

void Sandbox::ReloadShaders() {
    shader = std::make_shared<Shader>("res/shaders/cycling_shader");
    shader->InitTextureSlots(Renderer::TextureSlotsCount());
    colorPalette.UpdateShaderValues(shader);
}
