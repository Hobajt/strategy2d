#pragma once

#include <engine/engine.h>

#include "stage.h"

namespace IntroState { enum { INVALID, GAME_START, COMPANY_LOGO, CINEMATIC, GAME_LOGO, CINEMATIC_REPLAY, COUNT }; }

//Manages both intro screens, starting cinematic as well as credits.
class IntroController : public GameStageController {
public:
    IntroController(const eng::TextureRef& gameLogoTexture);

    virtual void Update() override;
    virtual void Render() override;

    virtual int GetStageID() const override { return GameStageName::INTRO; }

    virtual void OnPreStart(int prevStageID, int data) override;
    virtual void OnStart(int prevStageID, int data) override;
    virtual void OnStop() override;
private:
    void KeyPressCallback(int keycode, int modifiers);
private:
    int state = IntroState::INVALID;

    float timeStart = 0.f;
    bool interrupted = false;

    eng::TextureRef gameLogoTexture = nullptr;
};