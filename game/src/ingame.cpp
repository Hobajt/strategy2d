#include "ingame.h"

static std::string stage_names[] = { "CAMPAIGN", "CUSTOM", "LOAD" };

IngameController::IngameController() {}

void IngameController::Update() {

}

void IngameController::Render() {

}

void IngameController::OnPreStart(int prevStageID, int info, void* data) {

}

void IngameController::OnStart(int prevStageID, int info, void* data) {
    LOG_INFO("GameStage = InGame (init = {})", stage_names[info]);
}

void IngameController::OnStop() {

}
