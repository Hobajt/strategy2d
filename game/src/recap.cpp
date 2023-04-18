#include "recap.h"

#include "resources.h"

#include <nlohmann/json.hpp>


using namespace eng;

#define ACT_INTRO_FADEIN_TIME 2.f
#define ACT_INTRO_SOUND_DURATION 1.f

RecapController::RecapController() {

    //init objectives menu

}

void RecapController::Update() {
    float currentTime = Input::CurrentTime();

    Input& input = Input::Get();
    interrupted = (input.lmb.down() || input.rmb.down() || input.enter || input.space);

    switch(state) {
        case RecapState::ACT_INTRO:
        {
            bool conditionMet = false;

            switch(flag) {
                case 0:     //text fade-in (waiting for stage transition handler, controlled from OnStart() call)
                    t = 0.f;
                    break;
                case 1:     //background texture fade-in (fixed timer)
                    t = std::min((currentTime - timing) / ACT_INTRO_FADEIN_TIME, 1.f);
                    //start playing sound when faded in
                    if(t >= 1.f) {
                        //TODO: replace the sound
                        Audio::Play("res/sounds/test_sound.mp3");
                        timing = currentTime + ACT_INTRO_SOUND_DURATION;
                        flag = 2;
                    }
                    break;
                case 2:     //sound playback & delay
                    t = 1.f;
                    conditionMet = currentTime >= timing;
                    break;
            }

            //transition to objectives substage
            if(interrupted || conditionMet) {
                GetTransitionHandler()->InitTransition(
                    TransitionParameters(TransitionDuration::MID, TransitionType::FADE_OUT, GameStageName::RECAP, RecapState::OBJECTIVES, (void*)gameInitData, true)
                );
            }
            break;
        }
        case RecapState::OBJECTIVES:
            //TODO: text rolling updates + stop condition

            //do objectiveMenu GUI update
            //either copy the selection logic from MainMenuCtrl or move it to an independent class
            //-class is probably better

            bool conditionMet = false;
            if(interrupted || conditionMet) {
                GetTransitionHandler()->InitTransition(
                    TransitionParameters(TransitionDuration::MID, TransitionType::FADE_OUT, GameStageName::INGAME, 0, (void*)gameInitData, true)
                );
            }
            break;
    }
}

void RecapController::Render() {
    FontRef font = Resources::DefaultFont();

    switch(state) {
        case RecapState::ACT_INTRO:
        {
            Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f), glm::vec2(1.f, 1.f), glm::vec4(1.f, 1.f, 1.f, t), scenario.act_background));
            font->RenderTextCentered("Act I", glm::vec2(0.f), 10.f, clr_interpolate(glm::vec4(1.f), textColor, t));
            break;
        }
        case RecapState::OBJECTIVES:
            Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f), glm::vec2(1.f, 1.f), glm::vec4(1.f), scenario.obj_background));
            Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.56f, -0.53f, -1e-3f), glm::vec2(0.4f, 0.3f), glm::vec4(glm::vec3(0.f), 0.5f)));
            //TODO: render rolling text
            break;
    }
    /*
    render() depends on state:
        - objectives    = background texture + rolling text
        - credits       = same as objectives
        - recap         = background texture + stats + slowly revealing bars
        - act intro     = background texture + fading transition + text over the fading
        - interlude     = video playing
    */
}

void RecapController::OnPreStart(int prevStageID, int info, void* data) {
    //substage identification
    state = info;
    switch(info) {
        case RecapState::CREDITS:
            break;
        case RecapState::ACT_INTRO:
            //can only be triggered if cinematic was played (data is already loaded)
            ActIntro_Reset(scenario.isOrc);
            break;
        case RecapState::OBJECTIVES:
        {
            //async await for the data issued from OnPreLoad
            gameInitData = static_cast<GameInitParams*>(data);
            int cIdx = gameInitData->campaignIdx;
            bool isOrc = gameInitData->race == GameParams::Race::ORC;

            //load scenario info
            if(cIdx != scenario.campaignIdx || isOrc != scenario.isOrc) {
                if(!LoadScenarioInfo(cIdx, isOrc)) {
                    LOG_WARN("Info file not found for scenario '{}_{}'.", isOrc ? "oc" : "hu", cIdx);
                }
            }

            //optional switch to cinematic/act intro substages (based on loaded config)
            if(prevStageID == GameStageName::MAIN_MENU) {
                if(scenario.cinematic) {
                    //TODO:
                }
                else if(scenario.act) {
                    state = RecapState::ACT_INTRO;
                    ActIntro_Reset(scenario.isOrc);
                }
            }
        }
            break;
        case RecapState::GAME_RECAP:
            break;
    }

    /*what stages can transition to recapStage:
        - main menu - when starting campaign
        - main menu - credits btn clicked
        - in game - game over (both win/lose), display recap & stats
        - in game - restart btn clicked (no act info, only objectives)
    */
}

void RecapController::OnStart(int prevStageID, int info, void* data) {
    LOG_INFO("GameStage = Recap");

    switch(state) {
        case RecapState::ACT_INTRO:
            timing = Input::CurrentTime();
            flag = 1;
            break;
    }
}

void RecapController::OnStop() {}

#ifdef ENGINE_DEBUG
void RecapController::DBG_StageSwitch(int stateIdx) {

    //normally passing pointer to object in MenuController
    static GameInitParams params = {};
    params.campaignIdx = 0;
    params.race = GameParams::Race::ORC;
    OnPreStart(GameStageName::MAIN_MENU, RecapState::OBJECTIVES, (void*)&params);

    //need to setup new transition
    GetTransitionHandler()->InitTransition(
        TransitionParameters(TransitionDuration::MID, TransitionType::FADE_IN, GameStageName::RECAP, RecapState::OBJECTIVES, (void*)&params, false)
    );
}
#endif

void RecapController::ActIntro_Reset(bool isOrc) {
    flag = 0;
    t = 0.f;
    textColor = isOrc ? glm::vec4(1.f, 0.f, 0.f, 1.f) : glm::vec4(0.f, 0.f, 1.f, 1.f);
}

bool RecapController::LoadScenarioInfo(int campaignIdx, bool isOrc) {
    using json = nlohmann::json;

    //data already loaded
    if(scenario.campaignIdx == campaignIdx && scenario.isOrc == isOrc) {
        return true;
    }

    scenario.campaignIdx = campaignIdx;
    scenario.isOrc = isOrc;

    //load & parse the scenario file
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "res/campaign/%s_%02d.json", isOrc ? "oc" : "hu", campaignIdx);

    std::string data_str;
    if(!TryReadFile(filepath, data_str)) {
        return false;
    }
    json data = json::parse(data_str);
    
    if(data.count("obj")) {
        auto& obj = data.at("obj");
        scenario.obj_text = obj.at("text");
        scenario.obj_background = Resources::LoadTexture(obj.at("background"), true);
    }
    else {
        //use default
        scenario.obj_text = "";
        scenario.obj_background = nullptr;
    }

    if(data.count("act")) {
        scenario.act = true;
        auto& act = data.at("act");
        scenario.act_text = act.at("text");
        scenario.act_background = Resources::LoadTexture(act.at("background"), true);
    }

    if(data.count("cinematic")) {
        scenario.cinematic = true;
        //TODO: campaign cinematics
    }

    return true;
}
