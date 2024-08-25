#include "recap.h"
#include "ingame.h"
#include "menu.h"

#include <nlohmann/json.hpp>


using namespace eng;

#define ACT_INTRO_FADEIN_TIME 2.f
#define ACT_INTRO_SOUND_DURATION 1.f

#define OBJ_SCROLL_SPEED 1.f
#define OBJ_AFTERSROLL_DELAY 2.f

RecapController::RecapController() {
    glm::vec2 buttonSize = glm::vec2(0.16f, 0.05f);
    glm::vec2 ts = glm::vec2(Window::Get().Size()) * buttonSize;
    float upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
    glm::ivec2 textureSize = ts * upscaleFactor;
    int borderWidth = 2 * upscaleFactor;

    GUI::StyleRef btnStyle = std::make_shared<GUI::Style>();
    btnStyle->textColor = glm::vec4(1.f);
    btnStyle->font = Resources::DefaultFont();
    btnStyle->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, false));
    btnStyle->hoverTexture = btnStyle->texture;
    btnStyle->holdTexture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, true));
    btnStyle->highlightTexture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonHighlightTexture(textureSize.x, textureSize.y, borderWidth));
    btnStyle->holdOffset = glm::ivec2(borderWidth);

    continue_btn = GUI::TextButton(glm::vec2(0.58f, 0.92f), buttonSize, 0.f, btnStyle, "Continue", 
        this, [](GUI::ButtonCallbackHandler* handler, int id) {
            static_cast<RecapController*>(handler)->interrupted = true;
        }
    );

    GUI::StyleRef textStyle = std::make_shared<GUI::Style>();
    textStyle->font = Resources::DefaultFont();
    textStyle->textColor = glm::vec4(1.f);
    textStyle->color = glm::vec4(0.f);

    objectives.text = GUI::ScrollText(glm::vec2(0.31f, -0.3f), glm::vec2(0.48f, 0.37), 0.f, textStyle, "placeholder");
}

void RecapController::Update() {
    float currentTime = Input::CurrentTime();

    Input& input = Input::Get();

    switch(state) {
        case RecapState::ACT_INTRO:
        {
            interrupted |= (input.lmb.down() || input.rmb.down() || input.enter || input.space);
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
        {
            //scroll until all the text is gone
            if(objectives.text.ScrollUpdate(input.deltaTime_real * OBJ_SCROLL_SPEED) && flag == 0) {
                timing = currentTime;
                flag = 1;
                LOG_TRACE("RecapState::OBJECTIVES - scroll over");
            }

            bool conditionMet = (flag == 1 && currentTime >= timing + OBJ_AFTERSROLL_DELAY);
            interrupted |= (input.enter || input.space);

            selection.Update(&continue_btn);
            continue_btn.OnHighlight();

            //transition to game on key interrupt or when all the text is gone
            if(interrupted || conditionMet) {
                GetTransitionHandler()->InitTransition(
                    TransitionParameters(TransitionDuration::MID, TransitionType::FADE_OUT, GameStageName::INGAME, 0, (void*)gameInitData, true)
                );
            }
            break;
        }
        case RecapState::GAME_RECAP:
        {
            //TODO:
            //animate the stats unveiling
            //      have an interpolation var that will track the revealing of one stat
            //      once revealed, move to another one
            //      reveal one stat at a time (for all factions at the same time)

            //detect button presses to skip the unveiling

            //begin transition once the 'Continue' button is pressed
            //      transition to main menu if game mode is custom game
            //      transition to Recap-Objectives if game mode is campaign - check where the campaign increment is done (possibly do it here if it's nowhere to be found)
            break;
        }
    }

    interrupted = false;
}

void RecapController::Render() {
    FontRef font = Resources::DefaultFont();

    switch(state) {
        case RecapState::ACT_INTRO:
        {
            float fh = (5.f * font->GetRowHeight()) / Window::Get().Height();
            Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f), glm::vec2(1.f, 1.f), glm::vec4(1.f, 1.f, 1.f, t), objectives.scenario.act_background));
            font->RenderTextCentered(objectives.scenario.act_text1.c_str(), glm::vec2(0.f, 0.1f), 4.f, clr_interpolate(glm::vec4(1.f), textColor, t));
            font->RenderTextCentered(objectives.scenario.act_text2.c_str(), glm::vec2(0.f, 0.1f-fh), 5.f, clr_interpolate(glm::vec4(1.f), textColor, t));
            break;
        }
        case RecapState::OBJECTIVES:
        {
            Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f), glm::vec2(1.f, 1.f), glm::vec4(1.f), objectives.scenario.obj_background));
            font->RenderTextCentered(objectives.scenario.obj_title.c_str(), glm::vec2(0.33f, 0.86f), 1.5f, glm::vec4(1.f));

            //objectives window
            float fh = (1.f * font->GetRowHeight()) / Window::Get().Height();
            Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.56f, -0.53f, -1e-3f), glm::vec2(0.4f, 0.3f), glm::vec4(glm::vec3(0.f), 0.5f)));
            glm::vec2 pos = glm::vec2(0.17f, -0.23f - fh);
            font->RenderText("Objectives:", pos, 1.f);
            for(int i = 0; i < (int)objectives.scenario.obj_objectives.size(); i++) {
                font->RenderText(objectives.scenario.obj_objectives[i].c_str(), glm::vec2(pos.x, pos.y - fh*1.1f - i*fh), 1.f);
            }
            continue_btn.Render();
            objectives.text.Render();
            break;
        }
        case RecapState::GAME_RECAP:
        {
            Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f), glm::vec2(1.f, 1.f), glm::vec4(0.f), recap.background));

            //TODO:

            //render background

            //render fully visible stat columns
            //render interpolated stat column
            //render the continue button once unveiling is done
            //render the texts

            break;
        }
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

void RecapController::OnPreLoad(int prevStageID, int info, void* data) {
    int state = info;
    switch(state) {
        case RecapState::GAME_RECAP:
        {
            IngameInitParams* params = nullptr;
            try {
                params = static_cast<IngameInitParams*>(data);
            } catch(std::exception&) {
                params = nullptr;
            }

            //invalid data - transition to main menu
            if(params == nullptr) {
                LOG_ERROR("RecapState::Recap - invalid data recieved, switching to main menu");
                GetTransitionHandler()->InitTransition(
                    TransitionParameters(TransitionDuration::MID, TransitionType::FADE_OUT, GameStageName::MAIN_MENU, MainMenuState::MAIN, nullptr, true),
                    true
                );
                return;
            }
            break;
        }
    }
}

void RecapController::OnPreStart(int prevStageID, int info, void* data) {
    //substage identification
    state = info;
    switch(info) {
        case RecapState::CREDITS:
            break;
        case RecapState::ACT_INTRO:
            //can only be triggered if cinematic was played (data is already loaded)
            ActIntro_Reset(objectives.scenario.isOrc);
            break;
        case RecapState::OBJECTIVES:
        {
            //async await for the data issued from OnPreLoad
            gameInitData = static_cast<GameInitParams*>(data);
            int cIdx = gameInitData->campaignIdx;
            bool isOrc = gameInitData->race == GameParams::Race::ORC;

            //load scenario info
            if(cIdx != objectives.scenario.campaignIdx || isOrc != objectives.scenario.isOrc) {
                if(!LoadScenarioInfo(cIdx, isOrc)) {
                    LOG_WARN("Info file not found for scenario '{}_{}'.", isOrc ? "oc" : "hu", cIdx);
                }
            }

            //optional switch to cinematic/act intro substages (based on loaded config)
            if(prevStageID == GameStageName::MAIN_MENU) {
                if(objectives.scenario.cinematic) {
                    //TODO:
                }
                else if(objectives.scenario.act) {
                    state = RecapState::ACT_INTRO;
                    ActIntro_Reset(objectives.scenario.isOrc);
                }
            }
            break;
        }
        case RecapState::GAME_RECAP:
        {
            SetupRecapScreen(static_cast<IngameInitParams*>(data));
            break;
        }
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
        case RecapState::OBJECTIVES:
            timing = Input::CurrentTime();
            flag = 0;
            break;
        case RecapState::GAME_RECAP:
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
    objectives.text.UpdateText(objectives.scenario.obj_text);
    objectives.text.SetPositionPreset(GUI::ScrollTextPosition::FIRST_LINE_GONE);
}

bool RecapController::LoadScenarioInfo(int campaignIdx, bool isOrc) {
    using json = nlohmann::json;

    //data already loaded
    if(objectives.scenario.campaignIdx == campaignIdx && objectives.scenario.isOrc == isOrc) {
        return true;
    }

    objectives.scenario.campaignIdx = campaignIdx;
    objectives.scenario.isOrc = isOrc;

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
        objectives.scenario.obj_text = obj.at("text");
        objectives.scenario.obj_title = obj.at("title");
        objectives.scenario.obj_background = Resources::LoadTexture(obj.at("background"), true);
        objectives.scenario.obj_objectives.clear();
        for(auto& o : obj.at("objectives")) {
            objectives.scenario.obj_objectives.push_back(o);
        }
    }
    else {
        //use default
        objectives.scenario.obj_text = "";
        objectives.scenario.obj_background = nullptr;
    }

    if(data.count("act")) {
        objectives.scenario.act = true;
        auto& act = data.at("act");
        objectives.scenario.act_text1 = act.at("text1");
        objectives.scenario.act_text2 = act.at("text2");
        objectives.scenario.act_background = Resources::LoadTexture(act.at("background"), true);
    }

    if(data.count("cinematic")) {
        objectives.scenario.cinematic = true;
        //TODO: campaign cinematics
    }

    return true;
}

void RecapController::SetupRecapScreen(IngameInitParams* params) {
    LoadRecapBackground(params->params.campaignIdx, bool(params->params.race), params->game_won);

    
}

bool RecapController::LoadRecapBackground(int campaignIdx, bool isOrc, bool game_won) {
    char filepath[512];
    snprintf(filepath, sizeof(filepath), "res/textures/backgrounds/end_%s_%s.json", isOrc ? "oc" : "hu", game_won ? "win" : "loss");

    try {
        recap.background = Resources::LoadTexture(filepath, true);
    }
    catch(std::exception&) {
        recap.background = nullptr;
    }
    return recap.background != nullptr;
}