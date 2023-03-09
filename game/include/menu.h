#pragma once

#include <engine/engine.h>

#include "stage.h"

//===== MainMenu =====

namespace MainMenuState { enum { INVALID, MAIN, SINGLE, SINGLE_CAMPAIGN, SINGLE_LOAD, SINGLE_CUSTOM }; }

class MainMenuController : public GameStageController, public eng::GUI::ButtonCallbackHandler {
public:
    MainMenuController(const eng::FontRef& font, const eng::TextureRef& backgroundTexture);

    virtual void Update() override;
    virtual void Render() override;

    virtual int GetStageID() const override { return GameStageName::MAIN_MENU; }

    virtual void OnPreStart(int prevStageID, int info, void* data) override;
    virtual void OnStart(int prevStageID, int info, void* data) override;
    virtual void OnStop() override;

    virtual void DBG_GUI() override;
private:
    void SwitchState(int newState);
    void SwitchStage(int stageID, int info);

    void KeyPressCallback(int keycode, int modifiers);

    void InitSubmenu_Main(const glm::vec2& buttonSize, const glm::vec2& menuSize, float gap, eng::GUI::StyleRef style);
    void InitSubmenu_Single(const glm::vec2& buttonSize, const glm::vec2& menuSize, float gap, eng::GUI::StyleRef style);
    void InitSubmenu_Single_Load(const glm::vec2& buttonSize, const glm::vec2& menuSize, const glm::vec2& scrollMenuSize, const glm::vec2& smallButtonSize, 
        float scrollButtonSize, float gap, int scrollMenuItems, eng::GUI::StyleMap& styles);
    void InitSubmenu_Single_Campaign(const glm::vec2& buttonSize, const glm::vec2& menuSize, float gap, eng::GUI::StyleMap& styles);
    void InitSubmenu_Single_Custom(const glm::vec2& buttonSize, const glm::vec2& menuSize, float gap, eng::GUI::StyleMap& styles);
private:
    std::unordered_map<int, eng::GUI::Menu> menu;

    eng::GUI::Menu* activeMenu = nullptr;
    int activeState = MainMenuState::INVALID;

    eng::TextureRef backgroundTexture = nullptr;

    eng::GUI::Element* clickedElement = nullptr;
    eng::GUI::Element* lastSelected = nullptr;

    eng::TextureRef dbg_texture = nullptr;

    GameInitParams gameInitParams;
};
