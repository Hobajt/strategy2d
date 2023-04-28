#pragma once

#include <engine/engine.h>

#include "menu.h"

class Editor : public eng::App {
public:
    Editor(int argc, char** argv);

    virtual void OnResize(int width, int height) override;
private:
    virtual void OnInit() override;
    virtual void OnUpdate() override;
    virtual void OnGUI() override;

    void Terrain_SetupNew();
    int Terrain_Load();
    int Terrain_Save();
private:
    eng::ShaderRef shader;
    eng::FontRef font;

    eng::InputButton btn_toggleFullscreen;
    eng::InputButton btn_toggleGUI;

    bool guiEnabled = true;

    FileMenu fileMenu;
};
