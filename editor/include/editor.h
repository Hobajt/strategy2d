#pragma once

#include <engine/engine.h>

#include "context.h"

class Editor : public eng::App {
public:
    Editor(int argc, char** argv);

    virtual void OnResize(int width, int height) override;
private:
    virtual void OnInit() override;
    virtual void OnUpdate() override;
    virtual void OnGUI() override;
private:
    eng::ShaderRef shader;

    eng::InputButton btn_toggleFullscreen;

    EditorContext context;
};
