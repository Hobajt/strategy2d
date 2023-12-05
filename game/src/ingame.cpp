#include "ingame.h"

using namespace eng;

static std::string stage_names[] = { "CAMPAIGN", "CUSTOM", "LOAD" };

IngameController::IngameController() {}

void IngameController::Update() {
    level.Update();
}

void IngameController::Render() {
    level.Render();
    level.factions.Player()->Render();
}

void IngameController::OnPreLoad(int prevStageID, int info, void* data) {
    //start loading game assets - probably just map data, since opengl can't handle async loading

    //TODO: do this in a separate thread
    GameInitParams* params = static_cast<GameInitParams*>(data);
    LOG_INFO("IngameController::OnPreLoad: loading from '{}'", params->filepath);
    if(Level::Load(params->filepath, level) != 0) {
        LOG_ERROR("IngameController::OnPreLoad: failed to initialize the level.");
        //TODO: could maybe redirect back to main menu instead
        throw std::runtime_error("");
    }

    //factions initialization for custom games
    if(!level.factions.IsInitialized()) {
        if(params->campaignIdx >= 0) {
            LOG_ERROR("Malformed savefile - campaign game has no faction data.");
            throw std::runtime_error("Malformed savefile - campaign game has no faction data.");
        }
        level.CustomGame_InitFactions(params->race, params->opponents);
    }

    LinkController(level.factions.Player());

    

    /* ways to reach ingame stage:
        - mainMenu - start campaign (goes through recap stage tho)
        - mainMenu - start custom
        - mainMenu - load
        - ingame - restart
        - ingame - load
        - ingame menu? (if it's gonna be a separate stage, probably not tho)
    */
}

void IngameController::OnPreStart(int prevStageID, int info, void* data) {
    Camera::Get().SetBounds(level.map.Size());
    Camera::Get().SetupZoomUpdates(true);           //TODO: should be disabled
    Camera::Get().ZoomToFit(glm::vec2(12.f));
    Camera::Get().SetGUIBoundsOffset(0.5f);     //based on GUI width (1/4 of screen; 2 = entire screen)
}

void IngameController::OnStart(int prevStageID, int info, void* data) {
    LOG_INFO("GameStage = InGame (init = {})", stage_names[info]);

    Input::Get().AddKeyCallback(-1, [](int keycode, int modifiers, bool single_press, void* handler){
        static_cast<IngameController*>(handler)->SignalKeyPress(keycode, modifiers, single_press);
    }, true, this);
}

void IngameController::OnStop() {

}

void IngameController::DBG_GUI(bool active) {
#ifdef ENGINE_ENABLE_GUI
    if(active && level.initialized) {
        level.objects.DBG_GUI();
        level.map.DBG_GUI();
        if(level.factions.IsInitialized())
            level.factions.DBG_GUI();
    }
#endif
}
