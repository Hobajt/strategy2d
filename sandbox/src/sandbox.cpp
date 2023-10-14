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

void MockIngameStageController::RegisterKeyCallback() {
    Input::Get().AddKeyCallback(-1, [](int keycode, int modifiers, void* handler){
        static_cast<MockIngameStageController*>(handler)->SignalKeyPress(keycode, modifiers);
    }, true, this);
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
        tst.factions.push_back(FactionsFile::FactionEntry(FactionControllerID::INVALID,       0, "neutral", 5));
        tst.factions.push_back(FactionsFile::FactionEntry(FactionControllerID::LOCAL_PLAYER,  0, "faction_1", 0));
        tst.factions.push_back(FactionsFile::FactionEntry(FactionControllerID::INVALID,       0, "faction_2", 1));
        tst.diplomacy.push_back({1, 2, 1});
        level.factions = Factions(std::move(tst));

        FactionControllerRef f_n = level.factions[0];
        FactionControllerRef f1 = level.factions[1];
        FactionControllerRef f2 = level.factions[2];

        //TODO: link an actual ingame stage after the level init (sandbox has none tho, so using mock instead)
        ingameStage = {};
        ingameStage.LinkController(level.factions.Player());
        ingameStage.RegisterKeyCallback();

        //TODO: for custom games - override level.factions with new object (initialized based on starting locations & faction count)

        level.objects.Add(Building(level, Resources::LoadBuilding("human/town_hall"), f1, glm::vec2(0.f, 0.f), true));
        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/farm"), f1, glm::vec2(5.f, 0.f));
        trollID = level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/peon"), f1, glm::vec2(5.f, 5.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/peon"), f2, glm::vec2(6.f, 5.f), false);

        level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/troll"), f1, glm::vec2(5.f, 4.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/archer"), f1, glm::vec2(5.f, 3.f), false);
        level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"), f1, glm::vec2(6.f, 3.f), false);

        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("human/tower_guard"), f2, glm::vec2(11.f, 5.f), true);
        // trollID = level.objects.EmplaceUnit(level, Resources::LoadUnit("orc/troll"), dummy_faction, glm::vec2(0.f, 0.f));
        // trollID = level.objects.EmplaceUnit(level, Resources::LoadUnit("human/archer"), dummy_faction, glm::vec2(5.f, 5.f), false);
        // trollID = level.objects.EmplaceUnit(level, Resources::LoadUnit("human/footman"), dummy_faction, glm::vec2(5.f, 5.f), false);
        // trollID = level.objects.EmplaceUnit(level, Resources::LoadUnit("human/peasant"), dummy_faction, glm::vec2(5.f, 5.f), false);
        // trollID = level.objects.EmplaceUnit(level, Resources::LoadUnit("human/tanker"), dummy_faction, glm::vec2(16.f, 3.f), false);

        level.objects.EmplaceBuilding(level, Resources::LoadBuilding("misc/oil"), f_n, glm::vec2(16.f, 7.f), true);
        ObjectID gm = level.objects.EmplaceBuilding(level, Resources::LoadBuilding("misc/gold_mine"), f_n, glm::vec2(7.f, 7.f), true);
        level.objects.GetBuilding(gm).SetAmountLeft(123456);

        Unit& troll = level.objects.GetUnit(trollID);
        troll.ChangeCarryStatus(WorkerCarryState::WOOD);

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

    // Unit* troll = nullptr;
    // if(level.objects.GetUnit(trollID, troll)) {
    //     CommandDispatch(*troll);
    // }

    // if(input.lmb.down()) {
    //     glm::ivec2 coords = glm::ivec2(camera.GetMapCoords(input.mousePos_n * 2.f - 1.f) + 0.5f);
    //     ObjectID id = level.objects.GetObjectAt(coords);
    //     ENG_LOG_INFO("CLICK at {} - ID = {}", coords, id);
    // }
    
    level.Update();

    level.Render();
    level.factions.Player()->Render();

    //======================
    
    Renderer::End();
}

void Sandbox::CommandDispatch(Unit& unit) {
    Camera& camera = Camera::Get();
    Input& input = Input::Get();

    //only fire commands on RMB clicks
    if(!input.rmb.down())
        return;
    
    //click info
    glm::ivec2 target_pos = glm::ivec2(camera.GetMapCoords(input.mousePos_n * 2.f - 1.f) + 0.5f);
    ObjectID target_id = level.map.ObjectIDAt(target_pos);
    TileData tile_info = level.map(target_pos);

    if(adaptiveCommand) {
        bool harvestable = ObjectID::IsHarvestable(target_id);
        bool gatherable = (ObjectID::IsObject(target_id) && level.objects.GetObject(target_id).IsGatherable(unit.NavigationType()));

        //resolve what type of command to issue based on unit & click info
        if(unit.IsWorker() && gatherable) {
            //issue gather command
            ENG_LOG_INFO("GATHER FROM {} ({})", target_id, level.objects.GetObject(target_id));
            unit.IssueCommand(Command::Gather(target_id));

            if(unit.Sound_Yes().valid) {
                Audio::Play(unit.Sound_Yes().Random());
            }
        }
        else if(unit.IsWorker() && harvestable) {
            //issue harvest command
            ENG_LOG_INFO("HARVEST WOOD AT {}", target_pos);
            unit.IssueCommand(Command::Harvest(target_pos));

            if(unit.Sound_Yes().valid) {
                Audio::Play(unit.Sound_Yes().Random());
            }
        }
        else if(ObjectID::IsAttackable(target_id)) {
            if(ObjectID::IsObject(target_id))
                ENG_LOG_INFO("ATTACK {} at {} ({})", target_id, target_pos, level.objects.GetObject(target_id));
            else
                ENG_LOG_INFO("ATTACK {} at {} (map object)", target_id, target_pos);
            if(unit.Sound_Yes().valid) {
                Audio::Play(unit.Sound_Yes().Random());
            }
            unit.IssueCommand(Command::Attack(target_id, target_pos));
        }
        else if(level.map.IsWithinBounds(target_pos)) {
            ENG_LOG_INFO("MOVE TO {}", target_pos);
            unit.IssueCommand(Command::Move(target_pos));
            //TODO: play units voices from wherever player commands are generated (not in the commands themselves)
            if(unit.Sound_Yes().valid) {
                Audio::Play(unit.Sound_Yes().Random());
            }
        }
        else {
            ENG_LOG_WARN("COULDN'T RESOLVE COMMAND");
        }
    }
    else {
        //force issue specific command
        switch(commandID) {
            case CommandType::MOVE:
                ENG_LOG_INFO("MOVE TO {}", target_pos);
                if(level.map.IsWithinBounds(target_pos)) {
                    unit.IssueCommand(Command::Move(target_pos));
                }
                break;
            case CommandType::ATTACK:
                if(ObjectID::IsAttackable(target_id)) {
                    ENG_LOG_INFO("ATTACK {} at {} ({})", target_id, target_pos, level.objects.GetObject(target_id));
                    unit.IssueCommand(Command::Attack(target_id, target_pos));
                }
                else {
                    ENG_LOG_INFO("ATTACK {} at {} (invalid target)", target_id, target_pos);
                }
                break;
            case CommandType::HARVEST_WOOD:
                ENG_LOG_INFO("HARVEST WOOD AT {}", target_pos);
                if(level.map.IsWithinBounds(target_pos)) {
                    unit.IssueCommand(Command::Harvest(target_pos));
                }
                break;
            case CommandType::GATHER_RESOURCES:
                ENG_LOG_INFO("GATHER FROM {}", target_id);
                if(ObjectID::IsValid(target_id)) {
                    unit.IssueCommand(Command::Gather(target_id));
                }
                break;
            case CommandType::RETURN_GOODS:
                ENG_LOG_INFO("RETURN GOODS ISSUED");
                unit.IssueCommand(Command::ReturnGoods());
                break;
            case CommandType::BUILD:
                ENG_LOG_INFO("BUILD ISSUED (ID={}, pos= ({}, {}))", buildingID, target_pos.x, target_pos.y);
                if(level.map.IsWithinBounds(target_pos)) {
                    //should probably do other sanity checks here - placement check, conditions check, worker check, etc. etc.
                    unit.IssueCommand(Command::Build(buildingID, target_pos));
                }
                break;
            default:
                ENG_LOG_INFO("BEHAVIOUR FOR COMMAND {} NOT IMPLEMENTED.", commandID);
                break;
        }
    }
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

        ImGui::Begin("Command");
        ImGui::Checkbox("Adaptive command", &adaptiveCommand);
        if(!adaptiveCommand)
            ImGui::Combo("type", &commandID, "Idle\0Move\0Attack\0Harvest\0Gather\0ReturnGoods\0Build");
        if(commandID == CommandType::BUILD) {
            ImGui::SliderInt("building ID", &buildingID, 0, BuildingType::COUNT);
        }
        ImGui::End();

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
