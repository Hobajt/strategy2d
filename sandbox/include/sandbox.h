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
    
    eng::Level level;

    eng::TextureRef texture;
    eng::ColorPalette colorPalette;

    bool whiteBackground = false;
    int paletteIndex = 0;

    eng::SpritesheetRef spritesheet;
    eng::Unit troll;


    int action = 0;
    int orientation = 0;
    int color = 0;
    int frame = 0;
    eng::Animator anim;

    eng::Sprite icon;
    glm::ivec2 iconIdx = glm::ivec2(0);
};
