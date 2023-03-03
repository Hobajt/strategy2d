#include "menu.h"

using namespace eng;

void InitStyles_Main(GUI::StyleMap& styles, const FontRef& font, const glm::vec2& buttonSize);
void InitStyles_Load(GUI::StyleMap& styles, const FontRef& font, const glm::vec2& scrollMenuSize, int scrollMenuItems, float scrollButtonSize, const glm::vec2& smallBtnSize);

static glm::vec4 textClr = glm::vec4(0.92f, 0.77f, 0.20f, 1.f);

//===== MainMenuController =====

MainMenuController::MainMenuController(const FontRef& font, const eng::TextureRef& backgroundTexture_) : backgroundTexture(backgroundTexture_) {
    glm::vec2 main_btnSize = glm::vec2(0.33f, 0.06f);
    glm::vec2 main_menuSize = glm::vec2(0.5f);
    float main_gap = 0.03f;

    glm::vec2 load_btnSize = glm::vec2(0.33f, 0.06f);
    glm::vec2 load_menuSize = glm::vec2(0.5f, 0.6f);
    float load_scrollBtnSize = 1.f / 16.f;
    glm::vec2 load_smallBtnSize = glm::vec2(0.1666f, 0.06f);
    glm::vec2 load_scrollMenuSize = glm::vec2(0.45f, 0.35f);
    float load_gap = 0.02f;
    int load_scrollMenuItems = 8;
    
    //==== styles initialization ====
    GUI::StyleMap styles = {};
    InitStyles_Main(styles, font, main_btnSize);
    InitStyles_Load(styles, font, load_scrollMenuSize, load_scrollMenuItems, load_scrollBtnSize, load_smallBtnSize);

    //==== sub-menu initializations ====
    InitSubmenu_Main(main_btnSize, main_menuSize, main_gap, styles["main"]);
    InitSubmenu_StartGame(main_btnSize, main_menuSize, main_gap, styles["main"]);
    InitSubmenu_LoadGame(load_btnSize, load_menuSize, load_scrollMenuSize, load_smallBtnSize, load_scrollBtnSize, load_gap, load_scrollMenuItems, styles);
    

    dbg_texture = styles["scroll_up"]->holdTexture;

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
    if(dbg_texture != nullptr) {
        ImGui::Begin("DBG_TEX");
        dbg_texture->DBG_GUI();
        ImGui::End();
    }
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
        case MainMenuState::LOAD_GAME:
            switch(keycode) {
                case GLFW_KEY_L: break;
                case GLFW_KEY_C: SwitchState(MainMenuState::START_GAME); break;
            }
            break;
    }
}

void MainMenuController::InitSubmenu_Main(const glm::vec2& buttonSize, const glm::vec2& menuSize, float gap, GUI::StyleRef style) {
    float off = -1.f + buttonSize.y / menuSize.y;           //button offset to start at the top of the menu
    float step = (2.f*buttonSize.y + gap) / menuSize.y;     //vertical step to move each button
    glm::vec2 bSize = buttonSize / menuSize;                //final button size - converted from size relative to parent

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
}

void MainMenuController::InitSubmenu_StartGame(const glm::vec2& buttonSize, const glm::vec2& menuSize, float gap, GUI::StyleRef style) {
    float off = -1.f + buttonSize.y / menuSize.y;           //button offset to start at the top of the menu
    float step = (2.f*buttonSize.y + gap) / menuSize.y;     //vertical step to move each button
    glm::vec2 bSize = buttonSize / menuSize;                //final button size - converted from size relative to parent

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
}

void MainMenuController::InitSubmenu_LoadGame(const glm::vec2& buttonSize, const glm::vec2& menuSize, const glm::vec2& scrollMenuSize, 
        const glm::vec2& smallButtonSize, float scrollButtonSize, float gap, int scrollMenuItems, GUI::StyleMap& styles) {
    float off = -1.f + buttonSize.y / menuSize.y;
    glm::vec2 bSize = buttonSize / menuSize;
    glm::vec2 smSize = scrollMenuSize / menuSize;
    glm::vec2 sbSize = smallButtonSize / menuSize;
    float step = (buttonSize.y + gap + scrollMenuSize.y) / menuSize.y;            //step for menu
    float smStep = step + (scrollMenuSize.y + gap + 2.f * buttonSize.y) / menuSize.y;      //step after the menu

    loadGame_menu = GUI::Menu(glm::vec2(0.f, 0.4f), menuSize, 0.f, {
        new GUI::TextLabel(glm::vec2(0.f, off), bSize, 1.f, styles["label"], "Load Game"),
        new GUI::ScrollMenu(glm::vec2(0.f, off+step), smSize, 1.f, scrollMenuItems, scrollButtonSize, { 
            styles["scroll_items"], styles["scroll_up"], styles["scroll_down"], styles["scroll_slider"], styles["scroll_grip"]
        }),
        new GUI::TextButton(glm::vec2(-0.4f, off+smStep), bSize, 1.f, styles["main"], "Load",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                //...
            }, 0
        ),
        new GUI::TextButton(glm::vec2(1.f - sbSize.x + 0.1f, off+smStep), sbSize, 1.f, styles["small_btn"], "Cancel", 
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                static_cast<MainMenuController*>(handler)->SwitchState(MainMenuState::START_GAME);
            }, 0
        ),
    });

    //TODO: dbg only, do this once the menu is opened - query saves folder for items
    GUI::ScrollMenu* scrollMenu = ((GUI::ScrollMenu*)loadGame_menu.GetChild(1));
    scrollMenu->UpdateContent({ "item_0", "item_1", "item_2", "item_3", "item_4", "item_5", "item_6", "item_7", "item_8", "item_9", "item_10", "item_11", "item_12", "item_13" });
}

//================================ Style init functions ================================

void InitStyles_Main(GUI::StyleMap& styles, const FontRef& font, const glm::vec2& buttonSize) {
    //===== main style =====

    //precompute sizes for main style's button textures
    glm::vec2 ts = glm::vec2(Window::Get().Size()) * buttonSize;
    float upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
    glm::ivec2 textureSize = ts * upscaleFactor;
    int borderWidth = 2 * upscaleFactor;

    //main style
    GUI::StyleRef s = styles["main"] = std::make_shared<GUI::Style>();
    s->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, false);
    s->hoverTexture = s->texture;
    s->holdTexture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, true);
    s->highlightTexture = TextureGenerator::ButtonHighlightTexture(textureSize.x, textureSize.y, borderWidth);
    s->textColor = textClr;
    s->font = font;
    s->holdOffset = glm::ivec2(borderWidth);       //= texture border width
}

void InitStyles_Load(GUI::StyleMap& styles, const FontRef& font, const glm::vec2& scrollMenuSize, int scrollMenuItems, float scrollButtonSize, const glm::vec2& smallBtnSize) {
    GUI::StyleRef s = nullptr;

    //text label style
    s = styles["label"] = std::make_shared<GUI::Style>();
    s->font = font;
    s->textColor = glm::vec4(1.f);
    s->color = glm::vec4(0.f);

    //menu item style
    glm::vec2 buttonSize = glm::vec2(scrollMenuSize.x, scrollMenuSize.y / scrollMenuItems);
    glm::vec2 ts = glm::vec2(Window::Get().Size()) * buttonSize;
    float upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
    glm::ivec2 textureSize = ts * upscaleFactor;
    int shadingWidth = 2 * upscaleFactor;

    s = styles["scroll_items"] = std::make_shared<GUI::Style>();
    s->font = font;
    s->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, 0, shadingWidth, 0, false);
    s->hoverTexture = s->texture;
    s->holdTexture = s->texture;
    s->highlightTexture = s->texture;
    s->textColor = textClr;
    s->textAlignment = GUI::TextAlignment::LEFT;
    s->textScale = 0.8f;
    // s->color = glm::vec4(0.f);

    //small button style
    buttonSize = smallBtnSize;
    ts = glm::vec2(Window::Get().Size()) * buttonSize;
    upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
    textureSize = ts * upscaleFactor;
    shadingWidth = 2 * upscaleFactor;

    s = styles["small_btn"] = std::make_shared<GUI::Style>();
    s->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, shadingWidth, shadingWidth, 0, false);
    s->hoverTexture = s->texture;
    s->holdTexture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, shadingWidth, shadingWidth, 0, true);
    s->highlightTexture = TextureGenerator::ButtonHighlightTexture(textureSize.x, textureSize.y, shadingWidth);
    s->textColor = textClr;
    s->font = font;
    s->holdOffset = glm::ivec2(shadingWidth);       //= texture border width

    //scroll up button style
    buttonSize = glm::vec2(scrollButtonSize, scrollButtonSize);
    ts = glm::vec2(Window::Get().Size()) * buttonSize;
    upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
    textureSize = ts * upscaleFactor;
    shadingWidth = 2 * upscaleFactor;
    s = styles["scroll_up"] = std::make_shared<GUI::Style>();
    s->texture = TextureGenerator::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, false, true);
    s->hoverTexture = s->texture;
    s->holdTexture = TextureGenerator::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, true, true);
    s->highlightEnabled = false;

    s = styles["scroll_down"] = std::make_shared<GUI::Style>();
    s->texture = TextureGenerator::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, false, false);
    s->hoverTexture = s->texture;
    s->holdTexture = TextureGenerator::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, true, false);
    s->highlightEnabled = false;

    s = styles["scroll_grip"] = std::make_shared<GUI::Style>();
    s->texture = TextureGenerator::ButtonTexture_Gem(textureSize.x, textureSize.y, shadingWidth, false);
    s->hoverTexture = s->texture;
    s->holdTexture = TextureGenerator::ButtonTexture_Gem(textureSize.x, textureSize.y, shadingWidth, true);
    s->highlightEnabled = false;


    //scroll slider style
    buttonSize = glm::vec2(scrollButtonSize, scrollMenuSize.y);
    ts = glm::vec2(Window::Get().Size()) * buttonSize;
    upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
    textureSize = ts * upscaleFactor;
    shadingWidth = 2 * upscaleFactor;
    s = styles["scroll_slider"] = std::make_shared<GUI::Style>();
    s->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, shadingWidth, 0, 0, false);
    s->hoverTexture = s->texture;
    s->holdTexture = s->texture;
    s->highlightEnabled = false;
}