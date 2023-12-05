#pragma once

#include <engine/engine.h>

#include "stage.h"

class Game : public eng::App {
public:
    Game(int argc, char** argv);

    virtual void OnResize(int width, int height) override;
private:
    virtual void OnInit() override;
    virtual void OnUpdate() override;
    virtual void OnGUI() override;

    void ReloadShaders();
private:
    GameStage stageController;

    eng::ShaderRef shader;
    eng::ColorPalette colorPalette;

    DBGONLY(int dbg_stageIdx = -1);
    DBGONLY(int dbg_stageStateIdx = -1);
};
