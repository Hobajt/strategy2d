#pragma once

#include <engine/engine.h>

#include "menu.h"

namespace GameState { enum { INTRO, MAIN_MENU, INGAME }; }

class Game : public eng::App {
public:
    Game();

    virtual void OnResize(int width, int height) override;
private:
    virtual void OnInit() override;
    virtual void OnUpdate() override;
    virtual void OnGUI() override;

    void ReloadShaders();
private:
    eng::ShaderRef shader;
    eng::TextureRef texture;
    eng::TextureRef btnTexture;
    eng::FontRef font;

    int state = GameState::INTRO;
    MainMenu menu = {};
    TransitionHandler transition;

    eng::SelectionHandler guiSelection;
    eng::SelectionHandler mapSelection;
};
