#pragma once

#include <engine/engine.h>

#include "stage.h"

class Game : public eng::App {
public:
    Game();
    ~Game();

    virtual void OnResize(int width, int height) override;
private:
    virtual void OnInit() override;
    virtual void OnUpdate() override;
    virtual void OnGUI() override;

    void ReloadShaders();
private:
    GameStage stageController;

    eng::ShaderRef shader;
};
