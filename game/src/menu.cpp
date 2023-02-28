#include "menu.h"

using namespace eng;

MainMenuController::MainMenuController(const FontRef& font, const TextureRef& btnTexture, const eng::TextureRef& btnTextureClick, const eng::TextureRef& backgroundTexture_)
        : backgroundTexture(backgroundTexture_) {

    float bh = 0.1f;                    //button height
    float bw = 0.33f;
    float gap = 0.03f;
    float bg = (bh + gap) * 2.f;      //button gap (*2 to offset menu's size scaling)
    float mw = 0.5f;
    float mh = 0.5f;
    float mwi = 1.f / mw;
    float mhi = 1.f / mh;

    GUI::ElementProperties ep = {};
    ep.texture = btnTexture;
    ep.hoverTexture = btnTexture;
    ep.pressedTexture = btnTextureClick;
    ep.textColor = glm::vec4(0.92f, 0.77f, 0.20f, 1.f);
    ep.font = font;

    main_menu = GUI::Menu(glm::vec2(0.f, 0.5f), glm::vec2(0.5f), 0.f, std::vector<GUI::Element*>{
        new GUI::TextButton(glm::vec2(0.f,-1.f+bh+bg*0), glm::vec2(bw*mwi, bh), 1.f, ep, "Single Player Game",
            this, [](GUI::ButtonCallbackHandler* handler, int id){ 
                MainMenuController& menu = *static_cast<MainMenuController*>(handler);
                menu.SwitchState(MainMenuState::START_GAME);
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f,-1.f+bh+bg*1), glm::vec2(bw*mwi, bh), 1.f, ep, "Multi Player Game",
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                MainMenuController& menu = *static_cast<MainMenuController*>(handler);
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f,-1.f+bh+bg*2), glm::vec2(bw*mwi, bh), 1.f, ep, "Replay Introduction",
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                MainMenuController& menu = *static_cast<MainMenuController*>(handler);
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f,-1.f+bh+bg*3), glm::vec2(bw*mwi, bh), 1.f, ep, "Show Credits",
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                MainMenuController& menu = *static_cast<MainMenuController*>(handler);
            }, 5
        ),
        new GUI::TextButton(glm::vec2(0.f,-1.f+bh+bg*4), glm::vec2(bw*mwi, bh), 1.f, ep, "Exit Program",
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                Window::Get().Close();
            }, 1
        ),
    });

    startGame_menu = GUI::Menu(glm::vec2(0.f, 0.5f), glm::vec2(0.5f), 0.f, std::vector<GUI::Element*>{
        new GUI::TextButton(glm::vec2(0.f,-1.f+bh+bg*0), glm::vec2(bw*mwi, bh), 1.f, ep, "New Campaign",
            this, [](GUI::ButtonCallbackHandler* handler, int id){ 
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f,-1.f+bh+bg*1), glm::vec2(bw*mwi, bh), 1.f, ep, "Load Game",
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                MainMenuController& menu = *static_cast<MainMenuController*>(handler);
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f,-1.f+bh+bg*2), glm::vec2(bw*mwi, bh), 1.f, ep, "Custom Scenario",
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                MainMenuController& menu = *static_cast<MainMenuController*>(handler);
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f,-1.f+bh+bg*3), glm::vec2(bw*mwi, bh), 1.f, ep, "Previous Menu",
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                MainMenuController& menu = *static_cast<MainMenuController*>(handler);
                menu.SwitchState(MainMenuState::MAIN);
            }, 0
        ),
    });

    SwitchState(MainMenuState::MAIN);
}

void MainMenuController::Update() {
    ASSERT_MSG(activeMenu != nullptr, "There should always be active submenu in the main menu.");

    Input& input = Input::Get();

    GUI::Element* selection = activeMenu->ResolveMouseSelection(input.mousePos_n);
    if(selection != nullptr) {
        if(input.lmb.pressed()) {
            if(input.lmb.down())
                clickedElement = selection;
            if(selection == clickedElement)
                selection->OnPressed();
        }
        else if(input.lmb.up()) {
            if(selection == clickedElement) selection->OnUp();
            clickedElement = nullptr;
        }
        else {
            selection->OnHover();
        }
    }
}

void MainMenuController::Render() {
    ASSERT_MSG(activeMenu != nullptr, "There should always be active submenu in the main menu.");
    Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f), glm::vec3(1.f), glm::vec4(1.f), backgroundTexture));
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

void MainMenuController::OnStart(int prevStageID) {
    Input::Get().AddKeyCallback(-1, [](int keycode, int modifiers, void* data){
        static_cast<MainMenuController*>(data)->KeyPressCallback(keycode, modifiers);
    }, true, this);
}

void MainMenuController::OnStop() {

}

void MainMenuController::KeyPressCallback(int keycode, int modifiers) {
    switch(activeState) {
        case MainMenuState::MAIN:
            switch(keycode) {
                case GLFW_KEY_S: SwitchState(MainMenuState::START_GAME); break;
                case GLFW_KEY_M: break;
                case GLFW_KEY_R: break;
                case GLFW_KEY_C: break;
                case GLFW_KEY_X: Window::Get().Close(); break;
            }
            break;
        case MainMenuState::START_GAME:
            switch(keycode) {
                case GLFW_KEY_N: break;
                case GLFW_KEY_L: break;
                case GLFW_KEY_C: break;
                case GLFW_KEY_P: SwitchState(MainMenuState::MAIN); break;
            }
            break;
    }
}
