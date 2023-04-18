#include "stage.h"

using namespace eng;

//===== TransitionHandler =====

TransitionParameters::TransitionParameters(float duration_, int transitionType_, int nextStageID_, int info_, bool autoFadeIn_)
    : TransitionParameters(duration_, transitionType_, nextStageID_, info_, nullptr, autoFadeIn_) {}

TransitionParameters::TransitionParameters(float duration_, int transitionType_, int nextStageID_, int info_, void* data_, bool autoFadeIn_)
    : duration(duration_), type(transitionType_), nextStage(nextStageID_), info(info_), autoFadeIn(autoFadeIn_), data(data_) {}

bool TransitionParameters::EqualsTo(const TransitionParameters& o, float durationOverride) {
    bool res = type == o.type && nextStage == o.nextStage && info == o.info && autoFadeIn == o.autoFadeIn;
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
        startedFlag = active = true;
        params = params_;
        startTime = Input::Get().CurrentTime();
        endTime = startTime + params.duration;
        fadingOut = (params.type != TransitionType::FADE_IN);
        return true;
    }
    return false;
}

bool TransitionHandler::TransitionStarted() { 
    bool sf = startedFlag; 
    startedFlag = false;
    return sf;
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
    currentStage->OnPreStart(GameStageName::INVALID, -1, nullptr);
}

void GameStage::Update() {
    currentStage->Update();

    if(transitionHandler.TransitionStarted() && currentStageID != transitionHandler.NextStageID()) {
        stages[transitionHandler.NextStageID()]->OnPreLoad(currentStageID, transitionHandler.Info(), transitionHandler.Data());
    }
}

void GameStage::Render() {
    currentStage->Render();
    if(transitionHandler.Update()) {
        currentStage->OnStop();
        currentStage = stages[transitionHandler.NextStageID()];
        if(transitionHandler.IsFadedOut())
            currentStage->OnPreStart(currentStageID, transitionHandler.Info(), transitionHandler.Data());
        else
            currentStage->OnStart(currentStageID, transitionHandler.Info(), transitionHandler.Data());
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
