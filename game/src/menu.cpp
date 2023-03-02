#include "menu.h"

using namespace eng;

MainMenuController::MainMenuController(const FontRef& font, const eng::TextureRef& backgroundTexture_) : backgroundTexture(backgroundTexture_) {
    glm::vec4 textColor = glm::vec4(0.92f, 0.77f, 0.20f, 1.f);;

    //gui sizes in screen space
    glm::vec2 buttonSize = glm::vec2(0.33f, 0.06f);         //button size
    glm::vec2 menuSize = glm::vec2(0.5f);                   //menu size
    float gap = 0.03f;                                      //vertical gap between the buttons

    float off = -1.f + buttonSize.y / menuSize.y;           //button offset to start at the top of the menu
    float step = (2.f*buttonSize.y + gap) / menuSize.y;     //vertical step to move each button
    glm::vec2 bSize = buttonSize / menuSize;                //final button size - converted from size relative to parent
    
    //==== button textures setup ====
    glm::vec2 ts = glm::vec2(Window::Get().Size()) * buttonSize;
    float upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
    glm::ivec2 textureSize = ts * upscaleFactor;
    int borderWidth = 2 * upscaleFactor;

    btnTexture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, 0, false);
    btnTextureClick = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, 0, true);
    btnTextureHighlight = TextureGenerator::ButtonHighlightTexture(textureSize.x, textureSize.y, borderWidth);

    //==== style setup for the GUI buttons ====
    GUI::StyleRef style = std::make_shared<GUI::Style>();
    style->texture = btnTexture;
    style->hoverTexture = btnTexture;
    style->holdTexture = btnTextureClick;
    style->highlightTexture = btnTextureHighlight;
    style->textColor = textColor;
    style->font = font;
    style->holdOffset = glm::ivec2(borderWidth);       //= texture border width

    //==== sub-menu definitions ====

    main_menu = GUI::Menu(glm::vec2(0.f, 0.5f), menuSize, 0.f, std::vector<GUI::Element*>{
        new GUI::TextButton(glm::vec2(0.f, off+step*0), bSize, 1.f, style, "Single Player Game",
            this, [](GUI::ButtonCallbackHandler* handler, int id) { 
                static_cast<MainMenuController*>(handler)->SwitchState(MainMenuState::START_GAME);
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*1), bSize, 1.f, style, "Multi Player Game",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                //...
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*2), bSize, 1.f, style, "Replay Introduction",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                //...
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*3), bSize, 1.f, style, "Show Credits",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                //...
            }, 5
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*4), bSize, 1.f, style, "Exit Program",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                Window::Get().Close();
            }, 1
        ),
    });

    startGame_menu = GUI::Menu(glm::vec2(0.f, 0.5f), menuSize, 0.f, std::vector<GUI::Element*>{
        new GUI::TextButton(glm::vec2(0.f, off+step*0), bSize, 1.f, style, "New Campaign",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                //...
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*1), bSize, 1.f, style, "Load Game",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                static_cast<MainMenuController*>(handler)->SwitchState(MainMenuState::LOAD_GAME);
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*2), bSize, 1.f, style, "Custom Scenario",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                //...
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*3), bSize, 1.f, style, "Previous Menu",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                static_cast<MainMenuController*>(handler)->SwitchState(MainMenuState::MAIN);
            }, 0
        ),
    });

    buttonSize = glm::vec2(0.33f, 0.06f);
    menuSize = glm::vec2(0.5f, 0.6f);
    glm::vec2 smallButtonSize = glm::vec2(0.1666f, 0.06f);
    glm::vec2 scrollMenuSize = glm::vec2(0.45f, 0.35f);
    gap = 0.02f;

    off = -1.f + buttonSize.y / menuSize.y;
    bSize = buttonSize / menuSize;
    glm::vec2 smSize = scrollMenuSize / menuSize;
    glm::vec2 sbSize = smallButtonSize / menuSize;
    step = (buttonSize.y + gap + scrollMenuSize.y) / menuSize.y;            //step for menu
    float smStep = step + (scrollMenuSize.y + gap + 2.f * buttonSize.y) / menuSize.y;      //step after the menu

    GUI::StyleRef dummy = std::make_shared<GUI::Style>();

    GUI::StyleRef txtStyle = std::make_shared<GUI::Style>();
    txtStyle->font = font;
    txtStyle->color = glm::vec4(0.f);
    loadGame_menu = GUI::Menu(glm::vec2(0.f, 0.4f), menuSize, 0.f, {
        new GUI::TextLabel(glm::vec2(0.f, off), bSize, 1.f, txtStyle, "Load Game"),
        new GUI::ScrollMenu(glm::vec2(0.f, off+step), smSize, 1.f, 8, 1.f / 16.f, { style, dummy, style, style }),
        new GUI::TextButton(glm::vec2(-0.4f, off+smStep), bSize, 1.f, style, "Load",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                //...
            }, 0
        ),
        new GUI::TextButton(glm::vec2(1.f - sbSize.x + 0.1f, off+smStep), sbSize, 1.f, style, "Cancel", 
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                static_cast<MainMenuController*>(handler)->SwitchState(MainMenuState::START_GAME);
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
        if(input.lmb.down()) {
            lastSelected = clickedElement = selection;
            selection->OnDown();
        }
        else if(input.lmb.pressed() && selection == clickedElement) {
            selection->OnHold();
        }
        else if(input.lmb.up()) {
            if(selection == clickedElement) selection->OnUp();
            clickedElement = nullptr;
        }
        else {
            selection->OnHover();
        }

        // OnDown() & OnHold() can fire in the same frame with this setup
        // if(input.lmb.pressed()) {
        //     if(input.lmb.down()) {
        //         lastSelected = clickedElement = selection;
        //         selection->OnDown();
        //     }
        //     if(selection == clickedElement)
        //         selection->OnHold();
        // }
        // else if(input.lmb.up()) {
        //     if(selection == clickedElement) selection->OnUp();
        //     clickedElement = nullptr;
        // }
        // else {
        //     selection->OnHover();
        // }

        // ENG_LOG_INFO("TICK");
    }

    if(lastSelected != nullptr)
        lastSelected->Highlight();
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
        case MainMenuState::LOAD_GAME:
            activeMenu = &loadGame_menu;
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
                case GLFW_KEY_L: SwitchState(MainMenuState::LOAD_GAME); break;
                case GLFW_KEY_C: break;
                case GLFW_KEY_P: SwitchState(MainMenuState::MAIN); break;
            }
            break;
    }
}
