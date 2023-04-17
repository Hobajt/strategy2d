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

   /*
   act intro:
        - 2 separate fade-ins - first the text (quick), then background (slow + text turns red/blue)
            - the 1st can be done through transition handler - display text on black background & fade-in
            - the 2nd one ... some custom mechanism in here probably
        - play sound
        - when the sound ends or on input interrupt -> fade out & transition to objectives substage
   
   */
}

void RecapController::OnPreStart(int prevStageID, int info, void* data) {

    

    /*what stages can transition to recapStage:
        - main menu - when starting campaign
        - main menu - credits btn clicked
        - in game - game over (both win/lose), display recap & stats
        - in game - restart btn clicked (no act info, only objectives)
    */

    /*objectives:
        - load scenario info (based on campaignIdx)
        - should contain:
            - text (to roll)
            - background texture
            - is act?
                - act name & background texture name
            - play cinematic?
                - cinematic name
        - if is act or play cinematic:
            - switch substage to act intro or interlude
            - keep scenario info loaded
            - need to decide which has precedence, if they're both present (check out the game)
    */

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
