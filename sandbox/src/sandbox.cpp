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

        tex = std::make_shared<Texture>("res/textures/test2.png");

    } catch(std::exception& e) {
        LOG_INFO("ERROR: {}", e.what());
        LOG_ERROR("Failed to load resources; Terminating...");
        throw e;
    }

    guiStyle = std::make_shared<GUI::Style>();
    guiStyle->font = font;
    guiStyle->color = glm::vec4(1.f);
    guiStyle->textColor = glm::vec4(1.f, 0.f, 1.f, 1.f);
    guiStyle->textScale = 1.f;

    // guiStyle->textColor = glm::vec4(1.f);
    // guiStyle->color = glm::vec4(0.f);

    std::stringstream ss;
    for (int i = 0; i < 15; i++) {
        ss << "This is a sample gg,g' text. ";
    }
    text = ss.str();
    snprintf(buf, sizeof(char)*BUF_LEN, "%s", text.c_str());

    gui = GUI::ScrollText(glm::vec2(0.f), glm::vec2(0.5f, 0.25f), 0.f, guiStyle, text);
    tstGui = GUI::TextLabel(glm::vec2(0.25f, 0.75f), glm::vec2(0.1f, 0.1f), 0.f, guiStyle, "RENDER YOU PIECE OF SHEET.");

    SpritesheetData ssData = {};
    ssData.texture = tex;
    ssData.name = "test_spriteshseet";
    for(int i = 0; i < 4; i++) {
        SpriteData sd = {};
        sd.name = std::string("sprite_") + std::to_string(i);
        sd.size = glm::ivec2(256);
        sd.offset = glm::ivec2(256*(i%2), 256*(i/2));
        ssData.sprites.insert({ sd.name, Sprite(tex, sd) });
    }
    spritesheet = std::make_shared<Spritesheet>(ssData);

    sg = SpriteGroup(spritesheet->begin()->second);
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
    
    gui.Render();
    tstGui.Render();

    font->RenderTextCentered("KUNDA SEM, KUNDA TAM", glm::vec2(-0.25f, 0.75f), 2.f, glm::vec4(1.f));

    int i = 0;
    for(auto& [a, b] : (*spritesheet)) {
        b.Render(glm::uvec4(0), glm::vec3(0.25f - 0.75f * (i%2), 0.25f - 0.75f * (i/2), -0.999f), glm::vec2(0.5f));
        i++;
    }

    static glm::vec2 world_pos = glm::vec2(0.f, 0.f);
    static glm::vec2 size = glm::vec2(0.5f);

    sg.Render(glm::vec3((world_pos - camera.Position()) * camera.Mult(), -0.9999f), size * camera.Mult(), glm::uvec4(0));
    
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

        ImGui::Begin("ScrollText");
        static glm::vec4 clr = glm::vec4(1.f);
        if(ImGui::ColorPicker4("bg color", (float*)&clr)) {
            guiStyle->color = clr;
        }
        ImGui::SliderFloat("font scale", &guiStyle->textScale, 0.1f, 5.f);
        static float lineGap = 1.1f;
        if(ImGui::SliderFloat("line gap", &lineGap, 0.1f, 5.f))
            gui.SetLineGap(lineGap);
        ImGui::Separator();
        static float t = 0.f, line = 0.f;
        if(ImGui::SliderFloat("pos_norm", &t, 0.f, 1.f)) {
            gui.SetPositionNormalized(t);
            line = gui.GetPosition();
        }
        if(ImGui::SliderFloat("pos", &line, -((float)gui.NumLines()), (float)gui.NumLines())) {
            gui.SetPosition(line);
            t = gui.GetPositionNormalized();
        }
        if(ImGui::Button("TOP")) {
            gui.SetPositionPreset(GUI::ScrollTextPosition::TOP);
            line = gui.GetPosition();
            t = line / (float)gui.NumLines();
        }
        ImGui::SameLine();
        if(ImGui::Button("BOT")) {
            gui.SetPositionPreset(GUI::ScrollTextPosition::BOT);
            line = gui.GetPosition();
            t = line / (float)gui.NumLines();
        }
        ImGui::SameLine();
        if(ImGui::Button("LAST")) {
            gui.SetPositionPreset(GUI::ScrollTextPosition::LAST_LINE_TOP);
            line = gui.GetPosition();
            t = line / (float)gui.NumLines();
        }
        ImGui::SameLine();
        if(ImGui::Button("GONE")) {
            gui.SetPositionPreset(GUI::ScrollTextPosition::LAST_LINE_GONE);
            line = gui.GetPosition();
            t = line / (float)gui.NumLines();
        }
        ImGui::SameLine();
        if(ImGui::Button("FIRST")) {
            gui.SetPositionPreset(GUI::ScrollTextPosition::FIRST_LINE_BOT);
            line = gui.GetPosition();
            t = line / (float)gui.NumLines();
        }
        ImGui::SameLine();
        if(ImGui::Button("HIDDEN")) {
            gui.SetPositionPreset(GUI::ScrollTextPosition::FIRST_LINE_GONE);
            line = gui.GetPosition();
            t = line / (float)gui.NumLines();
        }
        
        ImGui::Separator();
        if(ImGui::InputTextMultiline("text", buf, sizeof(char)*BUF_LEN)) {
            text = std::string(buf);
            gui.UpdateText(text);
        }

        ImGui::End();

        ImGui::Begin("Spritesheet");
        spritesheet->DBG_GUI();
        ImGui::End();
    }
#endif
}

void Sandbox::ReloadShaders() {
    shader = std::make_shared<Shader>("res/shaders/test_shader");
    shader->InitTextureSlots(Renderer::TextureSlotsCount());
}
