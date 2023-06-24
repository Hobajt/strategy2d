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

struct clr {
    uint8_t r,g,b,a;

    clr bgr2rgb() const { return clr{b,g,r,a}; }
};

constexpr int num_colors = 4;

eng::TextureRef CreateColorPalette(int& paletteCount) {
    int s = 16;

    std::vector<std::vector<clr>> palettes = {
        std::vector<clr>{
            {0, 4, 68, 255},
            {0, 4, 92, 255},
            {0, 0, 124, 255},
            {0, 0, 164, 255},
        },
        std::vector<clr>{
            {76, 4, 0, 255},
            {108, 20, 0, 255},
            {148, 36, 0, 255},
            {192, 60, 0, 255},
        },
        std::vector<clr>{
            {12, 40, 0, 255},
            {44, 84, 4, 255},
            {92, 132, 20, 255},
            {148, 180, 44, 255},
        },
        std::vector<clr>{
            {44, 8, 44, 255},
            {76, 16, 80, 255},
            {132, 48, 116, 255},
            {176, 72, 152, 255},
        },
        std::vector<clr>{
            {12, 32, 108, 255},
            {16, 56, 152, 255},
            {16, 88, 196, 255},
            {20, 132, 240, 255},
        },
        std::vector<clr>{
            {20, 12, 12, 255},
            {32, 20, 20, 255},
            {44, 28, 28, 255},
            {60, 40, 40, 255},
        },
        std::vector<clr>{
            {76, 40, 36, 255},
            {128, 84, 84, 255},
            {180, 152, 152, 255},
            {224, 224, 224, 255},
        },
        std::vector<clr>{
            {0, 116, 180, 255},
            {16, 160, 204, 255},
            {40, 204, 228, 255},
            {72, 252, 252, 255},
        },
    };
    paletteCount = (int)palettes.size()-1;
    
    int width = s*num_colors;
    int height = s * (int)palettes.size();

    int size = width * height;
    clr* data = new clr[size];
    for(int i = 0; i < size; i++)
        data[i] = { 0,0,0,0 };
    
    for(int y = 0; y < palettes.size(); y++) {
        auto& palette = palettes[y];
        for(int x = 0; x < num_colors; x++) {

            for(int j = 0; j < s; j++) {
                for(int k = 0; k < s; k++) {
                    data[(y*s+j)*width+x*s+k] = palette[x].bgr2rgb();
                }
            }
        }
    }

    TextureRef tex = std::make_shared<Texture>(TextureParams::CustomData(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST), data, "colorPalette");
    delete[] data;
    return tex;
}

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

        colorPalette = CreateColorPalette(maxPaletteIndex);
        shader->SetFloat("paletteIndex", 0.f);
        shader->SetVec2("paletteCentering", glm::vec2(1.f / (2.f * num_colors), 1.f / (2.f*maxPaletteIndex)));

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
    
    shader->SetInt("colorPalette", Renderer::ForceBindTexture(colorPalette));

    // map.Render();
    glm::vec2 size = glm::vec2(texture->Size()) / float(texture->Size().y);
    Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f), size, glm::vec4(1.f), texture));

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
        if(ImGui::SliderInt("Palette index", &paletteIndex, 0, maxPaletteIndex)) {
            shader->SetFloat("paletteIndex", float(paletteIndex)/maxPaletteIndex);
        }

        ImGui::End();

        ImGui::Begin("Palettes");
        colorPalette->DBG_GUI();
        ImGui::End();
    }
#endif
}

void Sandbox::ReloadShaders() {
    shader = std::make_shared<Shader>("res/shaders/cycling_shader");
    shader->InitTextureSlots(Renderer::TextureSlotsCount());
}
