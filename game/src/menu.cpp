#include "menu.h"

using namespace eng;

MainMenuController::MainMenuController(const FontRef& font, const TextureRef& btnTexture) {
    main_menu = GUI::Menu(glm::vec2(0.f), glm::vec2(0.5f), 0.f, std::vector<GUI::Element*>{
        new GUI::TextButton(glm::vec2(0.f,-0.31f * 2.f), glm::vec2(0.9f, 0.3f), 1.f, btnTexture, glm::vec4(0.3f, 0.3f, 0.3f, 1.f),
            font, "Single Player Game", glm::vec4(1.f, 1.f, 1.f, 1.f),
            this, [](GUI::ButtonCallbackHandler* handler, int id){ 
                LOG_INFO("MainMenu::Main - 'Single Player Game' button clicked.");
                MainMenuController& menu = *static_cast<MainMenuController*>(handler);
                menu.SwitchState(MainMenuState::START_GAME);
            }
        ),
        new GUI::TextButton(glm::vec2(0.f, 0.00f * 2.f), glm::vec2(0.9f, 0.3f), 1.f, btnTexture, glm::vec4(0.7f, 0.7f, 0.7f, 1.f),
            font, "Multi Player Game", glm::vec4(1.f, 0.f, .9f, 1.f),
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                LOG_INFO("MainMenu::Main - 'Multi Player Game' button clicked.");
            }
        ),
        new GUI::TextButton(glm::vec2(0.f, 0.31f * 2.f), glm::vec2(0.9f, 0.3f), 1.f, btnTexture, glm::vec4(0.9f, 0.0f, 0.0f, 1.f),
            font, "Exit Program", glm::vec4(0.f, 0.f, 1.f, 1.f),
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                LOG_INFO("MainMenu::Main - 'Exit Program' button clicked.");
                Window::Get().Close();
            }
        ),
    });

    startGame_menu = GUI::Menu(glm::vec2(0.f), glm::vec2(0.5f), 0.f, std::vector<GUI::Element*>{
        new GUI::TextButton(glm::vec2(0.f,-0.31f * 2.f), glm::vec2(0.9f, 0.3f), 1.f, btnTexture, glm::vec4(0.3f, 0.3f, 0.3f, 1.f),
            font, "ASDF", glm::vec4(1.f, 1.f, 1.f, 1.f),
            this, [](GUI::ButtonCallbackHandler* handler, int id){ 
                LOG_INFO("MainMenu::StartGame - 'ASDF' button clicked.");
            }
        ),
        new GUI::TextButton(glm::vec2(0.f, 0.00f * 2.f), glm::vec2(0.9f, 0.3f), 1.f, btnTexture, glm::vec4(0.7f, 0.7f, 0.7f, 1.f),
            font, "FDAS", glm::vec4(1.f, 0.f, .9f, 1.f),
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                LOG_INFO("MainMenu::StartGame - 'FDAS' button clicked.");
            }
        ),
        new GUI::TextButton(glm::vec2(0.f, 0.31f * 2.f), glm::vec2(0.9f, 0.3f), 1.f, btnTexture, glm::vec4(0.9f, 0.0f, 0.0f, 1.f),
            font, "Back", glm::vec4(0.f, 0.f, 1.f, 1.f),
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                LOG_INFO("MainMenu::StartGame - 'Back' button clicked.");
                MainMenuController& menu = *static_cast<MainMenuController*>(handler);
                menu.SwitchState(MainMenuState::MAIN);
            }
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