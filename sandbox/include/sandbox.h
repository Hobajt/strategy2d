#pragma once

#include <engine/engine.h>

class MockIngameStageController : public eng::PlayerFactionController::GUIRequestHandler {
public:
    void RegisterKeyCallback();

    virtual void PauseRequest(bool pause) override;
    virtual void PauseToggleRequest() override;
    virtual void ChangeLevel(const std::string& filename) override;

    void LevelSwitching(eng::Level& level);
private:
    bool paused = false;

    bool switch_signaled = false;
    std::string switch_filepath;
};

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

    int buildingID = 0;

    eng::ObjectID trollID;

    MockIngameStageController ingameStage;
};
