#include "recap.h"

RecapController::RecapController() {}

void RecapController::Update() {

}

void RecapController::Render() {
    /*
    render() depends on state:
        - objectives    = background texture + rolling text
        - credits       = same as objectives
        - recap         = background texture + stats + slowly revealing bars
        - act intro     = background texture + fading transition + text over the fading
        - interlude     = video playing
    */
}

void RecapController::OnPreStart(int prevStageID, int info, void* data) {
    /*OBJECTIVES -> rolling text with cool background texture
        - load rolling text from file based on campaignIdx
        - texture is based on race (and probably campaignIdx as well)
    */
}

void RecapController::OnStart(int prevStageID, int info, void* data) {
    LOG_INFO("GameStage = Recap");
}

void RecapController::OnStop() {

}
