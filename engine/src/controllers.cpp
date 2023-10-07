#include "engine/game/controllers.h"

#include "engine/core/input.h"
#include "engine/core/renderer.h"
#include "engine/utils/generator.h"
#include "engine/game/resources.h"

namespace eng {

    constexpr float GUI_WIDTH = 0.25f;
    constexpr float BORDER_SIZE = 0.025f;

    void RenderGUIBorders(bool isOrc, float z);

    // PlayerFactionController::PlayerGUI::PlayerGUI() {
    //     //map, action buttons, selection icons

        


    //     /*how to manage button callbacks:
    //         - first of all, don't have to make this class as callback handler - can use faction controller directly
    //         - action buttons:
    //             - they just invoke commands (IssueCommand()) on current selection
    //             - only need to track button state (when selecting command target)
    //         - selection icons:
    //             - only change selection (select 1 unit from selected group)
    //         - menu buttons:
    //             - one pauses the game, the other also invokes game menu
    //             - haven't really thought through how to handle game pauses, nor ingame menu for that manner

    //         - game menu could just be located within the player controller & completely managed by it
    //         - there's still the need to propagate the choices from it to the game stage controller (ingame stage)
    //         - also need to communicate the request for game pause to the controller
    //     */
    // }

    

    // void PlayerFactionController::PlayerGUI::Render() {
        
        
    //     //render the GUI elements in the panel
    //     panel.Render();


    //     //validate selection existance again


    //     //render green boxes around currently selected objects
    //     //maybe render cursor from here as well (i think it changes based on current logic of the click)
    // }

    // void PlayerFactionController::PlayerGUI::Update() {
    //     /* process inputs - mouse clicks & hotkey presses
    //         do command dispatch
    //         take into consideration current selection (hotkeys & mouse logic is determined by that)
    //         also take into consideration button state (ie. if a command is currently clicked)
    //     */
    //     Input& input = Input::Get();

    //     // handler.Update(panel);

    //     // ENG_LOG_INFO("({}, {}) ({}, {})", input.mousePos.x, input.mousePos.y, input.mousePos_n.x, input.mousePos_n.y);

    //     /* selection update
    //         - toss out objects that no longer exist
    //         - identify selection type (commandable, group/individual, ...)
    //         - cancel selected command if selection updates (unit dies for example)
    //     */

    //     // selected_count = 0;
    //     // for(int i = 0; i < (int)selection.size(); i++) {
            
    //     //     selected_count++;
    //     // }

    //     // switch(gui_state) {
    //     //     case PlayerGUIState::NONE:
    //     //         break;
    //     //     case PlayerGUIState::
    //     // }

    //     // if(input.mousePos_n.x < GUI_WIDTH) {
    //     //     ENG_LOG_INFO("IN GUI");
    //     //     //mouse pos in GUI

    //     //     //LMB   -> button clicks
    //     //     //RMB   -> only when clicked in map (issue movement for the selection)
    //     //     //hover -> write out description in the chat prompt
    //     //     //cursor is always default (hand)
    //     // }
    //     // else {
    //         //mouse hovering over the game

    //         /* if command selected:
    //                 LMB -> issue command
    //                 RMB -> cancel command
    //                 hotkeys -> only ESC (cancel command), others ignored
    //            else:
    //                 LMB -> select object
    //                 RMB -> issue adaptive command
    //                 hotkeys -> 


    //             edge conditions:
    //                 - check that there is selection when issuing commands
    //                 - validate selection properly - player owned objects
    //         */

    //         /* cursor visuals:
    //             - switching conditions - what is being hovered over, is command selected
    //             - command selected
    //                 - render eagle
    //                 - green if hovering over object, yellow otherwise
    //             - no command:   
    //                 - render magnifying glass if hovering over object, or hand otherwise
    //         */
    //     // }

    //     if(input.lmb.down()) {

    //     }
    //     else if (input.rmb.down()) {

    //     }
    //     else {
    //         //hotkeys dispatch
    //     }
    // }

    //===== GUI::SelectionTab =====

    GUI::SelectionTab::SelectionTab(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, 
        const StyleRef& borders_style_, const StyleRef& text_style, const StyleRef& text_style_small,
        const StyleRef& btn_style_, const StyleRef& bar_style_, const glm::vec2& bar_borders_size_, const Sprite& sprite_, ButtonCallbackHandler* handler_, ButtonCallbackType callback_)
        : Element(offset_, size_, zOffset_, Style::Default(), nullptr) {
        
        // icon button grid, when there's multiple objects selected
        btns = static_cast<ImageButtonGrid*>(AddChild(new ImageButtonGrid(glm::vec2(0.f, 0.f), glm::vec2(1.f, 1.f), 0.f, btn_style_, bar_style_, bar_borders_size_, sprite_, 3, 3, glm::vec2(0.975f / 3.f, 0.9f / 3.f), handler_, callback_), true));
        

        //unit name & current level fields; next to the image
        name = static_cast<TextLabel*>(AddChild(new TextLabel(glm::vec2(0.333f, -0.8f), glm::vec2(0.666f, 0.1f), 2.f, text_style, "Unit Name"), true));
        level = static_cast<TextLabel*>(AddChild(new TextLabel(glm::vec2(0.333f, -0.6f), glm::vec2(0.666f, 0.1f), 2.f, text_style, "Level X"), true));

        //text with health value (curr/max); directly underneath the health bar
        health = static_cast<TextLabel*>(AddChild(new TextLabel(glm::vec2(-0.666f, -0.3f), glm::vec2(0.333f, 0.05f), 2.f, text_style_small, "xxx/yyy"), true));

        //headline for the stats ("Production" or "Food Usage" written above the actual stats)
        headline = static_cast<TextLabel*>(AddChild(new TextLabel(glm::vec2(-0.05f, -0.1f), glm::vec2(0.8f, 0.05f), 2.f, text_style, "Stats headline", false), true));

        // //stats fields
        // for(size_t i = 0; i < stats.size(); i++) {
        //     stats[i] = static_cast<KeyValue*>(AddChild(new KeyValue(...), true));
        // }

        // //mana bar
        // mana_bar = static_cast<ValueBar*>(AddChild(new ValueBar(...), true));

        // //production icon
        // production_icon = static_cast<ImageButton*>(AddChild(new ImageButton(...), true));

        // //production bar
        // production_bar = static_cast<ValueBar*>(AddChild(new ValueBar(...), true));


        /*types of data to display:
            - unit info - when single unit is selected (stats, level, etc)
            - selection group - unit icons, when multiple units are selected
            - production info - for town hall, refinery, lumber mill
            - food usage - for farms
            - production progress - whenever building is researching, upgrading or producing a unit

          regarding the text alignment:
            - it'd probably be easier if I've made align right functionality as well
            - could split the text into two objects - label and value
        */

        //TODO: GUI ELEMENTS TO CREATE - ValueBar (for mana & progress tracking), KeyValueLabel (for various entries in the GUI), 

        

        // //armor, damage, range, sight, speed, magic (if unit has it)
        // text_fields.push_back(AddChild(new TextLabel(glm::vec2(), glm::vec2(), 1.f, text_style, "Armor: X", false), true));
        // text_fields.push_back(AddChild(new TextLabel(glm::vec2(), glm::vec2(), 1.f, text_style, "Damage: X", false), true));
        // text_fields.push_back(AddChild(new TextLabel(glm::vec2(), glm::vec2(), 1.f, text_style, "Range: X", false), true));
        // text_fields.push_back(AddChild(new TextLabel(glm::vec2(), glm::vec2(), 1.f, text_style, "Sight: X", false), true));
        // text_fields.push_back(AddChild(new TextLabel(glm::vec2(), glm::vec2(), 1.f, text_style, "Speed: X", false), true));
        // text_fields.push_back(AddChild(new TextLabel(glm::vec2(), glm::vec2(), 1.f, text_style, "Magic:", false), true));
        // mana_bar = new ManaBar(glm::vec2(), glm::vec2(), 1.f, text_style, bar_style_, bar_borders_size_, color);

        // //render labels without centering, so that changing digits doesn't fuck it all up


        //frame borders around the entire area
        borders = AddChild(new Menu(glm::vec2(0.f, 0.f), glm::vec2(1.f, 1.f), 0.5f, borders_style_, {}), true);
    }

    void GUI::SelectionTab::Update(const Level& level, const PlayerSelection& selection) {
        //TODO:
        //pass in current selection data
        //if single unit selected -> disable all but one icons, enable & update the text
        //else hide the text & display all icons (+ update icon indices)

        //also update health bar values
    }

    //===== PlayerFactionController =====

    static glm::vec4 textClr = glm::vec4(0.92f, 0.77f, 0.20f, 1.f);

    PlayerFactionController::PlayerFactionController(FactionsFile::FactionEntry&& entry) : FactionController(std::move(entry)), gui_handler({}) {
        FontRef font = Resources::DefaultFont();
        glm::vec2 buttonSize = glm::vec2(0.33f, 0.06f);

        //precompute sizes for main style's button textures
        glm::vec2 ts = glm::vec2(Window::Get().Size()) * buttonSize;
        float upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
        glm::ivec2 textureSize = ts * upscaleFactor;
        int borderWidth = 2 * upscaleFactor;

        //general style for menu buttons
        GUI::StyleRef menu_btn_style = std::make_shared<GUI::Style>();
        menu_btn_style->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, false);
        menu_btn_style->hoverTexture = menu_btn_style->texture;
        menu_btn_style->holdTexture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, true);
        menu_btn_style->highlightTexture = TextureGenerator::ButtonHighlightTexture(textureSize.x, textureSize.y, borderWidth);
        menu_btn_style->textColor = textClr;
        menu_btn_style->font = font;
        menu_btn_style->holdOffset = glm::ivec2(borderWidth);       //= texture border width

        //style for the menu background
        GUI::StyleRef menu_style = std::make_shared<GUI::Style>();
        menu_style->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, false);
        menu_style->hoverTexture = menu_style->texture;
        menu_style->holdTexture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, true);
        menu_style->highlightTexture = TextureGenerator::ButtonHighlightTexture(textureSize.x, textureSize.y, borderWidth);
        menu_style->textColor = textClr;
        menu_style->font = font;
        menu_style->holdOffset = glm::ivec2(borderWidth);       //= texture border width

        //style for the menu background
        GUI::StyleRef text_style = std::make_shared<GUI::Style>();
        text_style->textColor = textClr;
        text_style->font = font;
        text_style->color = glm::vec4(0.f);

        GUI::StyleRef text_style_small = std::make_shared<GUI::Style>();
        text_style_small->textColor = textClr;
        text_style_small->font = font;
        text_style_small->textScale = 0.5f;
        text_style_small->color = glm::vec4(0.f);

        //dbg only - to visualize the element size
        text_style->color = glm::vec4(1.f);
        text_style_small->color = glm::vec4(1.f);
        text_style->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, false);
        text_style_small->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, false);

        buttonSize = glm::vec2(0.25f, 0.25f);
        ts = glm::vec2(Window::Get().Size()) * buttonSize;
        upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
        textureSize = ts * upscaleFactor;
        borderWidth = 7 * upscaleFactor;

        //style for icon buttons
        GUI::StyleRef icon_btn_style = std::make_shared<GUI::Style>();
        icon_btn_style->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, glm::uvec3(200), glm::uvec3(20), glm::uvec3(240), glm::uvec3(125));
        icon_btn_style->hoverTexture = TextureGenerator::ButtonTexture_Clear_2borders(textureSize.x, textureSize.y, borderWidth, borderWidth, glm::uvec3(200), glm::uvec3(20), glm::uvec3(240), glm::uvec3(125), glm::uvec3(125), borderWidth / 2);
        icon_btn_style->holdTexture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, glm::uvec3(170), glm::uvec3(20), glm::uvec3(170), glm::uvec3(110));
        icon_btn_style->highlightTexture = TextureGenerator::ButtonHighlightTexture(textureSize.x, textureSize.y, borderWidth / 2, glm::u8vec4(66, 245, 84, 255));
        icon_btn_style->textColor = textClr;
        icon_btn_style->font = font;
        icon_btn_style->holdOffset = glm::ivec2(borderWidth);

        buttonSize = glm::vec2(0.25f, 0.25f);
        ts = glm::vec2(Window::Get().Size()) * buttonSize;
        upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
        textureSize = ts * upscaleFactor;
        borderWidth = 3 * upscaleFactor;
        GUI::StyleRef borders_style = std::make_shared<GUI::Style>();
        borders_style->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, glm::u8vec4(0,0,0,0), glm::u8vec4(20,20,20,255), glm::u8vec4(240,240,240,255), glm::u8vec4(125,125,125,255));

        //style for icon buttons bar
        buttonSize = glm::vec2(0.25f, 0.25f * GUI::ImageButtonWithBar::BarHeightRatio());
        ts = glm::vec2(Window::Get().Size()) * buttonSize;
        upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
        textureSize = ts * upscaleFactor;
        borderWidth = 7 * upscaleFactor;
        glm::vec2 bar_border_size = 7.f / ts;
        ENG_LOG_TRACE("({}, {}), ({}, {}), {}", ts.x, ts.y, bar_border_size.x, bar_border_size.y, upscaleFactor);

        GUI::StyleRef bar_style = std::make_shared<GUI::Style>();
        bar_style->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, glm::uvec3(70), glm::uvec3(20), glm::uvec3(70), glm::uvec3(70));

        SpritesheetRef icons = Resources::LoadSpritesheet("misc/icons");
        Sprite icon = (*icons)("icon");

        actionButtons = new GUI::ImageButtonGrid(glm::vec2(0.5f, 0.675f), glm::vec2(0.475f, 0.3f), 1.f, icon_btn_style, icon, 3, 3, glm::vec2(0.975f / 3.f), this, [](GUI::ButtonCallbackHandler* handler, int id){
            ENG_LOG_TRACE("ActionButtons - Button [{}, {}] clicked", id % 3, id / 3);
        });

        selectionTab = new GUI::SelectionTab(glm::vec2(0.5f, 0.f), glm::vec2(0.475f, 0.35f), 1.f, 
            borders_style, text_style, text_style_small,
            icon_btn_style, bar_style, bar_border_size, icon, this, [](GUI::ButtonCallbackHandler* handler, int id){
            ENG_LOG_TRACE("SelectionTab - Button [{}, {}] clicked", id % 3, id / 3);
        });

        game_panel = GUI::Menu(glm::vec2(-1.f, 0.f), glm::vec2(GUI_WIDTH*2.f, 1.f), 0.f, std::vector<GUI::Element*>{
            // new GUI::Map(),             //map view
            new GUI::TextButton(glm::vec2(0.25f, -0.95f), glm::vec2(0.2f, 0.03f), 1.f, menu_btn_style, "Menu", this, [](GUI::ButtonCallbackHandler* handler, int id){
                static_cast<PlayerFactionController*>(handler)->SwitchMenu(true);
            }),
            new GUI::TextButton(glm::vec2(0.75f, -0.95f), glm::vec2(0.2f, 0.03f), 1.f, menu_btn_style, "Pause", this, [](GUI::ButtonCallbackHandler* handler, int id){
                // static_cast<PlayerFactionController*>(handler)->handler->PauseToggleRequest();
            }),

            actionButtons,
            selectionTab,

            // new GUI::ImageButtonWithBar(glm::vec2(0.5f, 0.f), glm::vec2(0.25f * Window::Get().Ratio(), 0.25f), 1.f, icon_btn_style, bar_style, bar_border_size, icon, glm::ivec2(0,0), 0.9f, this, [](GUI::ButtonCallbackHandler* handler, int id){}),
            // new GUI::Button(glm::vec2(0.5f, 0.6f), glm::vec2(0.25f * Window::Get().Ratio(), 0.25f), 1.f, icon_btn_style, this, [](GUI::ButtonCallbackHandler* handler, int id){}),

            // //temporary, test for ImageButtons logic
            // , new GUI::ImageButton(glm::vec2(0.5f, 0.f), glm::vec2(0.25f * Window::Get().Ratio(), 0.25f), 1.f, icon_btn_style, icon, glm::ivec2(0,0), 0.9f, this, [](GUI::ButtonCallbackHandler* handler, int id){})
            // , new GUI::Button(glm::vec2(0.5f, 0.6f), glm::vec2(0.25f * Window::Get().Ratio(), 0.25f), 1.f, icon_btn_style, this, [](GUI::ButtonCallbackHandler* handler, int id){})
        });

        //testing only
        // actionButtons->GetButton(2)->Enable(false);
        // actionButtons->GetButton(4)->Enable(false);
        // actionButtons->GetButton(6)->Enable(false);

        float b = BORDER_SIZE * Window::Get().Ratio();
        float xOff = 0.01f;
        text_prompt = GUI::TextLabel(glm::vec2(-1.f + GUI_WIDTH*2.f + xOff + (1.f - GUI_WIDTH), 1.f - b), glm::vec2(1.f - GUI_WIDTH - xOff, b*0.9f), 1.f, text_style, "TEXT PROMPT", false);

        //TODO: might want to wrap menu into a custom class, since there're going to be more panels than one
        menu_panel = GUI::Menu(glm::vec2(GUI_WIDTH, 0.f), glm::vec2(1.5f * GUI_WIDTH, 0.666f), 0.f, menu_style, std::vector<GUI::Element*>{
            //"Game Menu" label
            //Save & Load buttons (on the same line)
            //options
            //help
            //scenario objectives
            //end scenario
            //return to game
        });


        /*TODO next time:
            - work on game pause logic
            - maybe move camera controls in here - needs to get frozen along with the game
            - work out the gui dispatch logic
                - have a gui_state variable - will cover various actions & whether the menu is opened or not
                - have a switch for it
                    - if menu opened -> only dispatch within the menu
                    - if 

            - work out the object selection handling
        */


        /* element impl:
            - action buttons:
                - will probably be custom GUI element
                - ideally, there'd be only one GUI style for all the buttons - will probably have to write a modified Button class for that tho (IconButton?)
                - the single style would only define button borders & on hover/click visuals
                - buttons will have to be updated whenever selection changes
                    - could copy the button descriptions over for selected object (they should be lightweight)
                    - can retrieve a pointer to the ActionButtons wrapper class
            - selection tab:
                - there'll be 9 icons in it as well as multitude of text fields
                - there'd have to be an update logic, that will hide either text or 8 of the icons
                - the update is triggered when selection changes
                - again, probably retrieve a pointer to the wrapper class
            - map:
                - 
           
           hotkeys dispatch:
            - have a function on the IngameStageController that registers custom key callback
            - will use that to redirect inputs into this class (have handler function directly in here)
        */

        //NEXT UP - START WORKING ON THE INDIVIDUAL GUI ELEMENTS (SelectionTab - probably reuse ImageButtonGrid within)
        //NEED TO ADD VISIBILITY FUNCTIONALITY TO THE ImageButtonGrid - REQUIRED FOR BOTH SelectionTab AND FOR ActionButtons
    }

    void PlayerFactionController::Render() {
        //TODO: retrieve proper value
        bool isOrc = false;
        RenderGUIBorders(isOrc, -0.89f);

        if(is_menu_active) {
            menu_panel.Render();
        }

        game_panel.Render();
        text_prompt.Render();

        
        //ingame GUI render - left-side panel, resources bar, GUI borders
        //probably also render selection highlights from here (guess it should have lower z-index so that it's behind the actual object visuals)
    }

    void PlayerFactionController::Update(Level& level) {
        // gui.Update();
        //update the GUI - selection & input processing
        gui_handler.Update(&game_panel);

        //TODO: maybe move all GUI related stuff to its own class (if there's going to be more stuff managed within the class)
    }

    void PlayerFactionController::SwitchMenu(bool active) {
        // ASSERT_MSG(handler != nullptr, "PlayerFactionController not initialized properly!");
        is_menu_active = active;
        // handler->PauseRequest(active);
    }

    //==============================================================

    void RenderGUIBorders(bool isOrc, float z) {
        Window& window = Window::Get();

        //render GUI overlay (borders)
        float w = GUI_WIDTH*2.f;
        float b = BORDER_SIZE*2.f;
        glm::vec4 clr = glm::vec4(0.44f, 0.44f, 0.44f, 1.f);

        //gui panel background
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(-1.f, -1.f, z), glm::vec2(GUI_WIDTH*2.f, 2.f), clr));

        //borders
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(1.f - b, -1.f, z-2e-3f), glm::vec2(b, 2.f), clr));
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(-1.f + w, -1.f, z-1e-3f), glm::vec2(2.f - w, b*window.Ratio()), clr));
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(-1.f + w,  1.f-b*window.Ratio(), z-1e-3f), glm::vec2(2.f - w, b*window.Ratio()), clr));

        //TODO: modify based on race (at least the color, if not using textures)
    }

}//namespace eng
