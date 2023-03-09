#include "ingame.h"

IngameController::IngameController() {}

void IngameController::Update() {

}

void IngameController::Render() {

}

void IngameController::OnPreStart(int prevStageID, int info, void* data) {

}

void IngameController::OnStart(int prevStageID, int info, void* data) {
    LOG_INFO("GameStage = InGame");
}

void IngameController::OnStop() {

}
