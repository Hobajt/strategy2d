#include "menu.h"

#include "intro.h"
#include "ingame.h"
#include "recap.h"

#include "engine/game/config.h"

using namespace eng;

void InitStyles_Main(GUI::StyleMap& styles, const FontRef& font, const glm::vec2& buttonSize);
void InitStyles_Single_Load(GUI::StyleMap& styles, const FontRef& font, const glm::vec2& scrollMenuSize, int scrollMenuItems, float scrollButtonSize, const glm::vec2& smallBtnSize);

static glm::vec4 textClr = glm::vec4(0.92f, 0.77f, 0.20f, 1.f);

//===== MainMenuController =====

MainMenuController::MainMenuController() : backgroundTexture(Resources::LoadTexture("TitleMenu_BNE.png")) {
    FontRef font = Resources::DefaultFont();

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
    InitStyles_Single_Load(styles, font, load_scrollMenuSize, load_scrollMenuItems, load_scrollBtnSize, load_smallBtnSize);

    //==== sub-menu initializations ====
    InitSubmenu_Main(main_btnSize, main_menuSize, main_gap, styles["main"]);
    InitSubmenu_Single(main_btnSize, main_menuSize, main_gap, styles["main"]);
    InitSubmenu_Single_Load(load_btnSize, load_menuSize, load_scrollMenuSize, load_smallBtnSize, load_scrollBtnSize, load_gap, load_scrollMenuItems, styles);
    InitSubmenu_Single_Campaign(main_btnSize, main_menuSize, main_gap, styles);
    InitSubmenu_Single_Custom(main_btnSize, main_menuSize, main_gap, styles);

    dbg_texture = styles["scroll_grip"]->texture;

    SwitchState(MainMenuState::MAIN);
}

void MainMenuController::Update() {
    ASSERT_MSG(activeMenu != nullptr, "There should always be active submenu in the main menu.");
    selection.Update(activeMenu);
}

void MainMenuController::Render() {
    ASSERT_MSG(activeMenu != nullptr, "There should always be active submenu in the main menu.");
    Renderer::RenderQuad(Quad::FromCenter(glm::vec3(0.f), glm::vec3(1.f), glm::vec4(1.f), backgroundTexture));
    activeMenu->Render();
}

void MainMenuController::SwitchState(int newState) {
    if(menu.count(newState) == 0) {
        LOG_WARN("MainMenu - Unrecognized menu state ({}), switching to main submenu.", newState);
        newState = MainMenuState::MAIN;
    }

    activeMenu = &menu[newState];
    activeState = newState;

    switch(newState) {
        case MainMenuState::SINGLE_CUSTOM:
            mapSelection->UpdateContent(Config::Saves::ScanCustomGames(), true);
            break;
        case MainMenuState::SINGLE_LOAD:
            ((GUI::ScrollMenu*)menu[MainMenuState::SINGLE_LOAD].GetChild(1))->UpdateContent(Config::Saves::Scan(), true);
            break;
    }
}

void MainMenuController::StartCampaign(bool asOrc) {
    gameInitParams = {};
    gameInitParams.race = GameParams::Race::HUMAN + int(asOrc);
    gameInitParams.campaignIdx = 0;
    
    GetTransitionHandler()->InitTransition(
        TransitionParameters(TransitionDuration::MID, TransitionType::FADE_OUT, GameStageName::RECAP, RecapState::OBJECTIVES, (void*)&gameInitParams, true)
    );
}

void MainMenuController::StartCustomGame(bool asOrc, int opponents, const std::string& mapfile) {
    gameInitParams = {};
    gameInitParams.race = GameParams::Race::HUMAN + int(asOrc);
    gameInitParams.filepath = mapfile;
    gameInitParams.opponents = opponents;

    GetTransitionHandler()->InitTransition(
        TransitionParameters(TransitionDuration::MID, TransitionType::FADE_OUT, GameStageName::INGAME, GameStartType::CUSTOM, (void*)&gameInitParams, true)
    );
}

void MainMenuController::LoadGame() {
    GUI::ScrollMenu* scrollMenu = ((GUI::ScrollMenu*)menu[MainMenuState::SINGLE_LOAD].GetChild(1));
    if(scrollMenu->ItemCount() <= 0) {
        LOG_WARN("MainMenuController::LoadGame - invalid item, cannot load the game.");
        return;
    }
    std::string filename = scrollMenu->CurrentSelection();

    gameInitParams = {};
    gameInitParams.filepath = Config::Saves::FullPath(filename);

    GetTransitionHandler()->InitTransition(
        TransitionParameters(TransitionDuration::MID, TransitionType::FADE_OUT, GameStageName::INGAME, GameStartType::LOAD, (void*)&gameInitParams, true)
    );
}

void MainMenuController::OnPreStart(int prevStageID, int info, void* data) {
    LOG_INFO("GameStage = MainMenu");
    if(info != -1) {
        SwitchState(info);
    }
}

void MainMenuController::OnStart(int prevStageID, int info, void* data) {
    Input::Get().AddKeyCallback(-1, [](int keycode, int modifiers, bool single_press, void* handler){
        if(single_press) {
            static_cast<MainMenuController*>(handler)->KeyPressCallback(keycode, modifiers);
        }
    }, true, this);
}

void MainMenuController::OnStop(int nextStageID) {

}

void MainMenuController::DBG_GUI(bool active) {
#ifdef ENGINE_ENABLE_GUI
    if(dbg_texture != nullptr) {
        ImGui::Begin("DBG_TEX");
        dbg_texture->DBG_GUI();
        ImGui::End();
    }
#endif
}

#ifdef ENGINE_DEBUG
void MainMenuController::DBG_StageSwitch(int stateIdx) {
    SwitchState(stateIdx);
}
#endif

void MainMenuController::KeyPressCallback(int keycode, int modifiers) {
    switch(activeState) {
        case MainMenuState::MAIN:
            switch(keycode) {
                case GLFW_KEY_S: SwitchState(MainMenuState::SINGLE); break;
                case GLFW_KEY_M: break;
                case GLFW_KEY_R: break;
                case GLFW_KEY_C: break;
                case GLFW_KEY_X: Window::Get().Close(); break;
            }
            break;
        case MainMenuState::SINGLE:
            switch(keycode) {
                case GLFW_KEY_N: break;
                case GLFW_KEY_L: SwitchState(MainMenuState::SINGLE_LOAD); break;
                case GLFW_KEY_C: break;
                case GLFW_KEY_P: SwitchState(MainMenuState::MAIN); break;
            }
            break;
        case MainMenuState::SINGLE_LOAD:
            switch(keycode) {
                case GLFW_KEY_L: break;
                case GLFW_KEY_C: SwitchState(MainMenuState::SINGLE); break;
            }
            break;
    }
}

void MainMenuController::InitSubmenu_Main(const glm::vec2& buttonSize, const glm::vec2& menuSize, float gap, GUI::StyleRef style) {
    float off = -1.f + buttonSize.y / menuSize.y;           //button offset to start at the top of the menu
    float step = (2.f*buttonSize.y + gap) / menuSize.y;     //vertical step to move each button
    glm::vec2 bSize = buttonSize / menuSize;                //final button size - converted from size relative to parent

    menu[MainMenuState::MAIN] = GUI::Menu(glm::vec2(0.f, 0.5f), menuSize, 0.f, std::vector<GUI::Element*>{
        new GUI::TextButton(glm::vec2(0.f, off+step*0), bSize, 1.f, style, "Single Player Game",
            this, [](GUI::ButtonCallbackHandler* handler, int id) { 
                static_cast<MainMenuController*>(handler)->SwitchState(MainMenuState::SINGLE);
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*1), bSize, 1.f, style, "Multi Player Game",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                //...
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*2), bSize, 1.f, style, "Replay Introduction",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                static_cast<MainMenuController*>(handler)->GetTransitionHandler()->InitTransition(
                    TransitionParameters(TransitionDuration::MID, TransitionType::FADE_OUT, GameStageName::INTRO, IntroState::CINEMATIC_REPLAY, true)
                );
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

void MainMenuController::InitSubmenu_Single(const glm::vec2& buttonSize, const glm::vec2& menuSize, float gap, GUI::StyleRef style) {
    float off = -1.f + buttonSize.y / menuSize.y;           //button offset to start at the top of the menu
    float step = (2.f*buttonSize.y + gap) / menuSize.y;     //vertical step to move each button
    glm::vec2 bSize = buttonSize / menuSize;                //final button size - converted from size relative to parent

    menu[MainMenuState::SINGLE] = GUI::Menu(glm::vec2(0.f, 0.5f), menuSize, 0.f, std::vector<GUI::Element*>{
        new GUI::TextButton(glm::vec2(0.f, off+step*0), bSize, 1.f, style, "New Campaign",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                static_cast<MainMenuController*>(handler)->SwitchState(MainMenuState::SINGLE_CAMPAIGN);
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*1), bSize, 1.f, style, "Load Game",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                static_cast<MainMenuController*>(handler)->SwitchState(MainMenuState::SINGLE_LOAD);
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*2), bSize, 1.f, style, "Custom Scenario",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                static_cast<MainMenuController*>(handler)->SwitchState(MainMenuState::SINGLE_CUSTOM);
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*3), bSize, 1.f, style, "Previous Menu",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                static_cast<MainMenuController*>(handler)->SwitchState(MainMenuState::MAIN);
            }, 0
        ),
    });
}

void MainMenuController::InitSubmenu_Single_Load(const glm::vec2& buttonSize, const glm::vec2& menuSize, const glm::vec2& scrollMenuSize, 
        const glm::vec2& smallButtonSize, float scrollButtonSize, float gap, int scrollMenuItems, GUI::StyleMap& styles) {
    float off = -1.f + buttonSize.y / menuSize.y;
    glm::vec2 bSize = buttonSize / menuSize;
    glm::vec2 smSize = scrollMenuSize / menuSize;
    glm::vec2 sbSize = smallButtonSize / menuSize;
    float step = (buttonSize.y + gap + scrollMenuSize.y) / menuSize.y;            //step for menu
    float smStep = step + (scrollMenuSize.y + gap + 2.f * buttonSize.y) / menuSize.y;      //step after the menu

    menu[MainMenuState::SINGLE_LOAD] = GUI::Menu(glm::vec2(0.f, 0.4f), menuSize, 0.f, {
        new GUI::TextLabel(glm::vec2(0.f, off), bSize, 1.f, styles["label"], "Load Game"),
        new GUI::ScrollMenu(glm::vec2(0.f, off+step), smSize, 1.f, scrollMenuItems, scrollButtonSize, { 
            styles["scroll_items"], styles["scroll_up"], styles["scroll_down"], styles["scroll_slider"], styles["scroll_grip"]
        }),
        new GUI::TextButton(glm::vec2(-0.4f, off+smStep), bSize, 1.f, styles["main"], "Load",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                static_cast<MainMenuController*>(handler)->LoadGame();
            }, 0
        ),
        new GUI::TextButton(glm::vec2(1.f - sbSize.x + 0.1f, off+smStep), sbSize, 1.f, styles["small_btn"], "Cancel", 
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                static_cast<MainMenuController*>(handler)->SwitchState(MainMenuState::SINGLE);
            }, 0
        ),
    });
}

void MainMenuController::InitSubmenu_Single_Campaign(const glm::vec2& buttonSize, const glm::vec2& menuSize, float gap, eng::GUI::StyleMap& styles) {
    float off = -1.f + buttonSize.y / menuSize.y;           //button offset to start at the top of the menu
    float step = (2.f*buttonSize.y + gap) / menuSize.y;     //vertical step to move each button
    glm::vec2 bSize = buttonSize / menuSize;                //final button size - converted from size relative to parent

    menu[MainMenuState::SINGLE_CAMPAIGN] = GUI::Menu(glm::vec2(0.f, 0.5f), menuSize, 0.f, std::vector<GUI::Element*>{
        new GUI::TextLabel(glm::vec2(0.f, off+step*-1), bSize, 1.f, styles["label"], "Tides of Darkness"),
        new GUI::TextButton(glm::vec2(0.f, off+step*0), bSize, 1.f, styles["main"], "Orc Campaign",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                static_cast<MainMenuController*>(handler)->StartCampaign(true);
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*1), bSize, 1.f, styles["main"], "Human Campaign",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                static_cast<MainMenuController*>(handler)->StartCampaign(false);
            }, 0
        ),
        new GUI::TextButton(glm::vec2(0.f, off+step*2.5f), bSize, 1.f, styles["main"], "Previous Menu",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                static_cast<MainMenuController*>(handler)->SwitchState(MainMenuState::SINGLE);
            }, 0
        ),
    });
}

void MainMenuController::InitSubmenu_Single_Custom(const glm::vec2& buttonSize, const glm::vec2& menuSize, float gap, eng::GUI::StyleMap& styles) {
    float off = -1.f + buttonSize.y / menuSize.y;           //button offset to start at the top of the menu
    float step = (2.f*buttonSize.y + gap) / menuSize.y;     //vertical step to move each button
    glm::vec2 bSize = buttonSize / menuSize;                //final button size - converted from size relative to parent

    menu[MainMenuState::SINGLE_CUSTOM] = GUI::Menu(glm::vec2(0.f, 0.5f), menuSize, 0.f, std::vector<GUI::Element*>{
        new GUI::ScrollMenu(glm::vec2(0.7f, -0.3f), glm::vec2(0.7f, 0.6f), 2.f, 8, 1.f / 8.f, { styles["scroll_items"], styles["scroll_up"], styles["scroll_down"], styles["scroll_slider"], styles["scroll_grip"] }),
        new GUI::SelectMenu(glm::vec2(-0.8f, off+step*0.5f), bSize, 1.f, styles["scroll_items"], std::vector<std::string>{ "Human", "Orc", "Random" }, 2),
        new GUI::SelectMenu(glm::vec2(-0.8f, off+step*2.0f), bSize, 1.f, styles["scroll_items"], std::vector<std::string>{
            "Map Default", "1 Opponent", "2 Opponents", "3 Opponents", "4 Opponents", "5 Opponents", "6 Opponents", "7 Opponents"
        }, 0),

        new GUI::TextLabel(glm::vec2(0.f, off-step*1.f), bSize, 1.f, styles["label"], "Custom game setup"),
        new GUI::TextLabel(glm::vec2(-0.8f, off+step*0.f), bSize, 1.f, styles["label"], "Your Race:"),
        new GUI::TextLabel(glm::vec2(-0.8f, off+step*1.5f), bSize, 1.f, styles["label"], "Opponents:"),

        new GUI::TextButton(glm::vec2(1.f, 0.8f-step*1.f), bSize, 1.f, styles["main"], "Start Game",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                MainMenuController* menu = static_cast<MainMenuController*>(handler);

                int race = ((GUI::SelectMenu*)menu->menu.at(MainMenuState::SINGLE_CUSTOM).GetChild(1))->CurrentSelection();
                if(race > 1) race = Random::UniformInt(1);

                int opponents = ((GUI::SelectMenu*)menu->menu.at(MainMenuState::SINGLE_CUSTOM).GetChild(2))->CurrentSelection();

                std::string mapname = menu->mapSelection->CurrentSelection();

                menu->StartCustomGame(race, opponents, Config::Saves::CustomGames_FullPath(mapname));
            }, 0
        ),
        new GUI::TextButton(glm::vec2(1.f, 0.8f), bSize, 1.f, styles["main"], "Previous Menu",
            this, [](GUI::ButtonCallbackHandler* handler, int id) {
                static_cast<MainMenuController*>(handler)->SwitchState(MainMenuState::SINGLE);
            }, 0
        ),
    });

    mapSelection = (GUI::ScrollMenu*)menu.at(MainMenuState::SINGLE_CUSTOM).GetChild(0);
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
    s->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, false));
    s->hoverTexture = s->texture;
    s->holdTexture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, true));
    s->highlightTexture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonHighlightTexture(textureSize.x, textureSize.y, borderWidth));
    s->textColor = textClr;
    s->font = font;
    s->holdOffset = glm::ivec2(borderWidth);       //= texture border width
}

void InitStyles_Single_Load(GUI::StyleMap& styles, const FontRef& font, const glm::vec2& scrollMenuSize, int scrollMenuItems, float scrollButtonSize, const glm::vec2& smallBtnSize) {
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
    s->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, 0, shadingWidth, 0, false));
    s->hoverTexture = s->texture;
    s->holdTexture = s->texture;
    s->highlightTexture = s->texture;
    s->highlightMode = GUI::HighlightMode::TEXT;
    s->textColor = textClr;
    s->textAlignment = GUI::TextAlignment::LEFT;
    s->textScale = 0.8f;
    s->hoverColor = textClr;
    // s->color = glm::vec4(0.f);

    //small button style
    buttonSize = smallBtnSize;
    ts = glm::vec2(Window::Get().Size()) * buttonSize;
    upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
    textureSize = ts * upscaleFactor;
    shadingWidth = 2 * upscaleFactor;

    s = styles["small_btn"] = std::make_shared<GUI::Style>();
    s->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, shadingWidth, shadingWidth, 0, false));
    s->hoverTexture = s->texture;
    s->holdTexture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, shadingWidth, shadingWidth, 0, true));
    s->highlightTexture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonHighlightTexture(textureSize.x, textureSize.y, shadingWidth));
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
    s->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, false, true));
    s->hoverTexture = s->texture;
    s->holdTexture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, true, true));
    s->highlightMode = GUI::HighlightMode::NONE;

    s = styles["scroll_down"] = std::make_shared<GUI::Style>();
    s->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, false, false));
    s->hoverTexture = s->texture;
    s->holdTexture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, true, false));
    s->highlightMode = GUI::HighlightMode::NONE;

    s = styles["scroll_grip"] = std::make_shared<GUI::Style>();
    s->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Gem(textureSize.x, textureSize.y, shadingWidth, Window::Get().Ratio()));
    s->hoverTexture = s->texture;
    s->holdTexture = s->texture;
    s->highlightMode = GUI::HighlightMode::NONE;


    //scroll slider style
    buttonSize = glm::vec2(scrollButtonSize, scrollMenuSize.y);
    ts = glm::vec2(Window::Get().Size()) * buttonSize;
    upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
    textureSize = ts * upscaleFactor;
    shadingWidth = 2 * upscaleFactor;
    s = styles["scroll_slider"] = std::make_shared<GUI::Style>();
    s->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, shadingWidth, 0, 0, false));
    s->hoverTexture = s->texture;
    s->holdTexture = s->texture;
    s->highlightMode = GUI::HighlightMode::NONE;
}