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

        Resources::Preload();

        TilesetRef tileset = Resources::LoadTileset("summer");
        level = Level(glm::ivec2(10), tileset);
        Camera::Get().SetBounds(level.map.Size());

        Level::Load("res/ignored/tst.json", level);

        FactionControllerRef dummy_faction = std::make_shared<FactionController>();

        level.objects.Add(Building(level, Resources::LoadBuilding("human/town_hall"), dummy_faction, glm::vec2(2.f, 2.f)));
        trollID = level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/troll"), dummy_faction, glm::vec2(0.f, 0.f));

        colorPalette = ColorPalette(true);
        colorPalette.UpdateShaderValues(shader);

        Camera::Get().SetupZoomUpdates(true);
        Camera::Get().ZoomToFit(glm::vec2(12.f));

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

    // static bool g = true;
    // if(g) ImGui::ShowDemoWindow(&g);

    //======================

    colorPalette.Bind(shader);

    if(input.rmb.down()) {
        Unit& troll = level.objects.GetUnit(trollID);

        glm::ivec2 target_pos = glm::ivec2(camera.GetMapCoords(input.mousePos_n * 2.f - 1.f) + 0.5f);
        ObjectID target_id;
        switch(rmb_commandID) {
            case CommandType::MOVE:
                ENG_LOG_INFO("MOVE TO {}", target_pos);
                if(level.map.IsWithinBounds(target_pos)) {
                    troll.IssueCommand(Command::Move(target_pos));
                }
                break;
            case CommandType::ATTACK:
                target_id = level.objects.GetObjectAt(target_pos);
                ENG_LOG_INFO("ATTACK {} (at {})", target_id, target_pos);
                if(ObjectID::IsValid(target_id)) {
                    troll.IssueCommand(Command::Attack(target_id));
                }
                break;
        }
    }

    if(input.lmb.down()) {
        glm::ivec2 coords = glm::ivec2(camera.GetMapCoords(input.mousePos_n * 2.f - 1.f) + 0.5f);
        ObjectID id = level.objects.GetObjectAt(coords);
        ENG_LOG_INFO("CLICK at {} - ID = {}", coords, id);
    }
    
    level.Update();

    level.Render();

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
        
        level.objects.DBG_GUI();

        ImGui::Begin("Command");
        ImGui::Combo("type", &rmb_commandID, "Idle\0Move\0Attack\0");
        ImGui::End();

        level.map.DBG_GUI();
    }
#endif
}

void Sandbox::ReloadShaders() {
    shader = std::make_shared<Shader>("res/shaders/cycling_shader");
    shader->InitTextureSlots(Renderer::TextureSlotsCount());
    colorPalette.UpdateShaderValues(shader);
}
