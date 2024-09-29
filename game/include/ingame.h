#pragma once

#include <engine/engine.h>

#include "stage.h"

namespace GameStartType { enum { INVALID, CAMPAIGN = 0, CUSTOM, LOAD }; }

struct IngameInitParams {
    GameInitParams params;
    std::vector<eng::EndgameFactionData> stats;
    bool game_won;
};

class IngameController : public GameStageController, public eng::PlayerFactionController::GUIRequestHandler {
public:
    IngameController();

    virtual void Update() override;
    virtual void Render() override;

    virtual int GetStageID() const override { return GameStageName::INGAME; }

    virtual void OnPreLoad(int prevStageID, int info, void* data) override;
    virtual void OnPreStart(int prevStageID, int info, void* data) override;
    virtual void OnStart(int prevStageID, int info, void* data) override;
    virtual void OnStop(int nextStageID) override;

    virtual void PauseRequest(bool pause) override;
    virtual void PauseToggleRequest() override;
    virtual void ChangeLevel(const std::string& filename) override;
    virtual void QuitMission() override;
    virtual void GameOver(bool game_won) override;

    virtual void DBG_GUI(bool active) override;
private:
    void KeyPressCallback(int keycode, int modifiers);

    void LevelSetup(int startType, GameInitParams* params);
    void StateReset();
private:
    eng::Level level;

    bool can_be_paused = true;
    bool paused = false;

    bool ready_to_render = false;
    bool ready_to_run = false;
};
