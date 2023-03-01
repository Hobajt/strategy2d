#include "stage.h"

using namespace eng;

//===== GameStageTransitionHandler =====

bool GameStageTransitionHandler::Update() {
    float currentTime = Input::CurrentTime();
    if(active) {
        float t =  (currentTime - startTime) / (endTime - startTime);
        Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f), glm::vec2(1.f), glm::vec4(0.f, 0.f, 0.f, t)));
        if(endTime >= currentTime) {
            //...
            active = false;
            return true;
        }
    }
    return false;
}

//===== GameStage =====

void GameStage::Initialize(const std::vector<GameStageControllerRef>& stages_, int initialStageIdx) {
    for(const GameStageControllerRef& stage : stages_) {
        ASSERT_MSG(stages.count(stage->GetStageID()) == 0, "GameStage - Duplicate game stage controllers for stage {}", stage->GetStageID());
        stages.insert({ stage->GetStageID(), stage });
    }

    currentStage = stages_[initialStageIdx];
    currentStageID = currentStage->GetStageID();
    currentStage->OnStart(GameStageName::INVALID);
}

void GameStage::Update() {
    currentStage->Update();
}

void GameStage::Render() {
    currentStage->Render();
    if(transition.Update()) {
        currentStage->OnStop();
        currentStage = stages[transition.NextStageID()];
        currentStage->OnStart(currentStageID);
        currentStageID = transition.NextStageID();
    }
}

void GameStage::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
    for(auto& [id, ctrl] : stages) {
        ctrl->DBG_GUI();
    }
#endif
}
