#include "ingame.h"
#include "menu.h"
#include "recap.h"

using namespace eng;

static std::string stage_names[] = { "CAMPAIGN", "CUSTOM", "LOAD" };

static IngameInitParams gameInitParams = {};

IngameController::IngameController() {}

void IngameController::Update() {
    if(!ready_to_run)
        return;
    
    if(!paused || !can_be_paused) {
        level.Update();
    }
    else {
        level.factions.Player()->Update_Paused(level);
    }
    level.factions.Player()->CameraPosition(Camera::Get().Position());
}

void IngameController::Render() {
    if(!ready_to_render)
        return;

    level.Render();
    level.factions.Player()->Render();
}

void IngameController::OnPreLoad(int prevStageID, int info, void* data) {
    //start loading game assets - probably just map data, since opengl can't handle async loading

    StateReset();

    //TODO: do this in a separate thread
    LevelSetup(info, static_cast<GameInitParams*>(data));

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
    Camera& camera = Camera::Get();

    camera.SetBounds(level.map.Size());
    camera.SetupZoomUpdates(true);           //TODO: should be disabled
    camera.ZoomToFit(glm::vec2(12.f));
    camera.SetGUIBoundsOffset(0.5f);     //based on GUI width (1/4 of screen; 2 = entire screen)

    camera.Center(level.factions.Player()->CameraPositionInit(level.MapSize()), false);
}

void IngameController::OnStart(int prevStageID, int info, void* data) {
    LOG_INFO("GameStage = InGame (init = {})", stage_names[info]);

    Input::Get().AddKeyCallback(-1, [](int keycode, int modifiers, bool single_press, void* handler){
        static_cast<IngameController*>(handler)->SignalKeyPress(keycode, modifiers, single_press);
    }, true, this);

    ready_to_render = true;
    ready_to_run = true;
}

void IngameController::OnStop(int nextStageID) {
    ready_to_render = false;
    ready_to_run = false;
}

void IngameController::PauseRequest(bool pause) {
    if(can_be_paused) {
        paused = pause;
    }
    Input::Get().SetPaused(paused);
}

void IngameController::PauseToggleRequest() {
    if(can_be_paused) {
        paused = !paused;
    }
    Input::Get().SetPaused(paused);
}

void IngameController::ChangeLevel(const std::string& filename) {
    gameInitParams.params = {};
    gameInitParams.params.filepath = filename;

    GetTransitionHandler()->InitTransition(
        TransitionParameters(TransitionDuration::SHORT, TransitionType::FADE_OUT, GameStageName::INGAME, GameStartType::LOAD, (void*)&gameInitParams.params, true, true)
    );
    ready_to_run = false;
}

void IngameController::QuitMission() {
    GetTransitionHandler()->InitTransition(
        TransitionParameters(TransitionDuration::MID, TransitionType::FADE_OUT, GameStageName::MAIN_MENU, MainMenuState::MAIN, nullptr, true)
    );
    ready_to_run = false;
}

void IngameController::GameOver(bool game_won) {
    gameInitParams.stats = level.factions.GetEndgameStats();
    gameInitParams.game_won = game_won;

    GetTransitionHandler()->InitTransition(
        TransitionParameters(TransitionDuration::SHORT, TransitionType::FADE_OUT, GameStageName::RECAP, RecapState::GAME_RECAP, (void*)&gameInitParams, true)
    );
    ready_to_run = false;
}

void IngameController::DBG_GUI(bool active) {
#ifdef ENGINE_ENABLE_GUI
    if(active && level.initialized) {
        level.objects.DBG_GUI();
        level.map.DBG_GUI();
        if(level.factions.IsInitialized())
            level.factions.DBG_GUI();

        level.info.DBG_GUI();
    }
#endif
}

void IngameController::LevelSetup(int startType, GameInitParams* params) {
    LOG_INFO("IngameController::LevelSetup: loading from '{}'", params->filepath);
    level.Release();
    if(Level::Load(params->filepath, level) != 0) {
        LOG_ERROR("IngameController::LevelSetup: failed to initialize the level.");
        // throw std::runtime_error("");
        GetTransitionHandler()->InitTransition(
            TransitionParameters(TransitionDuration::MID, TransitionType::FADE_OUT, GameStageName::MAIN_MENU, MainMenuState::MAIN, nullptr, true),
            true
        );
        return;
    }

    //factions initialization for custom games
    if(!level.factions.IsInitialized()) {
        if(params->campaignIdx >= 0 || startType != GameStartType::CUSTOM) {
            LOG_ERROR("Malformed savefile - campaign game (or savefile) has no faction data.");
            // throw std::runtime_error("Malformed savefile - campaign game (or savefile) has no faction data.");
            GetTransitionHandler()->InitTransition(
                TransitionParameters(TransitionDuration::MID, TransitionType::FADE_OUT, GameStageName::MAIN_MENU, MainMenuState::MAIN, nullptr, true),
                true
            );
            return;
        }
        level.CustomGame_InitFactions(params->race, params->opponents);
        level.CustomGame_InitEndConditions();
    }
    gameInitParams.params = *params;
    
    //restore campaign transition data for loaded savefiles
    if(startType == GameStartType::LOAD) {
        gameInitParams.params.campaignIdx = level.info.campaignIdx;
        gameInitParams.params.race = level.info.race;

        //note: no need to restore these when starting new game - should be baked in the savefile already
    }

    LinkController(level.factions.Player());
    ready_to_render = true;

    //manually trigger first GUI update - to have it finalized when fading in
    level.factions.Player()->Update(level);
}

void IngameController::StateReset() {
    can_be_paused = true;
    paused = false;
    Input::Get().SetPaused(paused);

    ready_to_render = false;
    ready_to_run = false;
}