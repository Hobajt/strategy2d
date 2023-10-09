#include "engine/game/controllers.h"

#include "engine/core/input.h"
#include "engine/core/renderer.h"
#include "engine/utils/generator.h"
#include "engine/game/resources.h"
#include "engine/game/camera.h"
#include "engine/game/config.h"
#include "engine/game/level.h"

namespace eng {

    constexpr float GUI_WIDTH = 0.25f;
    constexpr float BORDER_SIZE = 0.025f;

    constexpr float GUI_BORDERS_ZIDX = -0.89f;

    constexpr float SELECTION_ZIDX = GUI_BORDERS_ZIDX + 0.01f;
    constexpr float SELECTION_HIGHLIGHT_WIDTH = 0.025f;

    static glm::vec4 textClr = glm::vec4(0.92f, 0.77f, 0.20f, 1.f);

    void RenderGUIBorders(bool isOrc, float z);

    /*how to manage button callbacks:
        - first of all, don't have to make this class as callback handler - can use faction controller directly
        - action buttons:
            - they just invoke commands (IssueCommand()) on current selection
            - only need to track button state (when selecting command target)
        - selection icons:
            - only change selection (select 1 unit from selected group)
        - menu buttons:
            - one pauses the game, the other also invokes game menu
            - haven't really thought through how to handle game pauses, nor ingame menu for that manner

        - game menu could just be located within the player controller & completely managed by it
        - there's still the need to propagate the choices from it to the game stage controller (ingame stage)
        - also need to communicate the request for game pause to the controller
    */

    //cancel ongoing command target selection when selection updates (unit dies for example)
    /* cursor visuals:
        - switching conditions - what is being hovered over, is command selected
        - command selected
            - render eagle
            - green if hovering over object, yellow otherwise
        - no command:   
            - render magnifying glass if hovering over object, or hand otherwise
    */

    //===== GUI::SelectionTab =====

    GUI::SelectionTab::SelectionTab(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, 
        const StyleRef& borders_style_, const StyleRef& text_style, const StyleRef& text_style_small,
        const StyleRef& mana_bar_style_, const glm::vec2& mana_borders_size_, const StyleRef& progress_bar_style_, const glm::vec2& progress_borders_size_, const StyleRef& passive_btn_style_,
        const StyleRef& btn_style_, const StyleRef& bar_style_, const glm::vec2& bar_borders_size_, const Sprite& sprite_, ButtonCallbackHandler* handler_, ButtonCallbackType callback_)
        : Element(offset_, size_, zOffset_, Style::Default(), nullptr) {
        
        //icon button grid, when there's multiple objects selected
        btns = static_cast<ImageButtonGrid*>(AddChild(new ImageButtonGrid(glm::vec2(0.f, 0.f), glm::vec2(1.f, 1.f), 0.f, btn_style_, bar_style_, bar_borders_size_, sprite_, 3, 3, glm::vec2(0.975f / 3.f, 0.9f / 3.f), handler_, callback_), true));
        

        //unit name & current level fields; next to the image
        name = static_cast<TextLabel*>(AddChild(new TextLabel(glm::vec2(0.3f, -0.8f), glm::vec2(0.625f, 0.1f), 2.f, text_style, "Unit Name"), true));
        level_text = static_cast<TextLabel*>(AddChild(new TextLabel(glm::vec2(0.3f, -0.6f), glm::vec2(0.625f, 0.1f), 2.f, text_style, "Level X"), true));

        //text with health value (curr/max); directly underneath the health bar
        health = static_cast<TextLabel*>(AddChild(new TextLabel(glm::vec2(-0.625f, -0.3f), glm::vec2(0.33f, 0.05f), 2.f, text_style_small, "xxx/yyy"), true));

        //headline for the stats ("Production" or "Food Usage" written above the actual stats)
        headline = static_cast<TextLabel*>(AddChild(new TextLabel(glm::vec2(-0.05f, -0.1f), glm::vec2(0.8f, 0.05f), 2.f, text_style, "Stats headline", false), true));

        //stats fields
        for(size_t i = 0; i < stats.size(); i++) {
            stats[i] = static_cast<KeyValue*>(AddChild(new KeyValue(glm::vec2(0.f, -0.1f + 0.1f * i), glm::vec2(0.8f, 0.05f), 2.f, text_style, "Key: Value"), true));
        }

        //mana bar
        mana_bar = static_cast<ValueBar*>(AddChild(new ValueBar(glm::vec2(0.525f, 0.4f), glm::vec2(0.45f, 0.05f), 3.f, mana_bar_style_, mana_borders_size_, text_style_small, glm::vec4(0.f, 0.69f, 0.79f, 1.f), "123"), true));

        //production stuff
        production_bar = static_cast<ValueBar*>(AddChild(new ValueBar(glm::vec2(0.0f, 0.85f), glm::vec2(0.95f, 0.1f), 3.f, progress_bar_style_, progress_borders_size_, text_style, glm::vec4(0.0f, 0.5f, 0.125f, 1.f), "% Complete"), true));
        production_icon = static_cast<ImageButton*>(AddChild(new ImageButton(glm::vec2(0.5f, 0.2f), glm::vec2(0.9f / 3.f), 5.f, btn_style_, sprite_, glm::ivec2(0), 0.9f, handler_, callback_), true));

        mana_bar->SetValue(0.75f);
        production_bar->SetValue(0.75f);

        //frame borders around the entire area
        borders = AddChild(new Menu(glm::vec2(0.f, 0.f), glm::vec2(1.f, 1.f), 0.5f, borders_style_, {}), true);

        Reset();
    }

    void GUI::SelectionTab::Update(Level& level, const PlayerSelection& selection) {
        //reset all the GUI elements into its default state
        Reset();
        char buf[1024];

        if(selection.selected_count == 1) {
            FactionObject* object = &level.objects.GetObject(selection.selection[0]);
            borders->Enable(true);

            //TODO: need to add separate ingame-name field to the object data (what's now treated as name essentially serves as an identifier)
            //TODO: need to think through where to take the icon idx (probably just another field in object data)
            //TODO: how to compute the unit Level
            //TODO: how to display various stats for various objects (it's not unified, not even among buildings - farms have stats, but barracks dont for example)

            /*UNIT LEVEL:
                - is computed from upgrades that affect the unit
                - not sure how exactly is it computed, but can probably treat it as a sum of all the upgrades
                - how are upgrades going to work:
                    - they'll be stored in faction data
                    - unit will have getters for various data properties
                    - the getters will query faction data and do conditional additions to the property
                - how will the value increments be handled internally in the faction data:
                    - could maybe create a table with increment values for each unit
                    - the unit can then just lookup a row based on unit type & column based on value type
                    - level could be stored in the table as well
            */

            /*STATS:
                - for units, always display armor, damage, range, sight, speed
                    - if unit has magic, also display mana bar
                - buildings:
                    - if it's a gatherable resource, display only the amount left
                    - if it's a dropoff point for a resource, display "Production" headline and the resource that you can dropoff
                    - if it's a farm, display population stats
                    - else display no stats at all
            */

            /*HOW TO HANDLE KNIGHT-PALA and OGRE-MAGI TRANSITIONS:
                - they could be two separate unit types or a single one
                    - having single unit would simplify the upgrade process
                    - but also causes troubles in object_data - need to track multiple icons & separate properties bcs of a singel unit type
                    - GOING TO BE 2 SEPARATE UNIT TYPES (think the game does this too, based on values in the game editor)
            */

            //TODO: implement proper unit speed handling (including the values, so that it matches move speed  from the game)
            //TODO: maybe redo the object sizes (there's map size and graphics size)

            //TODO: transition from string resource identifiers to numbers & enums
            //      - gonna need it anyway for the resource indexes
            //      - after that's done, might want to altogether remove the generic object_data loading

            //icon & health bar setup
            btns->GetButton(0)->Setup(object->Name(), object->Icon(), object->HealthPercentage());
            snprintf(buf, sizeof(buf), "%d/%d", object->Health(), object->MaxHealth());
            health->Setup(std::string(buf));
            
            name->Setup(object->Name());

            if(object->IsUnit()) {
                Unit* unit = static_cast<Unit*>(object);

                snprintf(buf, sizeof(buf), "Level %d", unit->UnitLevel());
                level_text->Setup(std::string(buf));

                snprintf(buf, sizeof(buf), "Armor: %d", unit->Armor());
                stats[0]->Setup(buf);

                snprintf(buf, sizeof(buf), "Damage: %d-%d", unit->MinDamage(), unit->MaxDamage());
                stats[1]->Setup(buf);

                snprintf(buf, sizeof(buf), "Range: %d", unit->AttackRange());
                stats[2]->Setup(buf);

                snprintf(buf, sizeof(buf), "Sight: %d", unit->VisionRange());
                stats[3]->Setup(buf);

                snprintf(buf, sizeof(buf), "Speed: %d", (int)unit->MoveSpeed());
                stats[4]->Setup(buf);

                if(unit->IsCaster()) {
                    stats[5]->Setup("Magic:");
                    snprintf(buf, sizeof(buf), "%d", unit->Mana());
                    mana_bar->Setup(unit->Mana(), buf);
                }
            }
        }
        else if(selection.selected_count > 1) {
            
        }
    }

    void GUI::SelectionTab::ValuesUpdate(Level& level, const PlayerSelection& selection) {
        char buf[1024];

        if(selection.selected_count == 1) {
            FactionObject* object = &level.objects.GetObject(selection.selection[0]);

            btns->GetButton(0)->SetValue(object->HealthPercentage());
            snprintf(buf, sizeof(buf), "%d/%d", object->Health(), object->MaxHealth());
            health->Setup(std::string(buf));
        }
        else if(selection.selected_count > 1) {
            
        }
    }

    void GUI::SelectionTab::Reset() {
        //hide all the icon btns
        for(int i = 0; i < 9; i++) {
            btns->GetButton(i)->Enable(false);
        }

        borders->Enable(false);

        name->Enable(false);
        level_text->Enable(false);
        health->Enable(false);

        headline->Enable(false);
        mana_bar->Enable(false);

        production_bar->Enable(false);
        production_icon->Enable(false);

        for(size_t i = 0; i < stats.size(); i++) {
            stats[i]->Enable(false);
        }
    }

    //===== PlayerSelection =====

    void PlayerSelection::Select(Level& level, const glm::vec2& start, const glm::vec2& end, int playerFactionID) {
        auto [m, M] = order_vals(start, end);

        //coordinates rounding (includes any tile that is even partially within the rectangle)
        glm::ivec2 im = glm::ivec2(std::floor(m.x), std::floor(m.y));
        glm::ivec2 iM = glm::ivec2(std::floor(M.x), std::floor(M.y));

        glm::ivec2 bounds = level.map.Size();
        im = glm::ivec2(std::max(im.x, 0), std::max(im.y, 0));
        iM = glm::ivec2(std::min(bounds.x-1, iM.x), std::min(bounds.y-1, iM.y));

        ENG_LOG_FINE("PlayerSelection::Select - range: ({}, {}) - ({}, {})", im.x, im.y, iM.x, iM.y);

        int object_count = 0;
        int selection_mode = 0;
        for(int y = im.y; y <= iM.y; y++) {
            for(int x = im.x; x <= iM.x; x++) {
                const TileData& td = level.map(y, x);

                if(!ObjectID::IsObject(td.id) || !ObjectID::IsValid(td.id))
                    continue;
                
                //under which category does current object belong; reset object counting when more important category is picked
                int object_mode = 1*int(td.id.type == ObjectType::UNIT) + 2*int(td.factionId == playerFactionID);
                if(selection_mode < object_mode) {
                    selection_mode = object_mode;
                    object_count = 0;
                }

                if(object_mode == selection_mode) {
                    if((selection_mode < 3 && object_count < 1) || (selection_mode == 3 && object_count < selection.size())) {
                        selection[object_count++] = td.id;
                    }
                }
            }
        }
        
        //dont update values if there was nothing selected (keep the old selection)
        if(object_count != 0) {
            selected_count = object_count;
            selection_type = selection_mode;
            update_flag = true;
            ENG_LOG_FINE("PlayerSelection::Select - mode {}, count: {}", selection_type, selected_count);
        }
        else {
            ENG_LOG_FINE("PlayerSelection::Select - no change");
        }
    }

    void PlayerSelection::Render() {
        Camera& camera = Camera::Get();
        glm::vec2 mult = camera.Mult();
        glm::vec4 highlight_clrs[3] = {
            glm::vec4(0.f, 1.f, 0.f, 1.f),
            glm::vec4(1.f, 1.f, 0.f, 1.f),
            glm::vec4(1.f, 0.f, 0.f, 1.f),
        };
        glm::vec4 clr = highlight_clrs[clr_idx];

        for(int i = 0; i < selected_count; i++) {
            auto& [pos, size] = location[i];
            glm::vec2 m = camera.map2screen(pos);
            glm::vec2 M = camera.map2screen(pos+size);
            glm::vec2 v = M - m;

            Renderer::RenderQuad(Quad::FromCorner(glm::vec3(m.x, m.y, SELECTION_ZIDX), glm::vec2(v.x, SELECTION_HIGHLIGHT_WIDTH * mult.y), clr));
            Renderer::RenderQuad(Quad::FromCorner(glm::vec3(m.x, M.y, SELECTION_ZIDX), glm::vec2(v.x, SELECTION_HIGHLIGHT_WIDTH * mult.y), clr));
            Renderer::RenderQuad(Quad::FromCorner(glm::vec3(m.x, m.y, SELECTION_ZIDX), glm::vec2(SELECTION_HIGHLIGHT_WIDTH * mult.x, v.y), clr));
            Renderer::RenderQuad(Quad::FromCorner(glm::vec3(M.x, m.y, SELECTION_ZIDX), glm::vec2(SELECTION_HIGHLIGHT_WIDTH * mult.x, v.y + SELECTION_HIGHLIGHT_WIDTH*mult.y), clr));
        }
    }

    void PlayerSelection::Update(Level& level, GUI::SelectionTab* selectionTab, GUI::ImageButtonGrid* actionButtons, int playerFactionID) {
        //check that all the objects still exist & toss out invalid ones
        int new_count = 0;
        for(int i = 0; i < selected_count; i++) {
            FactionObject* object = nullptr;
            if(!level.objects.GetObject(selection[i], object)) {
                update_flag = true;
            }
            else {
                selection[new_count] = selection[i];
                location[new_count] = { object->Position(), object->Data()->size };
                new_count++;

                int fID = object->FactionIdx();
                clr_idx = int(playerFactionID != fID) + int(level.factions.Diplomacy().AreHostile(playerFactionID, fID));
            }
        }
        selected_count = new_count;

        
        if(update_flag) {
            selectionTab->Update(level, *this);
            //update action buttons
            //TODO: maybe add a wrapper class for action buttons - could store additional data (like the button text, thats displayed on hover)
        }
        else {
            selectionTab->ValuesUpdate(level, *this);
        }

        update_flag = false;
    }

    //===== PlayerFactionController =====

    void PlayerFactionController::GUIRequestHandler::LinkController(const PlayerFactionControllerRef& ctrl) {
        playerController = ctrl;
        playerController->handler = this;
    }

    void PlayerFactionController::GUIRequestHandler::SignalKeyPress(int keycode, int modifiers) {
        ASSERT_MSG(playerController != nullptr, "PlayerController was not linked! Call LinkController()...");
        playerController->OnKeyPressed(keycode, modifiers);
    }

    PlayerFactionController::PlayerFactionController(FactionsFile::FactionEntry&& entry) : FactionController(std::move(entry)) {
        InitializeGUI();
    }

    void PlayerFactionController::Render() {
        //TODO: retrieve proper value
        bool isOrc = false;

        //render GUI borders based on player's race
        RenderGUIBorders(isOrc, GUI_BORDERS_ZIDX);

        //render the game menu if activated
        if(is_menu_active) {
            menu_panel.Render();
        }

        //render individual GUI elements
        game_panel.Render();
        text_prompt.Render();

        switch(state) {
            case PlayerControllerState::SELECTION:
                RenderSelectionRectangle();
                break;
        }

        //render selection highlights
        selection.Render();
    }

    void PlayerFactionController::Update(Level& level) {
        Camera& camera = Camera::Get();
        Input& input = Input::Get();

        //update the GUI - selection & input processing
        gui_handler.Update(&game_panel);

        glm::vec2 pos = input.mousePos_n;
        switch(state) {
            case PlayerControllerState::IDLE:       //default state
                //lmb.down in game view -> start selection
                //rmb.down in game view -> adaptive command for current selection
                //lmb.down in map view  -> center camera
                //rmb.down in map view  -> adaptive command for current selection
                //cursor at borders     -> move camera

                if(CursorInGameView(pos)) {
                    glm::vec2 coords = camera.GetMapCoords(input.mousePos_n * 2.f - 1.f) + 0.5f;
                    if(input.lmb.down()) {
                        //transition to object selection state
                        coords_start = coords_end = coords;
                        state = PlayerControllerState::SELECTION;
                    }
                    else if (input.rmb.down()) {
                        //TODO: issue adaptive command to current selection
                    }
                }
                else if(CursorInMapView(pos)) {
                    // glm::vec2 coords = ... coords from map image position ...
                    if(input.lmb.down()) {
                        //transition to camera movement state
                        state = PlayerControllerState::CAMERA_CENTERING;
                    }
                    else if (input.rmb.down()) {
                        //TODO: issue adaptive command to current selection
                    }
                }
                else {
                    //camera panning
                    glm::ivec2 vec = glm::ivec2(int(pos.x > 0.95f) - int(pos.x < 0.05f), -int(pos.y > 0.95f) + int(pos.y < 0.05f));
                    if(vec.x != 0 || vec.y != 0) {
                        camera.Move(vec);
                    }
                }
                break;
            case PlayerControllerState::SELECTION:          //selection in progress (lmb click or drag)
                input.ClampCursorPos(glm::vec2(GUI_WIDTH, BORDER_SIZE), glm::vec2(1.f-BORDER_SIZE, 1.f-BORDER_SIZE));
                pos = input.mousePos_n;

                coords_end = camera.GetMapCoords(input.mousePos_n * 2.f - 1.f) + 0.5f;
                if(input.lmb.up()) {
                    selection.Select(level, coords_start, coords_end, ID());
                    state = PlayerControllerState::IDLE;
                }
                break;
            case PlayerControllerState::CAMERA_CENTERING:   //camera centering (lmb drag in the map view)
                //TODO:
                break;
        }

        //selection data update (& gui updates that display selection)
        selection.Update(level, selectionTab, actionButtons, ID());

        //TODO: setup current cursor from here as well (based on current state'n'stuff)
    }

    void PlayerFactionController::SwitchMenu(bool active) {
        ASSERT_MSG(handler != nullptr, "PlayerFactionController not initialized properly!");
        is_menu_active = active;
        handler->PauseRequest(active);
    }

    void PlayerFactionController::OnKeyPressed(int keycode, int modifiers) {
        ENG_LOG_TRACE("KEY PRESSED: {}", keycode);

        switch(keycode) {
            case GLFW_KEY_ESCAPE:
                break;
            default:
                //dispatch to current selection
                break;
        }
    }

    bool PlayerFactionController::CursorInGameView(const glm::vec2& pos) const {
        return pos.x > GUI_WIDTH && pos.x < (1.f-BORDER_SIZE) && pos.y > BORDER_SIZE && pos.y < (1.f-BORDER_SIZE);
    }

    bool PlayerFactionController::CursorInMapView(const glm::vec2& pos) const {
        return false;
    }

    void PlayerFactionController::RenderSelectionRectangle() {
        Camera& camera = Camera::Get();
        glm::vec2 mult = camera.Mult();
        glm::vec4 highlight_clr = glm::vec4(0.f, 1.f, 0.f, 1.f);

        auto [m, M] = order_vals(coords_start, coords_end);

        m = camera.map2screen(m);
        M = camera.map2screen(M);
        glm::vec2 v = M - m;

        if(fabsf(v.x) + fabsf(v.y) < 1e-2f)
            return;

        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(m.x, m.y, SELECTION_ZIDX), glm::vec2(v.x, SELECTION_HIGHLIGHT_WIDTH * mult.y), highlight_clr));
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(m.x, M.y, SELECTION_ZIDX), glm::vec2(v.x, SELECTION_HIGHLIGHT_WIDTH * mult.y), highlight_clr));
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(m.x, m.y, SELECTION_ZIDX), glm::vec2(SELECTION_HIGHLIGHT_WIDTH * mult.x, v.y), highlight_clr));
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(M.x, m.y, SELECTION_ZIDX), glm::vec2(SELECTION_HIGHLIGHT_WIDTH * mult.x, v.y + SELECTION_HIGHLIGHT_WIDTH*mult.y), highlight_clr));
    }

    void PlayerFactionController::InitializeGUI() {
        gui_handler = GUI::SelectionHandler();

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
        text_style->textScale = 0.9f;

        GUI::StyleRef text_style_small = std::make_shared<GUI::Style>();
        text_style_small->textColor = textClr;
        text_style_small->font = font;
        text_style_small->textScale = 0.5f;
        text_style_small->color = glm::vec4(0.f);

        //dbg only - to visualize the element size
        // text_style->color = glm::vec4(1.f);
        // text_style_small->color = glm::vec4(1.f);
        // text_style->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, false);
        // text_style_small->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, false);

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

        GUI::StyleRef bar_style = std::make_shared<GUI::Style>();
        bar_style->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, glm::uvec3(70), glm::uvec3(20), glm::uvec3(70), glm::uvec3(70));

        buttonSize = glm::vec2(0.25f, 0.25f * GUI::ImageButtonWithBar::BarHeightRatio());
        ts = glm::vec2(Window::Get().Size()) * buttonSize;
        upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
        textureSize = ts * upscaleFactor;
        borderWidth = 7 * upscaleFactor;
        glm::vec2 mana_bar_borders_size = 7.f / ts;
        GUI::StyleRef mana_bar_style = std::make_shared<GUI::Style>();
        mana_bar_style->texture = TextureGenerator::ButtonTexture_Clear_2borders(textureSize.x, textureSize.y, borderWidth, borderWidth, glm::uvec3(120), glm::uvec3(20), glm::uvec3(120), glm::uvec3(120), glm::uvec3(240), borderWidth/2);

        glm::vec2 progress_bar_borders_size = 7.f / ts;
        GUI::StyleRef progress_bar_style = std::make_shared<GUI::Style>();
        progress_bar_style->texture = TextureGenerator::ButtonTexture_Clear_3borders(textureSize.x, textureSize.y, borderWidth, borderWidth, glm::uvec3(120), glm::uvec3(20), glm::uvec3(120), glm::uvec3(120), glm::uvec3(252,208,61), 2*borderWidth/3, glm::uvec3(20), borderWidth/3);

        SpritesheetRef icons = Resources::LoadSpritesheet("misc/icons");
        Sprite icon = (*icons)("icon");

        actionButtons = new GUI::ImageButtonGrid(glm::vec2(0.5f, 0.675f), glm::vec2(0.475f, 0.3f), 1.f, icon_btn_style, icon, 3, 3, glm::vec2(0.975f / 3.f), this, [](GUI::ButtonCallbackHandler* handler, int id){
            ENG_LOG_TRACE("ActionButtons - Button [{}, {}] clicked", id % 3, id / 3);
        });

        selectionTab = new GUI::SelectionTab(glm::vec2(0.5f, 0.f), glm::vec2(0.475f, 0.35f), 1.f, 
            borders_style, text_style, text_style_small,
            mana_bar_style, mana_bar_borders_size, progress_bar_style, progress_bar_borders_size, icon_btn_style,
            icon_btn_style, bar_style, bar_border_size, icon, this, [](GUI::ButtonCallbackHandler* handler, int id){
            ENG_LOG_TRACE("SelectionTab - Button [{}, {}] clicked", id % 3, id / 3);
        });

        game_panel = GUI::Menu(glm::vec2(-1.f, 0.f), glm::vec2(GUI_WIDTH*2.f, 1.f), 0.f, std::vector<GUI::Element*>{
            // new GUI::Map(),             //map view
            new GUI::TextButton(glm::vec2(0.25f, -0.95f), glm::vec2(0.2f, 0.03f), 1.f, menu_btn_style, "Menu", this, [](GUI::ButtonCallbackHandler* handler, int id){
                static_cast<PlayerFactionController*>(handler)->SwitchMenu(true);
            }),
            new GUI::TextButton(glm::vec2(0.75f, -0.95f), glm::vec2(0.2f, 0.03f), 1.f, menu_btn_style, "Pause", this, [](GUI::ButtonCallbackHandler* handler, int id){
                ASSERT_MSG(static_cast<PlayerFactionController*>(handler)->handler != nullptr, "PlayerFactionController is not initialized properly!");
                static_cast<PlayerFactionController*>(handler)->handler->PauseToggleRequest();
            }),

            actionButtons,
            selectionTab,
        });

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
    }

    //==============================================================

    void RenderGUIBorders(bool isOrc, float z) {
        Window& window = Window::Get();

        //render GUI overlay (borders)
        float w = GUI_WIDTH*2.f;
        float b = BORDER_SIZE*2.f;
        glm::vec4 clr = glm::vec4(0.33f, 0.33f, 0.33f, 1.f);

        //gui panel background
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(-1.f, -1.f, z), glm::vec2(GUI_WIDTH*2.f, 2.f), clr));

        //borders
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(1.f - b, -1.f, z-2e-3f), glm::vec2(b, 2.f), clr));
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(-1.f + w, -1.f, z-1e-3f), glm::vec2(2.f - w, b*window.Ratio()), clr));
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(-1.f + w,  1.f-b*window.Ratio(), z-1e-3f), glm::vec2(2.f - w, b*window.Ratio()), clr));

        //TODO: modify based on race (at least the color, if not using textures)
    }

}//namespace eng
