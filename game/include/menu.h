#pragma once

#include <engine/engine.h>

#include "stage.h"

//===== MainMenu =====

namespace MainMenuState { enum { INVALID, MAIN, START_GAME, OPTIONS, LOAD_GAME }; }

class MainMenuController : public GameStageController, public eng::GUI::ButtonCallbackHandler {
public:
    MainMenuController(const eng::FontRef& font, const eng::TextureRef& backgroundTexture);

    virtual void Update() override;
    virtual void Render() override;

    virtual int GetStageID() const override { return GameStageName::MAIN_MENU; }

    virtual void OnStart(int prevStageID) override;
    virtual void OnStop() override;

    virtual void DBG_GUI() override;
private:
    void SwitchState(int newState);

    void KeyPressCallback(int keycode, int modifiers);
private:
    eng::GUI::Menu main_menu;
    eng::GUI::Menu startGame_menu;
    eng::GUI::Menu loadGame_menu;

    eng::GUI::Menu* activeMenu = nullptr;
    int activeState = MainMenuState::INVALID;

    eng::TextureRef backgroundTexture = nullptr;
    eng::TextureRef btnTextureHighlight = nullptr;
    eng::TextureRef btnTextureClick = nullptr;
    eng::TextureRef btnTexture = nullptr;

    eng::GUI::Element* clickedElement = nullptr;
    eng::GUI::Element* lastSelected = nullptr;
};
