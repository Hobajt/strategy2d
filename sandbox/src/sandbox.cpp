#include "sandbox.h"

#include <sstream>

using namespace eng;

#include <GLFW/glfw3.h>

Sandbox::Sandbox(int argc, char** argv) : App(1200, 900, "sandbox") {}

constexpr float fontScale = 0.055f;

void Sandbox::OnResize(int width, int height) {
    LOG_INFO("Resize triggered ({}x{})", width, height);
    font->Resize(fontScale * height);
}

static std::string text = "";
#define BUF_LEN (1 << 20)
static char* buf = new char[BUF_LEN];

void MockIngameStageController::RegisterKeyCallback() {
    Input::Get().AddKeyCallback(-1, [](int keycode, int modifiers, bool single_press, void* handler){
        static_cast<MockIngameStageController*>(handler)->SignalKeyPress(keycode, modifiers, single_press);
    }, true, this);
}

void MockIngameStageController::PauseRequest(bool pause) {
    paused = pause;
    Input::Get().SetPaused(paused);
}

void MockIngameStageController::PauseToggleRequest() {
    paused = !paused;
    Input::Get().SetPaused(paused);
}

void MockIngameStageController::ChangeLevel(const std::string& filename) {
    switch_signaled = true;
    switch_filepath = filename;
    LOG_INFO("ChangeLevel queued.");
}

void MockIngameStageController::LevelSwitching(Level& level) {
    //NOTE: switching is split this way to avoid triggering level overrides from within objects referenced by the level instance
    //this way, switch is only signaled from the code and then performed later when it's safe to do so
    if(switch_signaled) {
        LOG_INFO("Switching level to '{}'.", switch_filepath);
        level.Release();
        Level::Load(switch_filepath, level);

        LinkController(level.factions.Player());
        switch_signaled = false;
    }
}

#define LOAD_FROM_SAVEFILE

void Sandbox::OnInit() {
    Config::Reload();

    try {
        ReloadShaders();
        // font = std::make_shared<Font>("res/fonts/PermanentMarker-Regular.ttf", int(fontScale * Window::Get().Height()));
        font = std::make_shared<Font>("res/fonts/OpenSans-Regular.ttf", int(fontScale * Window::Get().Height()));

        Resources::Preload();

        TilesetRef tileset = Resources::LoadTileset("summer");
        level = Level(glm::ivec2(10), tileset);

        // //TODO: link an actual ingame stage after the level init (sandbox has none tho, so using mock instead)
        ingameStage = {};
        ingameStage.RegisterKeyCallback();

        Camera::Get().SetBounds(level.map.Size());
        Camera::Get().SetupZoomUpdates(true);
        Camera::Get().ZoomToFit(glm::vec2(12.f));
        Camera::Get().SetGUIBoundsOffset(0.5f);     //based on GUI width (1/4 of screen; 2 = entire screen)

        Command::EnableSwitching(false);

#ifdef LOAD_FROM_SAVEFILE
        // Level::Load("res/saves/AJJ.json", level);
        // Level::Load("res/saves/CHUJA.json", level);
        Level::Load("res/saves/all.json", level);
#else
        Level::Load("res/ignored/tst.json", level);
        
        // temporary faction initialization - will be done on level loads or during custom game initialization
        FactionsFile tst = {};
        tst.factions.push_back(FactionsFile::FactionEntry(FactionControllerID::NATURE,        0, "neutral", 5, 0));
        tst.factions.push_back(FactionsFile::FactionEntry(FactionControllerID::LOCAL_PLAYER,  0, "faction_1", 0, 1));
        tst.factions.push_back(FactionsFile::FactionEntry(FactionControllerID::INVALID,       0, "faction_2", 1, 2));
        tst.diplomacy.push_back({1, 2, 1});
        level.factions = Factions(std::move(tst), level.map.Size(), level.map);
        level.map.UploadOcclusionMask(level.factions.Player()->Occlusion(), level.factions.Player()->ID());

        FactionControllerRef f_n = level.factions[0];
        FactionControllerRef f1 = level.factions[1];
        FactionControllerRef f2 = level.factions[2];
#endif

        ingameStage.LinkController(level.factions.Player());

#ifndef LOAD_FROM_SAVEFILE
        level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/catapult"),   f1, glm::vec2(3.f, 5.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/ballista"), f1, glm::vec2(3.f, 4.f), false);

        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/church"), f1, glm::vec2(27.f, 25.f), true);
        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/barracks"), f1, glm::vec2(24.f, 25.f), true);
        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/mage_tower"), f1, glm::vec2(21.f, 25.f), true);

        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("orc/altar"), f1, glm::vec2(27.f, 22.f), true);
        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("orc/barracks"), f1, glm::vec2(24.f, 22.f), true);
        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("orc/temple"), f1, glm::vec2(21.f, 22.f), true);

        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/blacksmith"), f1, glm::vec2(21.f, 19.f), true);
        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("orc/blacksmith"), f1, glm::vec2(24.f, 19.f), true);

        Techtree& t = level.factions.Player()->Tech();
        for(int i = ResearchType::PALA_UPGRADE; i < ResearchType::COUNT; i++) {
            t.IncrementResearch(i, false);
            t.IncrementResearch(i, true);
        }
        t.RecalculateBoth();

        ObjectID id_mg = level.objects.EmplaceUnit(level, Resources::LoadUnit("human/mage"), f1, glm::vec2(20.f, 27.f), false);
        ObjectID id_dk = level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/death_knight"), f1, glm::vec2(20.f, 23.f), false);
        ObjectID id_pl = level.objects.EmplaceUnit(level, Resources::LoadUnit("human/paladin"), f1, glm::vec2(20.f, 26.f), false);
        ObjectID id_om = level.objects.EmplaceUnit(level, Resources::LoadUnit("human/demosquad"), f1, glm::vec2(20.f, 24.f), false);
        // level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/death_knight"), f2, glm::vec2(20.f, 22.f), false);

        // level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/catapult"), f1, glm::vec2(18.f, 24.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("misc/pig"), f1, glm::vec2(17.f, 24.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("misc/eye"), f1, glm::vec2(16.f, 24.f), false);

        level.objects.GetUnit(id_mg).Mana(255);
        level.objects.GetUnit(id_dk).Mana(255);
        level.objects.GetUnit(id_pl).Mana(255);
        level.objects.GetUnit(id_om).Mana(255);

        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/gnomish_inventor"), f1, glm::vec2(24.f, 16.f), true);
        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("orc/goblin_laboratory"), f1, glm::vec2(21.f, 16.f), true);
        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/watch_tower"), f1, glm::vec2(7.f, 5.f), true);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"),   f1, glm::vec2(15.f, 15.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"),   f1, glm::vec2(16.f, 15.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"),   f1, glm::vec2(15.f, 16.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"),   f1, glm::vec2(16.f, 16.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"),   f1, glm::vec2(15.f, 17.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"),   f1, glm::vec2(16.f, 17.f), false);

        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/cannon_tower"), f1, glm::vec2(7.f, 26.f), true);
        ObjectID un = level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"),   f2, glm::vec2(6.f, 26.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"),   f2, glm::vec2(9.f, 25.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/archer"),   f1, glm::vec2(4.f, 4.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/dragon"),   f1, glm::vec2(6.f, 4.f), false);
        level.objects.Add(Building(level, Resources::LoadBuilding("human/town_hall"), f1, glm::vec2(0.f, 0.f), true));
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/peasant"), f1, glm::vec2(5.f, 5.f), false);

        ObjectID ol2 = level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/oil_platform"), f2, glm::vec2(20.f, 6.f), true);
        ObjectID shipyard = level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/shipyard"), f1, glm::vec2(14.f, 1.f), true);
        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/foundry"), f1, glm::vec2(14.f, 4.f), true);
        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("orc/oil_refinery"), f1, glm::vec2(14.f, 7.f), true);

        ObjectID ol = level.objects.EmplaceBuilding(level, Resources::LoadBuilding("misc/oil"), f_n, glm::vec2(22.f, 8.f), true);
        ObjectID gm = level.objects.EmplaceBuilding(level, Resources::LoadBuilding("misc/gold_mine"), f_n, glm::vec2(7.f, 7.f), true);
        level.objects.GetBuilding(ol).SetAmountLeft(123456);
        level.objects.GetBuilding(gm).SetAmountLeft(654321);
        level.objects.GetBuilding(ol2).SetAmountLeft(100);

        // level.objects.EmplaceUnit(level, Resources::LoadUnit("human/destroyer"), f1, glm::vec2(18.f, 4.f), false);
        // level.objects.EmplaceUnit(level, Resources::LoadUnit("human/submarine"), f1, glm::vec2(20.f, 4.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/transport"), f2, glm::vec2(18.f, 12.f), false);
        // level.objects.EmplaceUnit(level, Resources::LoadUnit("human/tanker"), f1, glm::vec2(22.f, 8.f), false);

        // ObjectID sub = level.objects.EmplaceUnit(level, Resources::LoadUnit("human/submarine"), f2, glm::vec2(18.f, 2.f), false);
        // ObjectID en  = level.objects.EmplaceUnit(level, Resources::LoadUnit("human/destroyer"), f2, glm::vec2(20.f, 2.f), false);

        // level.objects.EmplaceUnit(level, Resources::LoadUnit("human/submarine"), f2, glm::vec2(18.f, 12.f), false);

        glm::ivec3 d = level.objects.GetBuilding(gm).Data()->num_id;
        ENG_LOG_INFO("GM NUM_ID: ({}, {}, {})", d.x, d.y, d.z);

        Camera::Get().Center(glm::vec2(16.f, 7.f));
#endif

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
        Audio::Play("res/sounds/test_sound.mp3", glm::vec2(10.f, 10.f));
        // static bool fullscreen = false;
        // fullscreen = !fullscreen;
        // LOG_INFO("SETTING FULLSCREEN = {}", fullscreen);
        // window.SetFullscreen(fullscreen);
    }

    Renderer::Begin(shader, true);

    // static bool g = true;
    // if(g) ImGui::ShowDemoWindow(&g);

    //======================

    colorPalette.Bind(shader);
    
    level.Update();

    level.Render();
    level.factions.Player()->Render();

    ingameStage.LevelSwitching(level);

    //======================
    
    Renderer::End();
}

void Sandbox::OnGUI() {
#ifdef ENGINE_ENABLE_GUI
    if(gui_enabled) {
        Config::DBG_GUI();
        Audio::DBG_GUI();
        Camera::Get().DBG_GUI();
        TextureGenerator::DBG_GUI();
        Resources::DBG_GUI();

        ImGui::Begin("MAPVEIW");
        level.factions.Player()->GetMapView().GetTexture()->DBG_GUI();
        ImGui::End();

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
        static bool cmd_switching = Command::SwitchingEnabled();
        if(ImGui::Button(cmd_switching ? "Disable Command Switching" : "Enable Command Switching")) {
            cmd_switching = !cmd_switching;
            Command::EnableSwitching(cmd_switching);
        }
        ImGui::End();

        ImGui::Begin("Sandbox stuff");

        ImGui::Checkbox("white background", &whiteBackground);
        ImGui::SliderInt("Palette index", &paletteIndex, 0, colorPalette.Size().y);

        ImGui::End();
        
        level.objects.DBG_GUI();

        level.map.DBG_GUI();

        level.factions.DBG_GUI();
    }
#endif
}

void Sandbox::ReloadShaders() {
    shader = std::make_shared<Shader>("res/shaders/cycling_shader");
    shader->InitTextureSlots(Renderer::TextureSlotsCount());
    colorPalette.UpdateShaderValues(shader);
}
