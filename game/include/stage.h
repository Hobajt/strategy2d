#pragma once

#include <engine/engine.h>

#include <memory>
#include <map>

namespace GameStageName { enum { INVALID, INTRO, MAIN_MENU, INGAME }; }

//Base class for controllers that manage different stages of the game.
class GameStageController {
public:
    //For inner logic updates.
    virtual void Update() = 0;

    //For any kind of rendering. Renderer session is already initialized - don't call Renderer::Begin().
    virtual void Render() = 0;

    //Value identifying this specific controller type (should be the same for multiple instances of the same controller).
    virtual int GetStageID() const = 0;

    //Triggered when switching into this state.
    virtual void OnStart(int prevStageID) {}

    //Triggered when switching from this state.
    virtual void OnStop() {}
};
using GameStageControllerRef = std::shared_ptr<GameStageController>;

//===== GameStageTransitionHandler =====

//Manages rendering of game stage switching transitions as well as the logic behind it.
class GameStageTransitionHandler {
public:
    //Transition update & render.
    bool Update();

    int NextStageID() const { return nextStateID; }
private:
    int nextStateID;

    float startTime;
    float endTime;
    bool active = false;
};

//===== GameStage =====

class GameStage {
public:
    void Initialize(const std::vector<GameStageControllerRef>& stages, int initialStageIdx = 0);

    void Update();
    void Render();
private:
    int currentStageID = GameStageName::INVALID;
    GameStageControllerRef currentStage = nullptr;

    std::map<int, GameStageControllerRef> stages;

    GameStageTransitionHandler transition;
};
