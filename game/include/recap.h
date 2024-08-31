#pragma once

#include <engine/engine.h>

#include "stage.h"

#include <string>
#include <unordered_map>

struct ScenarioInfo {
    int campaignIdx = -1;
    bool isOrc;

    std::string obj_title;
    std::string obj_text;
    eng::TextureRef obj_background = nullptr;
    std::vector<std::string> obj_objectives;
    
    bool act = false;
    std::string act_text1;
    std::string act_text2;
    eng::TextureRef act_background = nullptr;

    bool cinematic = false;
    //...

};

struct ObjectivesScreenData {
    ScenarioInfo scenario;

    eng::GUI::ScrollText text;
};

struct FactionGUIElements {
    eng::GUI::TextLabel factionName;
    std::array<int, 7> values;
    std::array<eng::GUI::ValueBar, 7> stats;
public:
    FactionGUIElements() = default;
    FactionGUIElements(const std::string& name, const eng::EndgameStats& stats, const eng::GUI::StyleRef& bar_style, const eng::GUI::StyleRef& text_style, const glm::vec4& color, int pos);

    void Render();
};

struct RecapScreenData {
    static constexpr int STATS_COUNT = 7;

    eng::TextureRef background = nullptr;

    eng::GUI::TextLabel outcomeLabel;
    eng::GUI::TextLabel outcome;

    eng::GUI::TextLabel rankLabel;
    eng::GUI::TextLabel rank;

    eng::GUI::TextLabel scoreLabel;
    eng::GUI::TextLabel score;

    std::array<eng::GUI::TextLabel, STATS_COUNT> statsLabels;
    std::vector<FactionGUIElements> factions;
};

struct IngameInitParams;

namespace RecapState { enum { INVALID, ACT_INTRO, OBJECTIVES, GAME_RECAP, CREDITS }; }

//==========================================

class RecapController : public GameStageController, public eng::GUI::ButtonCallbackHandler {
public:
    RecapController();

    virtual void Update() override;
    virtual void Render() override;

    virtual int GetStageID() const override { return GameStageName::RECAP; }

    virtual void OnPreLoad(int prevStageID, int info, void* data) override;
    virtual void OnPreStart(int prevStageID, int info, void* data) override;
    virtual void OnStart(int prevStageID, int info, void* data) override;
    virtual void OnStop() override;

    DBGONLY(virtual void DBG_StageSwitch(int stateIdx) override);
private:
    //reset state variables for proper stage functioning
    void ActIntro_Reset(bool isOrc);

    bool LoadScenarioInfo(int campaignIdx, bool isOrc);
    void SetupRecapScreen(IngameInitParams* params);
    bool LoadRecapBackground(int campaignIdx, bool isOrc, bool game_won);

    void GUI_Init_ObjectivesScreen();
    void GUI_Init_RecapScreen();
    void GUI_Init_Other();
private:
    int state = RecapState::INVALID;

    bool interrupted = false;
    float t = 0.f;
    float timing = 0.f;
    int flag = 0;

    glm::vec4 textColor = glm::vec4(1.f, 0.f, 0.f, 1.f);

    eng::GUI::SelectionHandler selection = {};
    eng::GUI::TextButton continue_btn;

    RecapScreenData recap;
    ObjectivesScreenData objectives;

    GameInitParams* gameInitData = nullptr;
};


