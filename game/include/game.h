#pragma once

#include <engine/engine.h>

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
    eng::FontRef font;

    eng::InputButton k1,k2,k3;
};
