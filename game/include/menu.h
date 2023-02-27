#pragma once

#include <engine/engine.h>

#include "stage.h"

//===== MainMenu =====

namespace MainMenuState { enum { MAIN, START_GAME, OPTIONS }; }

class MainMenuController : public GameStageController, public eng::GUI::ButtonCallbackHandler {
public:
    MainMenuController(const eng::FontRef& font, const eng::TextureRef& btnTexture);

    virtual void Update() override;
    virtual void Render() override;

    virtual int GetStageID() const { return GameStageName::MAIN_MENU; }

    virtual void OnClick(int buttonID) override;
private:
    void SwitchState(int newState);
private:
    int state = MainMenuState::MAIN;

    eng::GUI::Menu main_menu;
    eng::GUI::Menu startGame_menu;

    eng::SelectionHandler selection;
    int clickID = 0;
};
