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

namespace RecapState { enum { INVALID, ACT_INTRO, OBJECTIVES, GAME_RECAP, CREDITS }; }

//==========================================

class RecapController : public GameStageController, public eng::GUI::ButtonCallbackHandler {
public:
    RecapController();

    virtual void Update() override;
    virtual void Render() override;

    virtual int GetStageID() const override { return GameStageName::RECAP; }

    virtual void OnPreStart(int prevStageID, int info, void* data) override;
    virtual void OnStart(int prevStageID, int info, void* data) override;
    virtual void OnStop() override;

    DBGONLY(virtual void DBG_StageSwitch(int stateIdx) override);
private:
    //reset state variables for proper stage functioning
    void ActIntro_Reset(bool isOrc);

    bool LoadScenarioInfo(int campaignIdx, bool isOrc);
private:
    int state = RecapState::INVALID;

    bool interrupted = false;
    float t = 0.f;
    float timing = 0.f;
    int flag = 0;

    glm::vec4 textColor = glm::vec4(1.f, 0.f, 0.f, 1.f);

    eng::GUI::TextButton btn;
    eng::GUI::SelectionHandler selection = {};

    ScenarioInfo scenario;
    GameInitParams* gameInitData = nullptr;
};


