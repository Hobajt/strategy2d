#include "menu.h"

using namespace eng;

MainMenuController::MainMenuController(const FontRef& font, const TextureRef& btnTexture) {
    glm::vec4 txtClr = glm::vec4(0.92f, 0.77f, 0.20f, 1.f);

    // Input::Get().AddKeyCallback(-1, [](int, int){}, true);

    float bh = 0.2f;                    //button height
    float bg = (bh + 0.01f) * 2.f;      //button gap (*2 to offset menu's size scaling)

    main_menu = GUI::Menu(glm::vec2(0.f), glm::vec2(0.5f), 0.f, std::vector<GUI::Element*>{
        new GUI::TextButton(glm::vec2(0.f,-1.f+bh+bg*0), glm::vec2(0.9f, bh), 1.f, btnTexture, glm::vec4(0.3f, 0.3f, 0.3f, 1.f),
            font, "Single Player Game", txtClr,
            this, [](GUI::ButtonCallbackHandler* handler, int id){ 
                MainMenuController& menu = *static_cast<MainMenuController*>(handler);
                menu.SwitchState(MainMenuState::START_GAME);
            }, -1, 0
        ),
        new GUI::TextButton(glm::vec2(0.f,-1.f+bh+bg*1), glm::vec2(0.9f, bh), 1.f, btnTexture, glm::vec4(0.7f, 0.7f, 0.7f, 1.f),
            font, "Multi Player Game", txtClr,
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                MainMenuController& menu = *static_cast<MainMenuController*>(handler);
            }, -1, 0
        ),
        new GUI::TextButton(glm::vec2(0.f,-1.f+bh+bg*2), glm::vec2(0.9f, bh), 1.f, btnTexture, glm::vec4(0.7f, 0.7f, 0.7f, 1.f),
            font, "Replay Introduction", txtClr,
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                MainMenuController& menu = *static_cast<MainMenuController*>(handler);
            }, -1, 0
        ),
        new GUI::TextButton(glm::vec2(0.f,-1.f+bh+bg*3), glm::vec2(0.9f, bh), 1.f, btnTexture, glm::vec4(0.7f, 0.7f, 0.7f, 1.f),
            font, "Show Credits", txtClr,
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                MainMenuController& menu = *static_cast<MainMenuController*>(handler);
            }, -1, 5
        ),
        new GUI::TextButton(glm::vec2(0.f,-1.f+bh+bg*4), glm::vec2(0.9f, bh), 1.f, btnTexture, glm::vec4(0.9f, 0.0f, 0.0f, 1.f),
            font, "Exit Program", txtClr,
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                MainMenuController& menu = *static_cast<MainMenuController*>(handler);
                Window::Get().Close();
            }, -1, 1
        ),
    });

    bh = 0.2f;
    bg = (bh + 0.01f) * 2.f;
    startGame_menu = GUI::Menu(glm::vec2(0.f), glm::vec2(0.5f), 0.f, std::vector<GUI::Element*>{
        new GUI::TextButton(glm::vec2(0.f,-1.f+bh+bg*0), glm::vec2(0.9f, bh), 1.f, btnTexture, glm::vec4(0.3f, 0.3f, 0.3f, 1.f),
            font, "New Campaign", txtClr,
            this, [](GUI::ButtonCallbackHandler* handler, int id){ 
            }, -1, 0
        ),
        new GUI::TextButton(glm::vec2(0.f,-1.f+bh+bg*1), glm::vec2(0.9f, bh), 1.f, btnTexture, glm::vec4(0.7f, 0.7f, 0.7f, 1.f),
            font, "Load Game", txtClr,
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                MainMenuController& menu = *static_cast<MainMenuController*>(handler);
            }, -1, 0
        ),
        new GUI::TextButton(glm::vec2(0.f,-1.f+bh+bg*2), glm::vec2(0.9f, bh), 1.f, btnTexture, glm::vec4(0.9f, 0.0f, 0.0f, 1.f),
            font, "Custom Scenario", txtClr,
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                MainMenuController& menu = *static_cast<MainMenuController*>(handler);
            }, -1, 0
        ),
        new GUI::TextButton(glm::vec2(0.f,-1.f+bh+bg*3), glm::vec2(0.9f, bh), 1.f, btnTexture, glm::vec4(0.9f, 0.0f, 0.0f, 1.f),
            font, "Previous Menu", txtClr,
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                MainMenuController& menu = *static_cast<MainMenuController*>(handler);
                menu.SwitchState(MainMenuState::MAIN);
            }, -1, 0
        ),
    });

    SwitchState(MainMenuState::MAIN);
}

void MainMenuController::Update() {
    ASSERT_MSG(activeMenu != nullptr, "There should always be active submenu in the main menu.");

    Input& input = Input::Get();

    GUI::Element* selection = activeMenu->ResolveMouseSelection(input.mousePos_n);
    if(selection != nullptr) {
        if(input.lmb.down())
            selection->OnClick();
        else
            selection->OnHover();
    }
}

void MainMenuController::Render() {
    ASSERT_MSG(activeMenu != nullptr, "There should always be active submenu in the main menu.");
    //TODO: render the background image here
    activeMenu->Render();
}

void MainMenuController::SwitchState(int newState) {
    switch(newState) {
        default:
            LOG_WARN("MainMenu - Unrecognized menu state ({}), switching to main submenu.", newState);
            newState = MainMenuState::MAIN;
        case MainMenuState::MAIN:
            activeMenu = &main_menu;
            break;
        case MainMenuState::START_GAME:
            activeMenu = &startGame_menu;
            break;
    }
    activeState = newState;
}