#include "stage.h"

using namespace eng;

//===== TransitionHandler =====

TransitionParameters::TransitionParameters(float duration_, int transitionType_, int nextStageID_, int data_, bool autoFadeIn_)
    : duration(duration_), type(transitionType_), nextStage(nextStageID_), data(data_), autoFadeIn(autoFadeIn_) {}

bool TransitionParameters::EqualsTo(const TransitionParameters& o, float durationOverride) {
    bool res = type == o.type && nextStage == o.nextStage && data == o.data && autoFadeIn == o.autoFadeIn;
    res &= (durationOverride < 0.f) ? (duration == o.duration) : (duration == durationOverride);
    return res;
}

TransitionParameters TransitionParameters::WithDuration(float d) {
    TransitionParameters params = *this;
    params.duration = d;
    return params;
}

bool TransitionHandler::Update() {
    float currentTime = Input::CurrentTime();
    if(active) {
        //fade-in/out transition; render black quad with alpha channel interpolated based on time
        float t = std::min((currentTime - startTime) / (endTime - startTime), 1.f);
        t = fadingOut ? t : (1.f - t);
        Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f, 0.f, -1.f), glm::vec2(1.f), glm::vec4(0.f, 0.f, 0.f, t)));

        //transition done
        if(endTime <= currentTime) {
            fadedOut = fadingOut;
            active = false;
            return true;
        }
    }
    else if(fadedOut) {
        //render black quad when transition ends in faded out state
        Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f, 0.f, -1.f), glm::vec2(1.f), glm::vec4(0.f, 0.f, 0.f, 1.f)));
    }
    return false;
}

void TransitionHandler::AutoFadeIn() {
    if(params.autoFadeIn && params.type == TransitionType::FADE_OUT) {
        params.type = TransitionType::FADE_IN;
        InitTransition(params);
    }
}

bool TransitionHandler::InitTransition(const TransitionParameters& params_, bool forceOverride) {
    if(!active || forceOverride) {
        active = true;
        params = params_;
        startTime = Input::Get().CurrentTime();
        endTime = startTime + params.duration;
        fadingOut = (params.type != TransitionType::FADE_IN);
        return true;
    }
    return false;
}

//===== GameStage =====

void GameStage::Initialize(const std::vector<GameStageControllerRef>& stages_, int initialStageIdx) {
    for(const GameStageControllerRef& stage : stages_) {
        ASSERT_MSG(stages.count(stage->GetStageID()) == 0, "GameStage - Duplicate game stage controllers for stage {}", stage->GetStageID());
        stages.insert({ stage->GetStageID(), stage });
        stage->SetupTransitionHandler(&transitionHandler);
    }

    transitionHandler.ForceFadeOut();
    currentStage = stages_[initialStageIdx];
    currentStageID = currentStage->GetStageID();
    currentStage->OnPreStart(GameStageName::INVALID, -1);
}

void GameStage::Update() {
    currentStage->Update();
}

void GameStage::Render() {
    currentStage->Render();
    if(transitionHandler.Update()) {
        currentStage->OnStop();
        currentStage = stages[transitionHandler.NextStageID()];
        if(transitionHandler.IsFadedOut())
            currentStage->OnPreStart(currentStageID, transitionHandler.NextStageData());
        else
            currentStage->OnStart(currentStageID, transitionHandler.NextStageData());
        currentStageID = transitionHandler.NextStageID();
        
        transitionHandler.AutoFadeIn();
    }
}

void GameStage::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
    for(auto& [id, ctrl] : stages) {
        ctrl->DBG_GUI();
    }
#endif
}
