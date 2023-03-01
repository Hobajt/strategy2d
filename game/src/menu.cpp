#include "menu.h"

using namespace eng;

MainMenuController::MainMenuController(const FontRef& font, const eng::TextureRef& backgroundTexture_) : backgroundTexture(backgroundTexture_) {
    //gui sizes in screen space
    glm::vec2 buttonSize = glm::vec2(0.33f, 0.06f);         //button size
    glm::vec2 menuSize = glm::vec2(0.5f);                   //menu size
    float gap = 0.03f;                                      //vertical gap between the buttons

    float off = -1.f + buttonSize.y;                        //button offset to start at the top of the menu
    float step = (2.f*buttonSize.y + gap) / menuSize.y;     //vertical step to move each button
    glm::vec2 bSize = buttonSize / menuSize;                //final button size - converted from size relative to parent
    
    //==== button textures setup ====
    glm::vec2 ts = glm::vec2(Window::Get().Size()) * buttonSize;
    float upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
    glm::ivec2 textureSize = ts * upscaleFactor;
    int borderWidth = 2 * upscaleFactor;
    btnTexture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, 0, false);
    btnTextureClick = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, 0, true);

    //==== style setup for the GUI buttons ====
    GUI::StyleRef style = std::make_shared<GUI::Style>();
    style->texture = btnTexture;
    style->hoverTexture = btnTexture;
    style->pressedTexture = btnTextureClick;
    style->textColor = glm::vec4(0.92f, 0.77f, 0.20f, 1.f);
    style->font = font;
    style->pressedOffset = glm::ivec2(borderWidth);       //= texture border width

    //==== sub-menu definitions ====

    main_menu = GUI::Menu(glm::vec2(0.f, 0.5f), glm::vec2(0.5f), 0.f, std::vector<GUI::Element*>{
        new GUI::TextButton(glm::vec2(0.f, off+step*0), bSize, 1.f, style, "Single Player Game",
            this, [](GUI::ButtonCallbackHandler* handler, int id){ 
                MainMenuController& menu = *static_cast<MainMenuController*>(handler);
                menu.SwitchState(MainMenuState::START_GAME);
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*1), bSize, 1.f, style, "Multi Player Game",
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                //...
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*2), bSize, 1.f, style, "Replay Introduction",
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                //...
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*3), bSize, 1.f, style, "Show Credits",
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                //...
            }, 5
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*4), bSize, 1.f, style, "Exit Program",
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                Window::Get().Close();
            }, 1
        ),
    });

    startGame_menu = GUI::Menu(glm::vec2(0.f, 0.5f), glm::vec2(0.5f), 0.f, std::vector<GUI::Element*>{
        new GUI::TextButton(glm::vec2(0.f, off+step*0), bSize, 1.f, style, "New Campaign",
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                //...
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*1), bSize, 1.f, style, "Load Game",
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                //...
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*2), bSize, 1.f, style, "Custom Scenario",
            this, [](GUI::ButtonCallbackHandler* handler, int id){
                //...
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*3), bSize, 1.f, style, "Previous Menu",
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

void MainMenuController::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Begin("Menu controller");
    btnTexture->DBG_GUI();
    ImGui::End();
#endif
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
