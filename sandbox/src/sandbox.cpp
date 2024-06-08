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

void Sandbox::OnInit() {
    Config::Reload();

    try {
        ReloadShaders();
        // font = std::make_shared<Font>("res/fonts/PermanentMarker-Regular.ttf", int(fontScale * Window::Get().Height()));
        font = std::make_shared<Font>("res/fonts/OpenSans-Regular.ttf", int(fontScale * Window::Get().Height()));

        Resources::Preload();

        TilesetRef tileset = Resources::LoadTileset("summer");
        level = Level(glm::ivec2(10), tileset);

        Level::Load("res/ignored/tst.json", level);
        
        //temporary faction initialization - will be done on level loads or during custom game initialization
        FactionsFile tst = {};
        tst.factions.push_back(FactionsFile::FactionEntry(FactionControllerID::NATURE,        0, "neutral", 5));
        tst.factions.push_back(FactionsFile::FactionEntry(FactionControllerID::LOCAL_PLAYER,  0, "faction_1", 0));
        tst.factions.push_back(FactionsFile::FactionEntry(FactionControllerID::INVALID,       0, "faction_2", 1));
        tst.diplomacy.push_back({1, 2, 1});
        level.factions = Factions(std::move(tst), level.map.Size());
        level.map.UploadOcclusionMask(level.factions.Player()->Occlusion(), level.factions.Player()->ID());

        FactionControllerRef f_n = level.factions[0];
        FactionControllerRef f1 = level.factions[1];
        FactionControllerRef f2 = level.factions[2];

        //TODO: link an actual ingame stage after the level init (sandbox has none tho, so using mock instead)
        ingameStage = {};
        ingameStage.LinkController(level.factions.Player());
        ingameStage.RegisterKeyCallback();

        //TODO: for custom games - override level.factions with new object (initialized based on starting locations & faction count)

        // trollID = level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/peon"), f1, glm::vec2(5.f, 5.f), false);
        // level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/catapult"), f1, glm::vec2(7.f, 5.f), false);
        // level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/dragon"), f1, glm::vec2(9.f, 5.f), false);
        // level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/farm"), f1, glm::vec2(5.f, 0.f));

        // level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/troll"),      f1, glm::vec2(5.f, 4.f), false);
        // level.objects.EmplaceUnit(level, Resources::LoadUnit("human/archer"),   f1, glm::vec2(4.f, 4.f), false);
        // ObjectID id = level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/berserker"),  f1, glm::vec2(5.f, 5.f), false);
        // level.objects.EmplaceUnit(level, Resources::LoadUnit("human/ranger"),   f1, glm::vec2(4.f, 5.f), false);
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
        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/stables"), f1, glm::vec2(21.f, 16.f), true);
        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("orc/ogre_mound"), f1, glm::vec2(24.f, 16.f), true);

        Techtree& t = level.factions.Player()->Tech();
        for(int i = ResearchType::PALA_UPGRADE; i < ResearchType::COUNT; i++) {
            t.IncrementResearch(i, false);
            t.IncrementResearch(i, true);
        }
        t.RecalculateBoth();

        ObjectID id_mg = level.objects.EmplaceUnit(level, Resources::LoadUnit("human/mage"), f1, glm::vec2(20.f, 27.f), false);
        ObjectID id_dk = level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/death_knight"), f1, glm::vec2(20.f, 23.f), false);
        ObjectID id_pl = level.objects.EmplaceUnit(level, Resources::LoadUnit("human/paladin"), f1, glm::vec2(20.f, 26.f), false);
        ObjectID id_om = level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/ogre_magi"), f1, glm::vec2(20.f, 24.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/death_knight"), f2, glm::vec2(20.f, 22.f), false);

        level.objects.EmplaceUnit(level, Resources::LoadUnit("misc/skeleton"), f1, glm::vec2(18.f, 24.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("misc/pig"), f1, glm::vec2(17.f, 24.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("misc/eye"), f1, glm::vec2(16.f, 24.f), false);

        level.objects.GetUnit(id_mg).Mana(255);
        level.objects.GetUnit(id_dk).Mana(255);
        level.objects.GetUnit(id_pl).Mana(255);
        level.objects.GetUnit(id_om).Mana(255);

        // level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/lumber_mill"), f1, glm::vec2(24.f, 22.f), true);
        // level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/gnomish_inventor"), f1, glm::vec2(21.f, 22.f), true);

        // level.objects.EmplaceBuilding(level, Resources::LoadBuilding("orc/blacksmith"), f1, glm::vec2(27.f, 25.f), true);
        // level.objects.EmplaceBuilding(level, Resources::LoadBuilding("orc/lumber_mill"), f1, glm::vec2(24.f, 25.f), true);
        // level.objects.EmplaceBuilding(level, Resources::LoadBuilding("orc/barracks"), f1, glm::vec2(21.f, 25.f), true);

        // level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/stables"), f1, glm::vec2(18.f, 22.f), true);
        // level.objects.EmplaceBuilding(level, Resources::LoadBuilding("orc/ogre_mound"), f1, glm::vec2(18.f, 25.f), true);

        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/watch_tower"), f1, glm::vec2(7.f, 5.f), true);

        // Unit& troll = level.objects.GetUnit(id);
        // level.objects.EmplaceUtilityObj(level, Resources::LoadUtilityObj("tst"), glm::ivec2(10, 10), ObjectID(5,0,9), troll);

        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"),   f1, glm::vec2(15.f, 15.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"),   f1, glm::vec2(16.f, 15.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"),   f1, glm::vec2(15.f, 16.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"),   f1, glm::vec2(16.f, 16.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"),   f1, glm::vec2(15.f, 17.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"),   f1, glm::vec2(16.f, 17.f), false);

        ObjectID un = level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"),   f2, glm::vec2(6.f, 26.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"),   f2, glm::vec2(9.f, 25.f), false);
        // level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/watch_tower"), f2, glm::vec2(6.f, 28.f), true);

        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/archer"),   f1, glm::vec2(4.f, 4.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/dragon"),   f1, glm::vec2(6.f, 4.f), false);

        // level.objects.GetUnit(un).IssueCommand(Command::Patrol(glm::ivec2(15, 26)));


        // ObjectID to_kill = level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/stables"), f1, glm::vec2(18.f, 22.f), true);
        // level.objects.EmplaceBuilding(level, Resources::LoadBuilding("orc/ogre_mound"), f2, glm::vec2(18.f, 25.f), false);

        // level.objects.GetBuilding(to_kill).Kill();

        level.objects.Add(Building(level, Resources::LoadBuilding("human/town_hall"), f1, glm::vec2(0.f, 0.f), true));
        // trollID = level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/peon"), f1, glm::vec2(5.f, 5.f), false);
        // level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/peon"), f2, glm::vec2(6.f, 5.f), false);
        // level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/peon"), f1, glm::vec2(5.f, 6.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/peasant"), f1, glm::vec2(5.f, 5.f), false);

        // level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/troll"), f1, glm::vec2(5.f, 4.f), false);
        // level.objects.EmplaceUnit(level, Resources::LoadUnit("human/archer"), f1, glm::vec2(5.f, 3.f), false);
    // level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"), f1, glm::vec2(6.f, 3.f), false);

        // level.objects.EmplaceUnit(level, Resources::LoadUnit("human/archer"), f1, glm::vec2(21.f, 23.f), false);

        // level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/tower_guard"), f2, glm::vec2(11.f, 5.f), true);



        // trollID = level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/troll"), dummy_faction, glm::vec2(0.f, 0.f));
        // trollID = level.objects.EmplaceUnit(level, Resources::LoadUnit("human/archer"), dummy_faction, glm::vec2(5.f, 5.f), false);
        // trollID = level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"), dummy_faction, glm::vec2(5.f, 5.f), false);
        // trollID = level.objects.EmplaceUnit(level, Resources::LoadUnit("human/peasant"), dummy_faction, glm::vec2(5.f, 5.f), false);
        // trollID = level.objects.EmplaceUnit(level, Resources::LoadUnit("human/tanker"), dummy_faction, glm::vec2(16.f, 3.f), false);

        ObjectID ol2 = level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/oil_platform"), f2, glm::vec2(20.f, 6.f), true);
        ObjectID shipyard = level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/shipyard"), f1, glm::vec2(14.f, 1.f), true);
        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/foundry"), f1, glm::vec2(14.f, 4.f), true);
        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("orc/oil_refinery"), f1, glm::vec2(14.f, 7.f), true);

        // ObjectID ol = level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/oil_platform"), f1, glm::vec2(22.f, 8.f), true);
        ObjectID ol = level.objects.EmplaceBuilding(level, Resources::LoadBuilding("misc/oil"), f_n, glm::vec2(22.f, 8.f), true);
        ObjectID gm = level.objects.EmplaceBuilding(level, Resources::LoadBuilding("misc/gold_mine"), f_n, glm::vec2(7.f, 7.f), true);
        level.objects.GetBuilding(ol).SetAmountLeft(123456);
        level.objects.GetBuilding(gm).SetAmountLeft(654321);
        level.objects.GetBuilding(ol2).SetAmountLeft(100);

        // level.objects.EmplaceUnit(level, Resources::LoadUnit("human/battleship"), f1, glm::vec2(18.f, 4.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/destroyer"), f1, glm::vec2(18.f, 4.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/submarine"), f1, glm::vec2(20.f, 4.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/transport"), f1, glm::vec2(18.f, 6.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/tanker"), f1, glm::vec2(22.f, 8.f), false);
        // level.objects.EmplaceUnit(level, Resources::LoadUnit("human/tanker"), f1, glm::vec2(22.f, 0.f), false);

        ObjectID sub = level.objects.EmplaceUnit(level, Resources::LoadUnit("human/submarine"), f2, glm::vec2(18.f, 2.f), false);
        ObjectID en  = level.objects.EmplaceUnit(level, Resources::LoadUnit("human/destroyer"), f2, glm::vec2(20.f, 2.f), false);
        // level.objects.GetUnit(en).SetInvisible(true);

        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/submarine"), f2, glm::vec2(18.f, 12.f), false);
        // level.objects.GetUnit(sub).IssueCommand(Command::Attack(shipyard, level.objects.GetBuilding(shipyard).Position()));


        glm::ivec3 d = level.objects.GetBuilding(gm).Data()->num_id;
        ENG_LOG_INFO("GM NUM_ID: ({}, {}, {})", d.x, d.y, d.z);

        // Unit& troll = level.objects.GetUnit(trollID);
        // troll.ChangeCarryStatus(WorkerCarryState::WOOD);

        colorPalette = ColorPalette(true);
        colorPalette.UpdateShaderValues(shader);

        Camera::Get().SetBounds(level.map.Size());
        Camera::Get().SetupZoomUpdates(true);
        Camera::Get().ZoomToFit(glm::vec2(12.f));
        Camera::Get().SetGUIBoundsOffset(0.5f);     //based on GUI width (1/4 of screen; 2 = entire screen)

    } catch(std::exception& e) {
        LOG_INFO("ERROR: {}", e.what());
        LOG_ERROR("Failed to load resources; Terminating...");
        throw e;
    }

    Camera::Get().Center(glm::vec2(16.f, 7.f));
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

    //======================
    
    Renderer::End();
}

void Sandbox::OnGUI() {
#ifdef ENGINE_ENABLE_GUI
    if(gui_enabled) {
        Config::DBG_GUI();
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
