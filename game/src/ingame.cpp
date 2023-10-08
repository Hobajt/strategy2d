#include "ingame.h"

using namespace eng;

static std::string stage_names[] = { "CAMPAIGN", "CUSTOM", "LOAD" };

IngameController::IngameController() {}

void IngameController::Update() {

}

void IngameController::Render() {
    Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f), glm::vec2(0.5f), glm::vec4(1.f)));
}

void IngameController::OnPreLoad(int prevStageID, int info, void* data) {
    //start loading game assets - probably just map data, since opengl can't handle async loading

    /* ways to reach ingame stage:
        - mainMenu - start campaign (goes through recap stage tho)
        - mainMenu - start custom
        - mainMenu - load
        - ingame - restart
        - ingame - load
        - ingame menu? (if it's gonna be a separate stage, probably not tho)
    */
}

void IngameController::OnPreStart(int prevStageID, int info, void* data) {

}

void IngameController::OnStart(int prevStageID, int info, void* data) {
    LOG_INFO("GameStage = InGame (init = {})", stage_names[info]);

    Input::Get().AddKeyCallback(-1, [](int keycode, int modifiers, void* handler){
        static_cast<IngameController*>(handler)->SignalKeyPress(keycode, modifiers);
    }, true, this);
}

void IngameController::OnStop() {

}
