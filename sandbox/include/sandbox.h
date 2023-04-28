#pragma once

#include <engine/engine.h>

class Sandbox : public eng::App {
public:
    Sandbox(int argc, char** argv);

    virtual void OnResize(int width, int height) override;
private:
    virtual void OnInit() override;
    virtual void OnUpdate() override;
    virtual void OnGUI() override;

    void ReloadShaders();
private:
    eng::ShaderRef shader;
    eng::FontRef font;

    eng::GUI::StyleRef guiStyle;

    eng::GUI::ScrollText gui;
    eng::GUI::TextLabel tstGui;
    eng::TextureRef tex;

    eng::SpriteGroup sg;
    eng::SpritesheetRef spritesheet;
};
