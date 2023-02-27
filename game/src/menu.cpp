#include "menu.h"

using namespace eng;

void MainMenu::Initialize(const FontRef& font, const TextureRef& btnTexture) {
    main_menu = GUI::Menu(glm::vec2(0.f), glm::vec2(0.5f), 0.f, std::vector<GUI::Button>{
        GUI::Button(glm::vec2(0.f,-0.31f * 2.f), glm::vec2(0.9f, 0.3f), 1.f, btnTexture, glm::vec4(0.3f, 0.3f, 0.3f, 1.f), this, 1, font, "Single Player Game", glm::vec4(1.f, 1.f, 1.f, 1.f)),
        GUI::Button(glm::vec2(0.f, 0.00f * 2.f), glm::vec2(0.9f, 0.3f), 1.f, btnTexture, glm::vec4(0.7f, 0.7f, 0.7f, 1.f), this, 2, font, "Multi Player Game", glm::vec4(1.f, 0.f, .9f, 1.f)),
        GUI::Button(glm::vec2(0.f, 0.31f * 2.f), glm::vec2(0.9f, 0.3f), 1.f, btnTexture, glm::vec4(0.9f, 0.0f, 0.0f, 1.f), this, 3, font, "Exit Program", glm::vec4(0.f, 0.f, 1.f, 1.f)),
        // GUI::Button(glm::vec2(-1.f, -1.f), glm::vec2(0.5f, 0.3f), 2.f, nullptr, glm::vec4(0.7f, 0.7f, 0.7f, 1.f), font, "KEK", glm::vec4(1.f, 0.f, .9f, 1.f)),
    });

    startGame_menu = GUI::Menu(glm::vec2(0.f), glm::vec2(0.5f), 0.f, std::vector<GUI::Button>{
        GUI::Button(glm::vec2(0.f,-0.31f * 2.f), glm::vec2(0.9f, 0.3f), 1.f, btnTexture, glm::vec4(0.3f, 0.3f, 0.3f, 1.f), this, 11, font, "ASDF", glm::vec4(1.f, 1.f, 1.f, 1.f)),
        GUI::Button(glm::vec2(0.f, 0.00f * 2.f), glm::vec2(0.9f, 0.3f), 1.f, btnTexture, glm::vec4(0.7f, 0.7f, 0.7f, 1.f), this, 12, font, "FDAS", glm::vec4(1.f, 0.f, .9f, 1.f)),
        GUI::Button(glm::vec2(0.f, 0.31f * 2.f), glm::vec2(0.9f, 0.3f), 1.f, btnTexture, glm::vec4(0.9f, 0.0f, 0.0f, 1.f), this, 13, font, "Back", glm::vec4(0.f, 0.f, 1.f, 1.f)),
        // GUI::Button(glm::vec2(-1.f, -1.f), glm::vec2(0.5f, 0.3f), 2.f, nullptr, glm::vec4(0.7f, 0.7f, 0.7f, 1.f), font, "KEK", glm::vec4(1.f, 0.f, .9f, 1.f)),
    });

    SwitchState(MainMenuState::MAIN);
}

void MainMenu::Update(TransitionHandler& transition) {
    Input& input = Input::Get();

    //button hover/click detection
    ScreenObject* selectedObject = selection.GetSelection(input.mousePos_n);
    if(selectedObject != nullptr) {
        if(input.lmb.down()) {
            selectedObject->OnClick();
        }
        else {
            selectedObject->OnHover();
        }
    }

    //button click processing
    if(clickID > 0) {
        switch(clickID) {
            case 1:
                SwitchState(MainMenuState::START_GAME);
                break;
            case 13:
                SwitchState(MainMenuState::MAIN);
                break;
        }
    }
    clickID = 0;
}

void MainMenu::Render() {
    //TODO: render the background image here
    switch(state) {
        default:
        case MainMenuState::MAIN:
            main_menu.Render();
            break;
        case MainMenuState::START_GAME:
            startGame_menu.Render();
            break;
    }
}

void MainMenu::OnClick(int buttonID) {
    LOG_INFO("MainMenu::OnClick - button {}", buttonID);
    clickID = buttonID;
}

void MainMenu::SwitchState(int newState) {
    //TODO: switch state as well as setup variables (selection handler for example)

    selection.visibleObjects.clear();
    switch(newState) {
        case MainMenuState::MAIN:
            selection.visibleObjects.push_back({ main_menu.GetAABB(), &main_menu });
            break;
        case MainMenuState::START_GAME:
            selection.visibleObjects.push_back({ startGame_menu.GetAABB(), &startGame_menu });
            break;
    }
    state = newState;

    //TODO: maybe rework, so that single menu object covers the entire menu stuff
    //then you could just change pointer to different menu, wouldn't have to fuck around with all these switches

    //also, maybe the button ID resolution could be done through some kind of array
}