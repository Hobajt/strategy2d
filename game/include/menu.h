#pragma once

#include <engine/engine.h>

#include "stage.h"

//===== MainMenu =====

namespace MainMenuState { enum { INVALID, MAIN, START_GAME, OPTIONS }; }

class MainMenuController : public GameStageController, public eng::GUI::ButtonCallbackHandler {
public:
    MainMenuController(const eng::FontRef& font, const eng::TextureRef& btnTexture);

    virtual void Update() override;
    virtual void Render() override;

    virtual int GetStageID() const override { return GameStageName::MAIN_MENU; }
private:
    void SwitchState(int newState);
private:

    eng::GUI::Menu main_menu;
    eng::GUI::Menu startGame_menu;

    eng::GUI::Menu* activeMenu = nullptr;
    int activeState = MainMenuState::INVALID;
};
