#pragma once

#include <engine/engine.h>

#include <memory>
#include <map>

//Identifies individual game stages.
namespace GameStageName { enum { INVALID, INTRO, MAIN_MENU, RECAP, INGAME }; }

namespace TransitionType { enum { FADE_OUT, FADE_IN }; }

namespace TransitionDuration {
    constexpr inline float SHORT = 0.25f;
    constexpr inline float MID = 0.5f;
    constexpr inline float LONG = 1.f;
}

//===== TransitionParameters =====

struct TransitionParameters {
    float duration;
    int type = TransitionType::FADE_OUT;

    int nextStage = GameStageName::INVALID;
    int info = -1;
    void* data = nullptr;

    bool autoFadeIn = false;
public:
    TransitionParameters() = default;
    TransitionParameters(float duration, int transitionType, int nextStageID, int info, bool autoFadeIn);
    TransitionParameters(float duration, int transitionType, int nextStageID, int info, void* data, bool autoFadeIn);

    bool EqualsTo(const TransitionParameters& other, float durationOverride = -1.f);

    //Clones this params object and replaces duration with provided value.
    TransitionParameters WithDuration(float d);
};

//===== TransitionHandler =====

//Manages transition logic and rendering when switching between game stages.
class TransitionHandler {
public:
    //Transition update & render.
    bool Update();

    int NextStageID() const { return params.nextStage; }
    int Info() const { return params.info; }
    void* Data() const { return params.data; }

    bool TransitionInProgress() const { return active; }
    bool IsFadedOut() const { return fadedOut; }

    //Initializes fade in transition if previous transition had autoFadeIn = true. Reuses all the values except for transition type.
    void AutoFadeIn();

    //Initializes new transition. Returns false on failure (other transition already in progress).
    bool InitTransition(const TransitionParameters& params, bool forceOverride = false);

    bool CompareTransitions(const TransitionParameters& other, float durationOverride = -1.f) { return params.EqualsTo(other, durationOverride); }

    void ForceFadeOut() { fadedOut = true; }
private:
    float startTime;
    float endTime;

    bool active = false;
    bool fadedOut = false;
    TransitionParameters params;

    bool fadingOut;
};

//===== GameStageController =====

//Base class for controllers that manage different stages of the game.
class GameStageController {
public:
    //For inner logic updates.
    virtual void Update() = 0;

    //For any kind of rendering. Renderer session is already initialized - don't call Renderer::Begin().
    virtual void Render() = 0;

    //Value identifying this specific controller type (should be the same for multiple instances of the same controller).
    virtual int GetStageID() const = 0;

    //Triggered when switching into this state - when screen fades out.
    //Use to initialize visuals, so that there's stuff on screen when fading in.
    virtual void OnPreStart(int prevStageID, int info, void* data) {}

    //Triggered when switching into this state - when screen fades in.
    //Use to initialize controls & game logic.
    virtual void OnStart(int prevStageID, int info, void* data) {}

    //Triggered when switching from this state.
    virtual void OnStop() {}

    virtual void DBG_GUI() {}
    
    void SetupTransitionHandler(TransitionHandler* handler) { transitionHandler = handler; }
    TransitionHandler* GetTransitionHandler() { return transitionHandler; }
private:
    TransitionHandler* transitionHandler = nullptr;
};
using GameStageControllerRef = std::shared_ptr<GameStageController>;

//===== GameStage =====

class GameStage {
public:
    void Initialize(const std::vector<GameStageControllerRef>& stages, int initialStageIdx = 0);

    void Update();
    void Render();

    void DBG_GUI();
private:
    int currentStageID = GameStageName::INVALID;
    GameStageControllerRef currentStage = nullptr;

    std::map<int, GameStageControllerRef> stages;

    TransitionHandler transitionHandler;
};

//===== GameInitParams =====

//Contains all the data needed to initialize the ingame state.
struct GameInitParams {

};
