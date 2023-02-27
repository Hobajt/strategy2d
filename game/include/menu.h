#pragma once

#include <engine/engine.h>

//===== TransitionHandler =====

class TransitionHandler {
public:
    bool IsHappening();
    bool IsDone();
};

//===== MainMenu =====

namespace MainMenuState { enum { MAIN, START_GAME, OPTIONS }; }

class MainMenu : public eng::GUI::ButtonCallbackHandler {
public:
    void Initialize(const eng::FontRef& font, const eng::TextureRef& btnTexture);

    void Update(TransitionHandler& transition);
    void Render();

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
