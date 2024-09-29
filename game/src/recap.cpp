#include "recap.h"
#include "ingame.h"
#include "menu.h"

#include <nlohmann/json.hpp>

using namespace eng;

#define ACT_INTRO_FADEIN_TIME 2.f
#define ACT_INTRO_SOUND_DURATION 1.f

#define OBJ_SCROLL_SPEED 1.f
#define OBJ_AFTERSROLL_DELAY 2.f

constexpr float RECAP_HEIGHT = 1.f / 20.f;

//==============================================

RecapController::RecapController() {
    GUI_Init_ObjectivesScreen();
    GUI_Init_RecapScreen();
    GUI_Init_Other();
}

void RecapController::Update() {
    float currentTime = Input::CurrentTime();

    Input& input = Input::Get();

    switch(state) {
        case RecapState::INVALID:
            GetTransitionHandler()->InitTransition(TransitionParameters(TransitionDuration::MID, TransitionType::FADE_OUT, GameStageName::MAIN_MENU, MainMenuState::MAIN, true));
            break;
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
            static std::array<float,17> time_steps = {
                1.f, 
                1.f, 1.f, 1.f,
                1.f, 0.1f, 1.f, 0.1f, 1.f, 0.1f, 1.f, 0.1f, 1.f, 0.1f, 1.f, 0.1f, 1.f
            };

            if(flag < time_steps.size()) {
                //GUI elements 'slow reveal' animation
                interrupted |= (input.lmb.down() || input.rmb.down() || input.enter || input.space);
                t = (currentTime - timing) / time_steps.at(flag);

                if(t >= 1.f) {
                    RecapSubstage_Finalize(flag);

                    timing = Input::CurrentTime();
                    flag++;
                }
                else {
                    RecapSubstage_Interpolate(flag, t);
                }

                if(flag >= 2) {
                    selection.Update(&continue_btn);
                    continue_btn.OnHighlight();
                }

                //display all when interrupted
                if(interrupted) {
                    for(int i = 0; i < time_steps.size(); i++) {
                        RecapSubstage_Finalize(i);
                    }
                    flag = time_steps.size();
                    interrupted = false;
                }
            }
            else {
                //unveiling animation done
                selection.Update(&continue_btn);
                continue_btn.OnHighlight();

                //transition to game on key interrupt or when all the text is gone
                if(interrupted) {
                    RecapSubstage_TransitionOut();
                }
            }
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
            //TODO: change color back to white once there are actual background textures in use
            Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f), glm::vec2(1.f, 1.f), glm::vec4(0.f), recap.background));

            recap.outcomeLabel.Render();
            recap.outcome.Render();

            recap.rankLabel.Render();
            recap.rank.Render();

            recap.scoreLabel.Render();
            recap.score.Render();

            for(auto& label : recap.statsLabels) {
                label.Render();
            }

            for(auto& faction : recap.factions) {
                faction.Render();
            }

            continue_btn.Render();

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
                    //transition to main menu if next scenario is not found
                    GetTransitionHandler()->ForceBlackScreen(true);
                    state = RecapState::INVALID;
                    return;
                }
            }

            ActIntro_Reset(objectives.scenario.isOrc);

            //optional switch to cinematic/act intro substages (based on loaded config)
            if(prevStageID == GameStageName::MAIN_MENU) {
                if(objectives.scenario.cinematic) {
                    //TODO:
                }
                else if(objectives.scenario.act) {
                    state = RecapState::ACT_INTRO;
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
            timing = Input::CurrentTime();
            flag = 0;
            interrupted = false;
            break;
    }
}

void RecapController::OnStop(int nextStageID) {
    //state reset
    if(nextStageID != GameStageName::RECAP) {
        objectives.scenario.campaignIdx = -1;
    }

    GetTransitionHandler()->ForceBlackScreen(false);
}

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

    if(data.count("filepath")) {
        gameInitData->filepath = data.at("filepath");
    }
    else {
        snprintf(filepath, sizeof(filepath), "res/campaign/maps/%s_%02d.json", isOrc ? "oc" : "hu", campaignIdx);
        gameInitData->filepath = std::string(filepath);
    }

    if(data.count("cinematic")) {
        objectives.scenario.cinematic = true;
        //TODO: campaign cinematics
    }

    return true;
}

void RecapController::SetupRecapScreen(IngameInitParams* params) {
    //load the background texture
    LoadRecapBackground(params->params.campaignIdx, bool(params->params.race), params->game_won);
    gameInitData = &params->params;

    auto colors = ColorPalette::Colors();

    GUI::StyleRef text_style = recap.rankLabel.Style();
    GUI::StyleRef bar_style = recap.bar_style;

    recap.outcome.Setup(params->game_won ? "Victory!" : "Defeat!");
    recap.game_won = params->game_won;

    //hide all the GUI to allow the 'slow reveal' animation
    recap.outcome.Enable(false);
    recap.outcomeLabel.Enable(false);

    recap.rank.Enable(false);
    recap.rankLabel.Enable(false);

    recap.score.Enable(false);
    recap.scoreLabel.Enable(false);

    continue_btn.Enable(false);

    for(auto& label : recap.statsLabels) {
        label.Enable(false);
    }

    for(int i = 0; i < RecapScreenData::STATS_COUNT; i++) {
        recap.statsMax.at(i) = 0;
    }

    //setup GUI elements for each ingame faction
    int row = 1;
    recap.factions.clear();
    for(const auto& f : params->stats) {
        if(f.controllerID == FactionControllerID::NATURE)
            continue;

        glm::vec4 color = (f.colorIdx >= 0) ? colors.at(f.colorIdx) : colors.at(0);
        recap.factions.push_back(FactionGUIElements(f.name, f.stats, bar_style, text_style, color, row++));

        auto stats_arr = f.stats.ToArray();
        for(int i = 0; i < RecapScreenData::STATS_COUNT; i++) {
            if(recap.statsMax.at(i) < stats_arr.at(i)) {
                recap.statsMax.at(i) = stats_arr.at(i);
            }
        }
    }
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

//==================================================

void RecapController::GUI_Init_ObjectivesScreen() {
    GUI::StyleRef textStyle = std::make_shared<GUI::Style>();
    textStyle->font = Resources::DefaultFont();
    textStyle->textColor = glm::vec4(1.f);
    textStyle->color = glm::vec4(0.f);

    objectives.text = GUI::ScrollText(glm::vec2(0.31f, -0.3f), glm::vec2(0.48f, 0.37), 0.f, textStyle, "placeholder");
}

void RecapController::GUI_Init_RecapScreen() {
    GUI::StyleRef text_sml = std::make_shared<GUI::Style>();
    GUI::StyleRef text_mid = std::make_shared<GUI::Style>();
    GUI::StyleRef text_big = std::make_shared<GUI::Style>();
    text_sml->font = text_mid->font = text_big->font = Resources::DefaultFont();
    text_sml->color = text_mid->color = text_big->color = glm::vec4(0.f);
    text_sml->textColor = text_mid->textColor = text_big->textColor = glm::vec4(1.f);
    text_sml->textScale = 1.f;
    text_mid->textScale = 2.f;
    text_big->textScale = 4.f;

    recap.outcomeLabel  = GUI::TextLabel(glm::vec2(-2.f/3.f, -1.f + 1.50f*RECAP_HEIGHT), glm::vec2(1.f/3.f, 1.f*RECAP_HEIGHT), 1.f, text_sml, "Outcome");
    recap.outcome       = GUI::TextLabel(glm::vec2(-2.f/3.f, -1.f + 3.75f*RECAP_HEIGHT), glm::vec2(1.f/3.f, 2.f*RECAP_HEIGHT), 1.f, text_big, "Victory!");

    recap.rankLabel  = GUI::TextLabel(glm::vec2(0.f, -1.f + 1.50f*RECAP_HEIGHT), glm::vec2(1.f/3.f, 1.0f*RECAP_HEIGHT), 1.f, text_sml, "Rank");
    recap.rank       = GUI::TextLabel(glm::vec2(0.f, -1.f + 3.75f*RECAP_HEIGHT), glm::vec2(1.f/3.f, 1.5f*RECAP_HEIGHT), 1.f, text_mid, "Peasant");

    recap.scoreLabel  = GUI::TextLabel(glm::vec2(2.f/3.f, -1.f + 1.50f*RECAP_HEIGHT), glm::vec2(1.f/3.f, 1.0f*RECAP_HEIGHT), 1.f, text_sml, "Score");
    recap.score       = GUI::TextLabel(glm::vec2(2.f/3.f, -1.f + 3.75f*RECAP_HEIGHT), glm::vec2(1.f/3.f, 1.5f*RECAP_HEIGHT), 1.f, text_mid, "1234567");

    constexpr int sz = RecapScreenData::STATS_COUNT;
    std::array<const char*, RecapScreenData::STATS_COUNT> stat_names = { "Units", "Buildings", "Gold", "Lumber", "Oil", "Kills", "Razings" };
    for(int i = 0; i < recap.statsLabels.size(); i++) {
        recap.statsLabels.at(i) = GUI::TextLabel(glm::vec2((2.f*i)/sz - 1.f + 1.f / (sz+0.5f), -1.f + 7.5f*RECAP_HEIGHT), glm::vec2(1.f/(sz+0.5f), 1.5f*RECAP_HEIGHT), 1.f, text_sml, stat_names.at(i));
    }

    glm::vec2 buttonSize = glm::vec2(0.25f, 0.25f * GUI::ImageButtonWithBar::BarHeightRatio());
    glm::vec2 ts = glm::vec2(Window::Get().Size()) * buttonSize;
    float upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
    glm::vec2 textureSize = ts * upscaleFactor;
    int borderWidth = 7 * upscaleFactor;
    glm::vec2 border_size = 7.f / ts;

    recap.bar_style = std::make_shared<GUI::Style>();
    recap.bar_style->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear_2borders(textureSize.x, textureSize.y, borderWidth, borderWidth, glm::uvec3(120), glm::uvec3(20), glm::uvec3(120), glm::uvec3(120), glm::uvec3(240), borderWidth/2));
}

void RecapController::GUI_Init_Other() {
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
}

void RecapController::RecapSubstage_Interpolate(int idx, float t) {
    if(idx >= 4 && idx <= 16 && idx % 2 == 0) {
        idx = (idx-4)/2;
        recap.statsLabels.at(idx).Enable(true);
        for(auto& f : recap.factions)
            f.Interpolate(idx, t, recap.statsMax.at(idx));
    }
}

void RecapController::RecapSubstage_Finalize(int idx) {
    switch(idx) {
    case 1:
        recap.outcome.Enable(true);
        recap.outcomeLabel.Enable(true);
        break;
    case 2:
        recap.rank.Enable(true);
        recap.rankLabel.Enable(true);
        break;
    case 3:
        recap.score.Enable(true);
        recap.scoreLabel.Enable(true);
        continue_btn.Enable(true);
        break;
    case 4:
    case 6:
    case 8:
    case 10:
    case 12:
    case 14:
    case 16:
        recap.statsLabels.at((idx-4)/2).Enable(true);
        for(auto& f : recap.factions)
            f.Finalize((idx-4)/2, recap.statsMax.at((idx-4)/2));
        break;
    default:
        break;
    }

    if((idx >= 1 && idx <= 3) || (idx > 3 && idx % 2 == 0)) {
        Audio::Play(SoundEffect::GetPath("misc/thunk"));
    }
}

void RecapController::RecapSubstage_TransitionOut() {
    ASSERT_MSG(gameInitData != nullptr, "GameInitData have to be set here!");

    if(gameInitData->campaignIdx >= 0) {
        if(recap.game_won)
            gameInitData->campaignIdx++;

        //campaign - next scenario, transition to RecapScreen::Objectives
        GetTransitionHandler()->InitTransition(
            TransitionParameters(TransitionDuration::MID, TransitionType::FADE_OUT, GameStageName::RECAP, RecapState::OBJECTIVES, (void*)gameInitData, true)
        );
    }
    else {
        //custom game - transition to main menu
        GetTransitionHandler()->InitTransition(
            TransitionParameters(TransitionDuration::MID, TransitionType::FADE_OUT, GameStageName::MAIN_MENU, MainMenuState::MAIN, nullptr, true)
        );
    }
}

FactionGUIElements::FactionGUIElements(const std::string& name, const eng::EndgameStats& factionStats, const eng::GUI::StyleRef& bar_style, const eng::GUI::StyleRef& text_style, const glm::vec4& color, int pos) {
    values = factionStats.ToArray();

    constexpr int sz = RecapScreenData::STATS_COUNT;

    factionName = GUI::TextLabel(
        glm::vec2((2.f*0)/sz - 1.f + 1.f / (sz+0.5f), -1.f + (7.5f + pos*4.75f)*RECAP_HEIGHT), 
        glm::vec2(1.f/(sz+0.5f), 1.5f*RECAP_HEIGHT), 
        1.f, 
        text_style,
        name,
        false
    );
    factionName.Enable(false);

    glm::vec2 size = glm::vec2(1.f/(sz+0.5f), 1.f*RECAP_HEIGHT);
    glm::vec2 border_size = size * glm::vec2(0.15f, 2.2f);

    for(int i = 0; i < sz; i++) {
        stats.at(i) = GUI::ValueBar(
            glm::vec2((2.f*i)/sz - 1.f + 1.f / (sz+0.5f), -1.f + (10.f + pos*4.75f)*RECAP_HEIGHT), 
            size, 
            1.f, 
            bar_style, 
            border_size, 
            text_style, 
            color, 
            "0"
        );
        stats.at(i).Setup(0.f, "0", false);
        stats.at(i).Enable(false);
    }
}

void FactionGUIElements::Render() {
    factionName.Render();
    for(auto& stat : stats) {
        stat.Render();
    }
}

void FactionGUIElements::Interpolate(int idx, float t, int maxval) {
    char buf[128];
    snprintf(buf, sizeof(buf), "%d", int(t * values.at(idx)));

    if(maxval != 0)
        t = t * (float(values.at(idx)) / maxval);
    else
        t = 0.f;

    factionName.Enable(true);
    stats.at(idx).Setup(t, buf, true);
}

void FactionGUIElements::Finalize(int idx, int maxval) {
    char buf[128];
    snprintf(buf, sizeof(buf), "%d", values.at(idx));

    float t = (maxval != 0) ? (float(values.at(idx)) / maxval) : 0.f;

    factionName.Enable(true);
    stats.at(idx).Setup(t, buf, true);
}
