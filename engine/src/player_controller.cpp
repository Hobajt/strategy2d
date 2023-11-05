#include "engine/game/player_controller.h"

#include "engine/core/input.h"
#include "engine/core/renderer.h"
#include "engine/utils/generator.h"
#include "engine/game/resources.h"
#include "engine/game/camera.h"
#include "engine/game/config.h"
#include "engine/game/level.h"
#include "engine/core/audio.h"

#include "engine/utils/dbg_gui.h"

namespace eng {

    constexpr float GUI_WIDTH = 0.25f;
    constexpr float BORDER_SIZE = 0.025f;

    constexpr float GUI_BORDERS_ZIDX = -0.89f;

    constexpr float SELECTION_ZIDX = GUI_BORDERS_ZIDX + 0.01f;
    constexpr float SELECTION_HIGHLIGHT_WIDTH = 0.025f;

    static glm::vec4 textClr = glm::vec4(0.92f, 0.77f, 0.20f, 1.f);

    constexpr glm::vec2 MAP_A = glm::vec2(0.025f,                0.05f);
    constexpr glm::vec2 MAP_B = glm::vec2(0.025f+GUI_WIDTH*0.8f, 0.3f);

    void RenderGUIBorders(bool isOrc, float z);
    std::vector<GUI::StyleRef> SetupScrollMenuStyles(const FontRef& font, const glm::vec2& scrollMenuSize, int scrollMenuItems, float scrollButtonSize, const glm::vec2& smallBtnSize);

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

            //icon & name setup
            btns->GetButton(0)->Setup(object->Name(), -1, object->Icon(), object->HealthPercentage());
            name->Setup(object->Name());

            //health bar (show only for player owned objects)
            if(selection.selection_type >= SelectionType::PLAYER_BUILDING) {
                snprintf(buf, sizeof(buf), "%d/%d", object->Health(), object->MaxHealth());
                health->Setup(std::string(buf));
            }

            if(object->IsUnit() && selection.selection_type >= SelectionType::PLAYER_BUILDING) {
                //display units stats (always the same, except the mana bar might be hidden)
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
            else {
                Building* building = static_cast<Building*>(object);
                int not_gnd = int(building->NavigationType() != NavigationBit::GROUND);
                if(building->IsResource()) {
                    //resource -> amount of resource left
                    snprintf(buf, sizeof(buf), "%s left: %d", GameResourceName(2*not_gnd), building->AmountLeft());
                    stats[1]->Setup(buf);
                }
                else if(selection.selection_type >= SelectionType::PLAYER_BUILDING) {
                    int production_boost;
                    switch(building->NumID()[1]) {
                        case BuildingType::TOWN_HALL:
                            stats[0]->Setup("Production ");
                            for(int i = 0; i < 3; i++) {
                                production_boost = building->Faction()->ProductionBoost(i);
                                if(production_boost != 0)
                                    snprintf(buf, sizeof(buf), "%s: 100+%d", GameResourceName(i), production_boost);
                                else
                                    snprintf(buf, sizeof(buf), "%s: 100", GameResourceName(i));
                                stats[1+i]->Setup(buf);
                            }
                            break;
                        case BuildingType::OIL_REFINERY:
                        case BuildingType::LUMBER_MILL:
                            stats[1]->Setup("Production ");
                            production_boost = building->Faction()->ProductionBoost(1+not_gnd);
                            if(production_boost != 0)
                                snprintf(buf, sizeof(buf), "%s: 100+%d", GameResourceName(1+not_gnd), production_boost);
                            else
                                snprintf(buf, sizeof(buf), "%s: 100", GameResourceName(1+not_gnd));
                            stats[2]->Setup(buf);
                            break;
                        case BuildingType::FARM:        //display population statistics
                            glm::ivec2 pop = building->Faction()->Population();
                            stats[0]->Setup("Food Usage ");
                            snprintf(buf, sizeof(buf), "Grown: %d", pop[1]);
                            stats[1]->Setup(buf);
                            snprintf(buf, sizeof(buf), "Used: %d", pop[0]);
                            stats[2]->Setup(buf);
                            break;
                    }
                }
            }
        }
        else if(selection.selected_count > 1) {
            FactionObject* object;
            for(int i = 0; i < selection.selected_count; i++) {
                object = &level.objects.GetObject(selection.selection[i]);
                btns->GetButton(i)->Setup(object->Name(), -1, object->Icon(), object->HealthPercentage());
            }
        }
    }

    void GUI::SelectionTab::ValuesUpdate(Level& level, const PlayerSelection& selection) {
        char buf[1024];

        if(selection.selected_count == 1) {
            FactionObject* object = &level.objects.GetObject(selection.selection[0]);

            btns->GetButton(0)->SetValue(object->HealthPercentage());

            if(selection.selection_type >= SelectionType::PLAYER_BUILDING) {
                snprintf(buf, sizeof(buf), "%d/%d", object->Health(), object->MaxHealth());
                health->Setup(std::string(buf));
            }
        }
        else if(selection.selected_count > 1) {
            FactionObject* object;
            for(int i = 0; i < selection.selected_count; i++) {
                object = &level.objects.GetObject(selection.selection[i]);
                btns->GetButton(i)->SetValue(object->HealthPercentage());
            }
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

    //===== GUI::ActionButtons =====

    GUI::ActionButtons::ActionButtons(ImageButtonGrid* btns_) : btns(btns_) {
        for(int i = 0; i < 4; i++)
            ResetPage(i);
        pages[3][8] = ActionButtonDescription::CancelButton();
        ChangePage(0);
    }

    void GUI::ActionButtons::Update(Level& level, const PlayerSelection& selection) {
        //buildup pages for the currently selected objects
        ResetPage(0);
        PageData& p = pages[0];
        
        //no player owned selection -> no pages
        if(selection.selection_type < SelectionType::PLAYER_BUILDING || selection.selected_count < 1)
            return;

        if(selection.selection_type == SelectionType::PLAYER_UNIT) {
            //scan through the selection & obtain properties that define how the GUI will look
            bool can_attack = false;
            bool all_workers = true;
            bool carrying_goods = false;
            bool can_attack_ground = false;
            bool all_water_units = true;
            int att_upgrade_src = UnitUpgradeSource::NONE;
            int def_upgrade_src = UnitUpgradeSource::NONE;
            FactionControllerRef faction = nullptr;

            for(size_t i = 0; i < selection.selected_count; i++) {
                Unit& unit = level.objects.GetUnit(selection.selection[i]);

                all_workers         &= unit.IsWorker();
                all_water_units     &= (unit.NavigationType() == NavigationBit::WATER);
                can_attack          |= unit.CanAttack();
                carrying_goods      |= unit.IsWorker() && (unit.CarryStatus() != WorkerCarryState::NONE);
                can_attack_ground   |= false;   //TODO: once there's the command for that

                if(i == 0) {
                    att_upgrade_src = unit.AttackUpgradeSource();
                    def_upgrade_src = unit.DefenseUpgradeSource();
                    faction = unit.Faction();
                }
                else {
                    if(att_upgrade_src != unit.AttackUpgradeSource())  att_upgrade_src = UnitUpgradeSource::NONE;
                    if(def_upgrade_src != unit.DefenseUpgradeSource()) def_upgrade_src = UnitUpgradeSource::NONE;
                }
            }

            bool isOrc = bool(faction->Race());
            att_upgrade_src = (att_upgrade_src == UnitUpgradeSource::BLACKSMITH_BALLISTA) ? UnitUpgradeSource::BLACKSMITH : att_upgrade_src;
            def_upgrade_src = (def_upgrade_src >  UnitUpgradeSource::FOUNDRY) ? UnitUpgradeSource::BLACKSMITH : def_upgrade_src;

            //add default unit commands for unit selection
            p[0] = ActionButtonDescription::Move(isOrc, all_water_units);
            p[1] = ActionButtonDescription::Stop(isOrc, def_upgrade_src, faction->UnitUpgradeTier(false, def_upgrade_src));
            p[3] = ActionButtonDescription::Patrol(isOrc, all_water_units);
            p[4] = ActionButtonDescription::StandGround(isOrc);

            //attack command only if there's at least one unit that can attack
            if(can_attack) {
                p[2] = ActionButtonDescription::Attack(isOrc, att_upgrade_src, faction->UnitUpgradeTier(true, att_upgrade_src));
            }

            //repair & harvest if they're all workers
            if(all_workers) {
                p[3] = ActionButtonDescription::Repair();
                p[4] = ActionButtonDescription::Harvest(all_water_units);

                //returns goods if one of them is carrying
                if(carrying_goods) {
                    p[5] = ActionButtonDescription::ReturnGoods(isOrc, all_water_units);
                }
            }

            if(can_attack_ground) {
                p[5] = ActionButtonDescription::AttackGround();
            }
        }

        //display specialized buttons only if there's no more than 1 object in the selection
        if(selection.selected_count == 1) {
            //retrieve object_data reference & page descriptions from that
            FactionObject& object = level.objects.GetObject(selection.selection[0]);
            const ButtonDescriptions& desc = object.ButtonDescriptions();

            int p_idx = 0;
            for(const ButtonDescriptions::PageEntry& page_desc : desc.pages) {
                if(p_idx > 0) {
                    ResetPage(p_idx);
                    pages[p_idx][8] = ActionButtonDescription::CancelButton();
                }
                PageData& current_page = pages[p_idx++];

                for(const auto& [pos_idx, btn_data] : page_desc) {
                    current_page[pos_idx] = btn_data;
                }
            }
        }
        
        ChangePage(0);
    }

    void GUI::ActionButtons::ValuesUpdate(Level& level, const PlayerSelection& selection) {
        if(selection.selected_count < 1)
            return;
        
        PageData& pg = pages[page];
        FactionObject& obj = level.objects.GetObject(selection.selection[0]);
        FactionControllerRef player = level.factions.Player();
        
        //iterate through all the buttons (in active page) & enable/disable them based on whether the button action conditions are met
        for(size_t i = 0; i < pg.size(); i++) {
            ActionButtonDescription btn = pg[i];
            bool conditions_met = (btn.command_id != ActionButton_CommandType::DISABLED) && player->ButtonConditionCheck(obj, btn);
            btns->GetButton(i)->Enable(conditions_met);
        }
    }

    void GUI::ActionButtons::DispatchHotkey(char hotkey) {
        PageData& pd = pages[page];

        // ENG_LOG_FINE("ActionButtons::DispatchHotkey - looking for hotkey '{}' ({})", hotkey, int(hotkey));
        for(size_t i = 0; i < pd.size(); i++) {
            // ENG_LOG_FINE("btn[{}] - ({}, {}), '{}' ({}), {}", i, pd[i].command_id, pd[i].has_hotkey, pd[i].hotkey, int(pd[i].hotkey), pd[i].hotkey_idx);
            if(pd[i].command_id != ActionButton_CommandType::DISABLED && pd[i].has_hotkey && pd[i].hotkey == hotkey) {
                ENG_LOG_FINE("ActionButtons::DispatchHotkey - key {:3d} -> button[{}] ({})", int(hotkey), i, pd[i].name);
                clicked_btn_idx = int(i);
                return;
            }
        }
    }

    void GUI::ActionButtons::ChangePage(int idx) {
        page = idx;

        PageData& p = pages[idx];
        for(size_t i = 0; i < p.size(); i++) {
            ActionButtonDescription& btn = p[i];
            if(btn.command_id != ActionButton_CommandType::DISABLED)
                btns->GetButton(i)->Setup(btn.name, btn.hotkey_idx, btn.icon, btn.price);
            else
                btns->GetButton(i)->Enable(false);
        }
    }

    void GUI::ActionButtons::ResetPage(int idx) {
        PageData& page = pages[idx];
        for(size_t i = 0; i < page.size(); i++) {
            page[i] = ActionButtonDescription();
        }
    }

    //===== GUI::ResourceBar =====

    GUI::ResourceBar::ResourceBar(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const StyleRef& style_, const Sprite& sprite_, bool alt_)
        : Element(offset_, size_, zOffset_, style_, nullptr), alt(alt_) {
        
        text[0] = static_cast<ImageAndLabel*>(AddChild(new ImageAndLabel(glm::vec2(alt ? -1.0f : -0.8f, 0.f), glm::vec2(1.f, 1.f), 1.f, style_, sprite_, IconIdx(0), "GOLD")));
        text[1] = static_cast<ImageAndLabel*>(AddChild(new ImageAndLabel(glm::vec2(alt ? -0.3f : -0.4f, 0.f), glm::vec2(1.f, 1.f), 1.f, style_, sprite_, IconIdx(1), "WOOD")));
        text[2] = static_cast<ImageAndLabel*>(AddChild(new ImageAndLabel(glm::vec2(alt ?  0.4f :  0.0f, 0.f), glm::vec2(1.f, 1.f), 1.f, style_, sprite_, IconIdx(2), "OIL ")));
        text[3] = static_cast<ImageAndLabel*>(AddChild(new ImageAndLabel(glm::vec2(alt ?  1.5f :  0.4f, 0.f), glm::vec2(1.f, 1.f), 1.f, style_, sprite_, IconIdx(4), "POP")));
    }

    void GUI::ResourceBar::Update(const glm::ivec3& resources, const glm::ivec2& population) {
        char buf[256];

        for(size_t i = 0; i < 3; i++) {
            snprintf(buf, sizeof(buf), "%d", resources[i]);
            text[i]->Setup(IconIdx(i), std::string(buf));
        }

        snprintf(buf, sizeof(buf), "%d/%d", population[0], population[1]);
        glm::ivec2 highlight = (population[0] > population[1]) ? glm::vec2(0, strchr(buf, '/') - buf) : glm::vec2(-1);
        text[3]->Setup(IconIdx(4), std::string(buf), highlight);
    }

    void GUI::ResourceBar::UpdateAlt(const glm::ivec4& price) {
        char buf[256];

        bool values_displayed = false;
        for(size_t i = 0; i < 3; i++) {
            if(price[i] == 0)
                continue;
            values_displayed = true;
            snprintf(buf, sizeof(buf), "%d", price[i]);
            text[i]->Setup(IconIdx(i), std::string(buf));
        }

        if(!values_displayed && price[3] != 0) {
            snprintf(buf, sizeof(buf), "%d", price[3]);
            text[0]->Setup(IconIdx(3), std::string(buf));
        }
        text[3]->Enable(false);
    }

    void GUI::ResourceBar::ClearAlt() {
        for(size_t i = 0; i < 4; i++) {
            text[i]->Enable(false);
        }
    }

    glm::ivec2 GUI::ResourceBar::IconIdx(int resource_idx) {
        constexpr static std::array<glm::ivec2, 5> idx = {
            glm::ivec2(0,18),    //gold
            glm::ivec2(1,18),    //wood
            glm::ivec2(2,18),    //oil
            glm::ivec2(3,18),    //mana
            glm::ivec2(4,18),    //population
        };
        return idx[resource_idx];
    }

    //===== GUI::PopupMessage =====

    GUI::PopupMessage::PopupMessage(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const StyleRef& style_)
        : TextLabel(offset_, size_, zOffset_, style_, "", false) {}

    void GUI::PopupMessage::DisplayMessage(const std::string& msg, float duration) {
        Setup(msg);
        time_limit = float(Input::Get().CurrentTime()) + duration;
    }

    void GUI::PopupMessage::Update() {
        if(time_limit <= (float)Input::Get().CurrentTime()) {
            Enable(false);
        }
    }

    //===== GUI::IngameMenu =====

    GUI::IngameMenu::IngameMenu(const glm::vec2& offset, const glm::vec2& size, float zOffset, PlayerFactionController* ctrl_, 
        const StyleRef& bg_style, const StyleRef& btn_style, const StyleRef& text_style) : ctrl(ctrl_) {

        constexpr float bw = 0.9f;
        constexpr float bh = 0.08f;

        int scrollMenuItems = 8;
        float scrollBtnSize = 1.f/16.f;
        glm::vec2 smallBtnSize = glm::vec2(0.1666f, 0.06f);
        glm::vec2 scrollMenuSize = glm::vec2(0.85f, 0.6f);
        std::vector<GUI::StyleRef> scrollMenu_styles = SetupScrollMenuStyles(text_style->font, scrollMenuSize, scrollMenuItems, scrollBtnSize, smallBtnSize);
        btn_styles = { btn_style, scrollMenu_styles.back() };
        
        menus.insert({ IngameMenuTab::MAIN, Menu(offset, size, zOffset, bg_style, std::vector<GUI::Element*>{
                new GUI::TextLabel(glm::vec2(0.f, -0.85f), glm::vec2(bw, bh), 1.f, text_style, "Game Menu"),
                new GUI::TextButton(glm::vec2(-0.5f, -0.65f), glm::vec2(bw*0.45f, bh), 1.f, btn_style, "Save (F11)", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::SAVE);
                }, glm::ivec2(6,9)),
                new GUI::TextButton(glm::vec2( 0.5f, -0.65f), glm::vec2(bw*0.45f, bh), 1.f, btn_style, "Load (F12)", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::LOAD);
                }, glm::ivec2(6,9)),
                new GUI::TextButton(glm::vec2( 0.f, -0.45f), glm::vec2(bw, bh), 1.f, btn_style, "Options (F5)", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::OPTIONS);
                }, glm::ivec2(9,11)),
                new GUI::TextButton(glm::vec2( 0.f, -0.25f), glm::vec2(bw, bh), 1.f, btn_style, "Help (F1)", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::HELP);
                }, glm::ivec2(6,8)),
                new GUI::TextButton(glm::vec2( 0.f, -0.05f), glm::vec2(bw, bh), 1.f, btn_style, "Scenario Objectives", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::OBJECTIVES);
                }, glm::ivec2(9,10)),
                new GUI::TextButton(glm::vec2( 0.f, 0.15f), glm::vec2(bw, bh), 1.f, btn_style, "End Scenario", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::END_SCENARIO);
                }, glm::ivec2(0,1)),
                new GUI::TextButton(glm::vec2( 0.f, 0.7f), glm::vec2(bw, bh), 1.f, btn_style, "Return to Game (Esc)", ctrl, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<PlayerFactionController*>(handler)->SwitchMenu(false);
                }, glm::ivec2(16,19)),
            })
        });

        menus.insert({ IngameMenuTab::OBJECTIVES, Menu(offset, size, zOffset, bg_style, std::vector<GUI::Element*>{
                new GUI::TextLabel(glm::vec2(0.f, -0.85f), glm::vec2(bw, bh), 1.f, text_style, "SCENARIO OBJECTIVES"),
                new GUI::ScrollText(glm::vec2(0.f, -0.075f), glm::vec2(bw, 0.7f), 1.f, bg_style, ""),
                new GUI::TextButton(glm::vec2( 0.f, 0.7f), glm::vec2(bw, bh), 1.f, btn_style, "Previous (Esc)", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::MAIN);
                }, glm::ivec2(16,19)),
            })
        });
        list_objectives = dynamic_cast<ScrollText*>(menus.at(IngameMenuTab::OBJECTIVES).GetChild(1));

        menus.insert({ IngameMenuTab::OPTIONS, Menu(offset, size, zOffset, bg_style, std::vector<GUI::Element*>{
                new GUI::TextLabel(glm::vec2(0.f, -0.85f), glm::vec2(bw, bh), 1.f, text_style, "Game Options"),
                new GUI::TextButton(glm::vec2( 0.f, -0.65f), glm::vec2(bw, bh), 1.f, btn_style, "Sound (F7)", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::OPTIONS_SOUND);
                }, glm::ivec2(7,9)),
                new GUI::TextButton(glm::vec2( 0.f, -0.45f), glm::vec2(bw, bh), 1.f, btn_style, "Speeds (F8)", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::OPTIONS_SPEED);
                }, glm::ivec2(8,10)),
                new GUI::TextButton(glm::vec2( 0.f, -0.25f), glm::vec2(bw, bh), 1.f, btn_style, "Preferences (F9)", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::OPTIONS_PREFERENCES);
                }, glm::ivec2(13,15)),
                new GUI::TextButton(glm::vec2( 0.f, 0.7f), glm::vec2(bw, bh), 1.f, btn_style, "Previous (Esc)", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::MAIN);
                }, glm::ivec2(16,19)),
            })
        });

        menus.insert({ IngameMenuTab::OPTIONS_SOUND, Menu(offset, size, zOffset, bg_style, std::vector<GUI::Element*>{
                new GUI::TextLabel(glm::vec2(0.f, -0.85f), glm::vec2(bw, bh), 1.f, text_style, "Sound Options"),

                new GUI::TextLabel(glm::vec2(0.f, -0.7f), glm::vec2(bw, bh), 1.f, text_style, "Mater Volume", false),
                new GUI::TextLabel(glm::vec2(0.f, -0.4f), glm::vec2(bw, bh), 1.f, text_style, "Music Volume", false),
                new GUI::TextLabel(glm::vec2(0.f, -0.5f), glm::vec2(bw, bh), 1.f, text_style, "Digital Volume", false),
                new GUI::TextButton(glm::vec2(-0.5f, 0.7f), glm::vec2(bw*0.45f, bh), 1.f, btn_style, "Cancel", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    // static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::MAIN);
                }, glm::ivec2(0,1)),
                new GUI::TextButton(glm::vec2( 0.5f, 0.7f), glm::vec2(bw*0.45f, bh), 1.f, btn_style, "OK", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    // static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::MAIN);
                }, glm::ivec2(0,1)),
            })
        });

        menus.insert({ IngameMenuTab::LOAD, Menu(offset, glm::vec2(size.x * 1.5f, size.y * 0.85f), zOffset, bg_style, std::vector<GUI::Element*>{
                new GUI::TextLabel(glm::vec2(0.f, -0.85f), glm::vec2(bw, bh), 1.f, text_style, "Load Game"),
                new GUI::ScrollMenu(glm::vec2(-0.075f,-0.1f), scrollMenuSize, 1.f, scrollMenuItems, scrollBtnSize, scrollMenu_styles),
                new GUI::TextButton(glm::vec2(-0.3f, 0.7f), glm::vec2(0.6f, bh), 1.f, btn_style, "Load", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->action = MenuAction::LOAD;
                }, glm::ivec2(0,1)),
                new GUI::TextButton(glm::vec2( 0.65f, 0.7f), glm::vec2(0.25f, bh), 1.f, btn_style, "Cancel", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::MAIN);
                }, glm::ivec2(0,1)),
            })
        });
        list_load = dynamic_cast<ScrollMenu*>(menus.at(IngameMenuTab::LOAD).GetChild(1));

        glm::vec2 scrollMenuSize_save = glm::vec2(scrollMenuSize.x, scrollMenuSize.y * ((scrollMenuItems-1)/float(scrollMenuItems)));
        menus.insert({ IngameMenuTab::SAVE, Menu(offset, glm::vec2(size.x * 1.5f, size.y * 0.85f), zOffset, bg_style, std::vector<GUI::Element*>{
                new GUI::TextLabel(glm::vec2(0.f, -0.85f), glm::vec2(bw, bh), 1.f, text_style, "Save Game"),
                new GUI::TextInput(glm::vec2(0.f, -0.7f), glm::vec2(bw, bh), 1.f, scrollMenu_styles[0], 27),
                new GUI::ScrollMenu(glm::vec2(-0.075f,0.f), scrollMenuSize_save, 1.f, scrollMenuItems-1, scrollBtnSize, scrollMenu_styles, this, [](GUI::ButtonCallbackHandler* handler, int id){
                    IngameMenu* h = static_cast<IngameMenu*>(handler);
                    h->textInput->SetText(h->list_save->CurrentSelection());
                    h->EnableDeleteButton(true);
                }),
                new GUI::TextButton(glm::vec2(-0.65f, 0.7f), glm::vec2(0.25f, bh), 1.f, btn_style, "Save", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->action = MenuAction::SAVE;
                }, glm::ivec2(0,1)),
                new GUI::TextButton(glm::vec2(0.f, 0.7f), glm::vec2(0.25f, bh), 1.f, btn_style, "Delete", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->action = MenuAction::DELETE;
                }, glm::ivec2(0,1)),
                new GUI::TextButton(glm::vec2( 0.65f, 0.7f), glm::vec2(0.25f, bh), 1.f, btn_style, "Cancel", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::MAIN);
                }, glm::ivec2(0,1)),
            })
        });
        textInput = dynamic_cast<TextInput*>(menus.at(IngameMenuTab::SAVE).GetChild(1));
        list_save = dynamic_cast<ScrollMenu*>(menus.at(IngameMenuTab::SAVE).GetChild(2));
        delet_btn = dynamic_cast<TextButton*>(menus.at(IngameMenuTab::SAVE).GetChild(4));

        /*TODO:
            - implement horizontal slider GUI elements & add them to options submenus
            - implement save logic      - saving can be done just directly from here (invoke on Level object)
            - implement load logic      - need to call methods on StageController objects in order to change level
            - implement delete logic    - can probably do also from here
            - also maybe create separate class/namespace for listing the Saves directory

            - Config::Saves - directory path (maybe from config?) & directory scan
            - visuals - separate visuals for horde/aliance
            - visuals - disabled button visuals (delete button)
        */

        menus.insert({ IngameMenuTab::HELP, Menu(offset, size, zOffset, bg_style, std::vector<GUI::Element*>{
                new GUI::TextLabel(glm::vec2(0.f, -0.85f), glm::vec2(bw, bh), 1.f, text_style, "Help"),
                new GUI::TextLabel(glm::vec2(0.0f, -0.65f), glm::vec2(bw*0.45f, bh), 1.f, text_style, "No help here, use google."),
                new GUI::TextButton(glm::vec2( 0.f, 0.7f), glm::vec2(bw, bh), 1.f, btn_style, "Previous (Esc)", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::MAIN);
                }, glm::ivec2(16,19)),
            })
        });
    }

    GUI::IngameMenu::IngameMenu(IngameMenu&& m) noexcept {
        Move(std::move(m));
    }

    GUI::IngameMenu& GUI::IngameMenu::operator=(IngameMenu&& m) noexcept {
        Move(std::move(m));
        return *this;
    }

    void GUI::IngameMenu::Render() {
        menus.at(active_menu).Render();
    }

    void GUI::IngameMenu::Update(Level& level, PlayerFactionController& ctrl, SelectionHandler& gui_handler) {
        gui_handler.Update(&menus.at(active_menu));

        switch(action) {
            case MenuAction::SAVE:
                ASSERT_MSG(textInput != nullptr, "TextInput GUI element not set.");
                // level.Save(Config::Saves::FullPath(textInput->Text()));
                ENG_LOG_INFO("SAVING GAME STATE TO '{}'", Config::Saves::FullPath(textInput->Text()));
                break;
            case MenuAction::LOAD:
                ASSERT_MSG(list_load != nullptr, "Scroll menu GUI element not set.");
                ctrl.ChangeLevel(Config::Saves::FullPath(list_load->CurrentSelection()));
                ENG_LOG_INFO("LOADING GAME STATE FROM '{}'", Config::Saves::FullPath(list_load->CurrentSelection()));
                break;
            case MenuAction::DELETE:
                ASSERT_MSG(list_save != nullptr, "Scroll menu GUI element not set.");
                //TODO:
                ENG_LOG_INFO("DELETING SAVEFILE '{}'", Config::Saves::FullPath(textInput->Text()));
                break;
        }
        action = MenuAction::NONE;

        //TODO:
    }

    void GUI::IngameMenu::OnKeyPressed(int keycode, int modifiers, bool single_press) {
        switch(active_menu) {
            case IngameMenuTab::MAIN:
                switch(keycode) {
                    case GLFW_KEY_F1:       //help
                        OpenTab(IngameMenuTab::HELP);
                        break;
                    case GLFW_KEY_F5:       //options
                        OpenTab(IngameMenuTab::OPTIONS);
                        break;
                    case GLFW_KEY_F11:      //save
                        OpenTab(IngameMenuTab::SAVE);
                        break;
                    case GLFW_KEY_F12:      //load
                        OpenTab(IngameMenuTab::LOAD);
                        break;
                    case GLFW_KEY_O:        //objectives
                        OpenTab(IngameMenuTab::OBJECTIVES);
                        break;
                    case GLFW_KEY_E:        //end scenario
                        OpenTab(IngameMenuTab::END_SCENARIO);
                        break;
                    case GLFW_KEY_ESCAPE:   //return to game
                        ctrl->SwitchMenu(false);
                        break;
                }
                break;
            case IngameMenuTab::OPTIONS:
                switch(keycode) {
                    case GLFW_KEY_F7:
                        OpenTab(IngameMenuTab::OPTIONS_SOUND);
                        break;
                    case GLFW_KEY_F8:
                        OpenTab(IngameMenuTab::OPTIONS_SPEED);
                        break;
                    case GLFW_KEY_F9:
                        OpenTab(IngameMenuTab::OPTIONS_PREFERENCES);
                        break;
                    case GLFW_KEY_ESCAPE:
                        OpenTab(IngameMenuTab::MAIN);
                        break;
                }
                break;
            case IngameMenuTab::SAVE:
                ASSERT_MSG(textInput != nullptr, "TextInput GUI element not set.");
                switch(keycode) {
                    case GLFW_KEY_ESCAPE:
                        OpenTab(IngameMenuTab::MAIN);
                        break;
                    case GLFW_KEY_BACKSPACE:
                        textInput->Backspace();
                        EnableDeleteButton(false);
                        break;
                    default:
                        if(keycode >= 32 && keycode <= 96) {
                            textInput->AddChar(char(keycode) + ('a'-'A') * int((modifiers & 1 == 0) && keycode >= GLFW_KEY_A && keycode <= GLFW_KEY_Z));
                            EnableDeleteButton(false);
                        }
                        break;
                }
                break;
            case IngameMenuTab::LOAD:
                switch(keycode) {
                    case GLFW_KEY_ESCAPE:
                        OpenTab(IngameMenuTab::MAIN);
                        break;
                }
                break;
        }
    }

    void GUI::IngameMenu::OpenTab(int tabID) {
        if(menus.count(tabID)) {
            active_menu = tabID;
            ENG_LOG_TRACE("IngameMenu - opening tab {}", tabID);

            switch(tabID) {
                case IngameMenuTab::SAVE:
                    list_save->UpdateContent(Config::Saves::Scan(), true);
                    textInput->SetText(list_save->CurrentSelection());
                    EnableDeleteButton(true);
                    break;
                case IngameMenuTab::LOAD:
                    list_load->UpdateContent(Config::Saves::Scan(), true);
                    break;
            }
        }
        else {
            ENG_LOG_WARN("IngameMenu - attempting to open a tab with invalid ID ({})", tabID);
        }
    }

    void GUI::IngameMenu::Move(IngameMenu&& m) noexcept {
        menus = std::move(m.menus);
        active_menu = m.active_menu;
        ctrl = m.ctrl;

        list_objectives = m.list_objectives;
        list_load = m.list_load;
        list_save = m.list_save;
        textInput = m.textInput;
        delet_btn = m.delet_btn;
        btn_styles = std::move(m.btn_styles);

        for(auto& [id, menu] : menus) {
            for(auto& child : menu) {
                child->HandlerPtrMove(&m, this);
            }
        }
    }

    void GUI::IngameMenu::EnableDeleteButton(bool enabled) {
        ASSERT_MSG(delet_btn != nullptr, "Delete button reference is not set.");
        delet_btn->Interactable(enabled);
        delet_btn->ChangeStyle(btn_styles[int(!enabled)]);
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

        //selection based on map tiles readthrough
        int object_count = 0;
        int selection_mode = 0;
        for(int y = im.y; y <= iM.y; y++) {
            for(int x = im.x; x <= iM.x; x++) {
                const TileData& td = level.map(y, x);

                if(!ObjectID::IsObject(td.id) || !ObjectID::IsValid(td.id))
                    continue;
                
                //under which category does current object belong; reset object counting when more important category is picked
                int object_mode = ObjectSelectionType(td.id, td.factionId, playerFactionID);
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

        //selection among traversable objects (which are not part of regular map tiles data)
        iM += 1;
        for(const ObjectID& id : level.map.TraversableObjects()) {
            if(!ObjectID::IsObject(id) || !ObjectID::IsValid(id))
                    continue;
            
            FactionObject* obj;
            if(!level.objects.GetObject(id, obj))
                continue;

            glm::ivec2 pm = glm::ivec2(obj->MinPos());
            glm::ivec2 pM = glm::ivec2(obj->MaxPos()) + 1;
            if(!((pm.x <= M.x && pM.x >= m.x) && (pm.y <= M.y && pM.y >= m.y)))
                continue;
            
            int object_mode = ObjectSelectionType(id, obj->FactionIdx(), playerFactionID);
            if(selection_mode < object_mode) {
                selection_mode = object_mode;
                object_count = 0;
            }

            if(object_mode == selection_mode) {
                if((selection_mode < 3 && object_count < 1) || (selection_mode == 3 && object_count < selection.size())) {
                    selection[object_count++] = id;
                }
            }
        }
        
        //dont update values if there was nothing selected (keep the old selection)
        if(object_count != 0) {
            selected_count = object_count;
            selection_type = selection_mode;
            update_flag = true;
            ENG_LOG_FINE("PlayerSelection::Select - mode {}, count: {}", selection_type, selected_count);

            FactionObject& object = level.objects.GetObject(selection[0]);
            if(object.Sound_Yes().valid && ((selection_type > SelectionType::ENEMY_UNIT) || (selection_type == SelectionType::ENEMY_BUILDING && ((Building&)object).IsGatherable()))) {
                Audio::Play(object.Sound_Yes().Random());
            }
        }
        else {
            ENG_LOG_FINE("PlayerSelection::Select - no change");
        }
    }

    void PlayerSelection::SelectionGroupsHotkey(int keycode, int modifiers) {
        ASSERT_MSG(keycode >= GLFW_KEY_0 && keycode <= GLFW_KEY_9, "Invalid keycode, only number keys are supported");

        group_update_flag = 1 + int(HAS_FLAG(modifiers, 2));
        group_update_idx = keycode - GLFW_KEY_0;
    }

    void PlayerSelection::SelectFromSelection(int id) {
        if(id < 0 || id >= selected_count) {
            ENG_LOG_WARN("PlayerSelection::SelectFromSelection - index out of bounds (idx={}, current selection={})", id, selected_count);
            return;
        }

        selected_count = 1;
        selection[0] = selection[id];
        update_flag = true;
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

    void PlayerSelection::Update(Level& level, GUI::SelectionTab* selectionTab, GUI::ActionButtons* actionButtons, int playerFactionID) {
        //check that all the objects still exist & toss out invalid ones
        int new_count = 0;
        for(int i = 0; i < selected_count; i++) {
            FactionObject* object = nullptr;
            if(!level.objects.GetObject(selection[i], object)) {
                update_flag = true;
            }
            else {
                selection[new_count] = selection[i];
                location[new_count] = { object->RenderPosition(), object->Data()->size };
                new_count++;

                int fID = object->FactionIdx();
                clr_idx = int(playerFactionID != fID) + int(level.factions.Diplomacy().AreHostile(playerFactionID, fID));
            }
        }
        selected_count = new_count;

        
        if(update_flag) {
            selectionTab->Update(level, *this);
            actionButtons->Update(level, *this);
        }
        else {
            selectionTab->ValuesUpdate(level, *this);
            actionButtons->ValuesUpdate(level, *this);
        }

        update_flag = false;
    }

    void PlayerSelection::GroupsUpdate(Level& level) {
        switch(group_update_flag) {
            case 1:     //select group
            {
                int s_type;
                int s_count;
                SelectionData s_data;
                std::tie(s_data, s_count, s_type) = groups[group_update_idx];
                update_flag = true;

                int new_count = 0;
                for(size_t i = 0; i < s_count; i++) {
                    FactionObject* object = nullptr;
                    if(level.objects.GetObject(s_data[i], object)) {
                        s_data[new_count] = s_data[i];

                        if(new_count == 0 && s_type > SelectionType::ENEMY_UNIT && object->Sound_Yes().valid) {
                            Audio::Play(object->Sound_Yes().Random());
                        }

                        new_count++;
                    }

                }
                s_count = new_count;

                if(s_count > 0) {
                    selection = s_data;
                    selection_type = s_type;
                    selected_count = s_count;
                    ENG_LOG_FINE("PlayerSelection::Select - mode {}, count: {}", selection_type, selected_count);
                }
            }
                break;
            case 2:     //override group with current selection
                groups[group_update_idx] = { selection, selected_count, selection_type };
                break;
        }
        group_update_flag = 0;
    }

    int PlayerSelection::ObjectSelectionType(const ObjectID& id, int factionID, int playerFactionID) {
        return 1*int(id.type == ObjectType::UNIT) + 2*int(factionID == playerFactionID);
    }

    bool PlayerSelection::IssueTargetedCommand(Level& level, const glm::ivec2& target_pos, const ObjectID& target_id, const glm::ivec2& cmd_data, GUI::PopupMessage& msg_bar) {
        int command_id = cmd_data.x;
        int payload_id = cmd_data.y;

        //targeted commands can only be issued on player owned units (player buildings have no targeted commands)
        if(selection_type != SelectionType::PLAYER_UNIT)
            return false;
        
        ASSERT_MSG(!GUI::ActionButton_CommandType::IsTargetless(command_id), "Attempting to issue targetless command using a wrong function.");

        static const char* cmd_names[10] = { "Cast", "Build", "Move", "Patrol", "Attack", "AttackGround", "Harvest", "Gather", "Repair", "???" };
        const char* cmd_name = cmd_names[9];
        Command cmd;
        bool conditions_met = true;

        if(GUI::ActionButton_CommandType::IsSingleUser(command_id) && selected_count == 1) {
            // do resource check here(), display message in GUI, if not enough resources
            Unit& unit = level.objects.GetUnit(selection[0]);

            switch(command_id) {
                case GUI::ActionButton_CommandType::CAST:
                    //TODO: resource check (mana on unit), target check, techtree check (is the spell available)
                    cmd = Command::Cast(payload_id);
                    cmd_name = cmd_names[0];
                    break;
                case GUI::ActionButton_CommandType::BUILD:
                {
                    //build site check
                    BuildingDataRef bd = Resources::LoadBuilding(payload_id, bool(unit.Race()));
                    if(!level.map.IsAreaBuildable(target_pos, glm::ivec2(bd->size), bd->navigationType, unit.Position())) {
                        ENG_LOG_FINE("Targeted command - '{}' - invalid position - (pos: ({}, {}), payload: {})", cmd_name, target_pos.x, target_pos.y, payload_id);
                        msg_bar.DisplayMessage("You cannot build there");
                        Audio::Play(SoundEffect::Error().Random());
                        return false;
                    }

                    //TODO: resource check, techtree check

                    cmd = Command::Build(payload_id, target_pos);
                    cmd_name = cmd_names[1];
                    break;
                }
            }
            
            ENG_LOG_FINE("Targeted command - '{}' - pos: ({}, {}), ID: {}, payload: {}", cmd_name, target_pos.x, target_pos.y, target_id, payload_id);
            unit.IssueCommand(cmd);

            if(unit.Sound_Yes().valid) {
                Audio::Play(unit.Sound_Yes().Random());
            }
        }
        else {
            FactionObject* target = nullptr;
            bool target_friendly = false;
            if(ObjectID::IsObject(target_id)) {
                level.objects.GetObject(target_id, target);
                target_friendly = !level.factions.Diplomacy().AreHostile(target->FactionIdx(), level.factions.Player()->ID());
            }
            
            //resolve command type & check command preconditions
            bool check_worker = false;
            bool check_gatherability = false;
            bool valid_target = true;
            switch(command_id) {
                case GUI::ActionButton_CommandType::MOVE:
                    if(!level.map.IsWithinBounds(target_pos))
                        valid_target = false;
                    cmd = Command::Move(target_pos);
                    cmd_name = cmd_names[2];
                    break;
                case GUI::ActionButton_CommandType::PATROL:
                    if(!level.map.IsWithinBounds(target_pos))
                        valid_target = false;
                    cmd = Command::Patrol(target_pos);
                    cmd_name = cmd_names[3];
                    break;
                case GUI::ActionButton_CommandType::ATTACK:
                    if(!ObjectID::IsAttackable(target_id) || !level.map.IsWithinBounds(target_pos))
                        valid_target = false;
                    cmd = Command::Attack(target_id, target_pos);
                    cmd_name = cmd_names[4];
                    break;
                case GUI::ActionButton_CommandType::ATTACK_GROUND:
                    if(!level.map.IsWithinBounds(target_pos))
                        valid_target = false;
                    cmd = Command::AttackGround(target_pos);
                    cmd_name = cmd_names[5];
                    break;
                case GUI::ActionButton_CommandType::HARVEST:
                    check_worker = true;
                    if(ObjectID::IsHarvestable(target_id) && level.map.IsWithinBounds(target_pos)) {
                        cmd = Command::Harvest(target_pos);
                        cmd_name = cmd_names[6];
                    }
                    else if(target != nullptr && target->IsGatherable()) {
                        cmd = Command::Gather(target_id);
                        check_gatherability = true;
                        cmd_name = cmd_names[7];
                    }
                    else
                        valid_target = false;
                    break;
                case GUI::ActionButton_CommandType::REPAIR:
                    check_worker = true;
                    if(target == nullptr || !target->IsRepairable() || !target_friendly)
                        valid_target = false;
                    cmd = Command::Repair(target_id);
                    cmd_name = cmd_names[8];
                    break;
                default:
                    ENG_LOG_WARN("IssueCommand - Unrecognized command_id ({})", command_id);
                    return false;
            }

            ENG_LOG_FINE("Targeted command - pos: ({}, {}), ID: {} (cmd: {}, payload: {})", target_pos.x, target_pos.y, target_id, cmd_name, payload_id);

            for(size_t i = 0; i < selected_count; i++) {
                Unit& unit = level.objects.GetUnit(selection[i]);

                bool conditions_met = valid_target;

                if(check_worker)
                    conditions_met &= unit.IsWorker();
                if(check_gatherability)
                    conditions_met &= (target != nullptr) && target->IsGatherable(unit.NavigationType());
                
                if(conditions_met) {
                    ENG_LOG_FINE("Targeted command - Unit[{}] - issued {}", i, cmd_name);
                    unit.IssueCommand(cmd);
                }
                else {
                    ENG_LOG_FINE("Targeted command - Unit[{}] - conditions failed", i);
                    unit.IssueCommand(Command::Move(target_pos));
                }

                if(i == 0 && unit.Sound_Yes().valid) {
                    Audio::Play(unit.Sound_Yes().Random());
                }
            }
        }

        return true;
    }

    void PlayerSelection::IssueAdaptiveCommand(Level& level, const glm::ivec2& target_pos, const ObjectID& target_id) {
        //adaptive commands can only be issued on player owned units
        if(selection_type != SelectionType::PLAYER_UNIT)
            return;
        ENG_LOG_FINE("Adaptive command - pos: ({}, {}), ID: {}", target_pos.x, target_pos.y, target_id);

        FactionObject* target_object = nullptr;
        if(ObjectID::IsObject(target_id))
            level.objects.GetObject(target_id, target_object);
        bool harvestable = ObjectID::IsHarvestable(target_id);
        bool attackable = ObjectID::IsAttackable(target_id);
        bool enemy = (target_object == nullptr || level.factions.Diplomacy().AreHostile(target_object->FactionIdx(), level.factions.Player()->ID()));
        bool repairable = !enemy && target_object->IsRepairable();

        for(size_t i = 0; i < selected_count; i++) {
            Unit& unit = level.objects.GetUnit(selection[i]);
            bool gatherable = (ObjectID::IsObject(target_id) && level.objects.GetObject(target_id).IsGatherable(unit.NavigationType()));

            //command resolution based on unit type & target type
            if(unit.IsWorker() && gatherable) {
                ENG_LOG_FINE("Adaptive command - Unit[{}] - Gather", i);
                unit.IssueCommand(Command::Gather(target_id));
            }
            else if(unit.IsWorker() && harvestable) {
                ENG_LOG_FINE("Adaptive command - Unit[{}] - Harvest", i);
                unit.IssueCommand(Command::Harvest(target_pos));
            }
            else if(unit.IsWorker() && repairable) {
                ENG_LOG_FINE("Adaptive command - Unit[{}] - Repair", i);
                unit.IssueCommand(Command::Repair(target_id));
            }
            else if(attackable && enemy) {
                ENG_LOG_FINE("Adaptive command - Unit[{}] - Attack", i);
                unit.IssueCommand(Command::Attack(target_id, target_pos));
            }
            else if(level.map.IsWithinBounds(target_pos)) {
                ENG_LOG_FINE("Adaptive command - Unit[{}] - Move", i);
                unit.IssueCommand(Command::Move(target_pos));
            }

            //play unit sound
            if(i == 0 && unit.Sound_Yes().valid) {
                Audio::Play(unit.Sound_Yes().Random());
            }
        }
    }

    void PlayerSelection::IssueTargetlessCommand(Level& level, const glm::ivec2& cmd_data) {
        int command_id = cmd_data.x;
        int payload_id = cmd_data.y;
        ASSERT_MSG(GUI::ActionButton_CommandType::IsTargetless(command_id), "Attempting to issue command that requires targets using a wrong function.");

        bool building_command = GUI::ActionButton_CommandType::IsBuildingCommand(command_id);
        ASSERT_MSG((building_command && selection_type == SelectionType::PLAYER_BUILDING) || (!building_command && selection_type == SelectionType::PLAYER_UNIT), "Invalid selection, cannot invoke targetless commands.");

        if(building_command) {
            Building& building = level.objects.GetBuilding(selection[0]);
            if(command_id == GUI::ActionButton_CommandType::UPGRADE) {
                ENG_LOG_FINE("Targetless command - Upgrade (payload={})", payload_id);
                building.IssueAction(BuildingAction::Upgrade(payload_id));
            }
            else {
                ENG_LOG_FINE("Targetless command - Train/Research (payload={})", payload_id);
                building.IssueAction(BuildingAction::TrainOrResearch(payload_id));
            }
        }
        else {
            static const char* cmd_names[4] = { "Stop", "StandGround", "ReturnGoods", "???" };
            const char* cmd_name = cmd_names[3];
            Command cmd;
            switch(command_id) {
                case GUI::ActionButton_CommandType::STOP:
                    cmd = Command::Idle();
                    cmd_name = cmd_names[0];
                    break;
                case GUI::ActionButton_CommandType::STAND_GROUND:
                    cmd = Command::StandGround();
                    cmd_name = cmd_names[1];
                    break;
                case GUI::ActionButton_CommandType::RETURN_GOODS:
                    cmd = Command::ReturnGoods();
                    cmd_name = cmd_names[2];              
                    break;
            }
            for(size_t i = 0; i < selected_count; i++) {
                Unit& unit = level.objects.GetUnit(selection[i]);
                bool return_goods_condition = unit.IsWorker() && (unit.CarryStatus() != WorkerCarryState::NONE);
                if(command_id != GUI::ActionButton_CommandType::RETURN_GOODS || return_goods_condition) {
                    unit.IssueCommand(cmd);
                    ENG_LOG_FINE("Targetless command - Unit[{}] - {}", i, cmd_name);

                    if(i == 0 && unit.Sound_Yes().valid) {
                        Audio::Play(unit.Sound_Yes().Random());
                    }
                }
            }
        }
    }

    //===== OcclusionMask =====

    OcclusionMask::OcclusionMask(const std::vector<uint32_t>& occlusionData, const glm::ivec2& size_) : size(size_) {
        int area = size.x * size.y;
        data = new int[area];
        for(int i = 0; i < area; i++)
            data[i] = 0;

        if(occlusionData.size() > 0) {
            if((occlusionData.size() * 32) < area) {
                ENG_LOG_WARN("OcclusionMask - loaded data don't cover the entire map (filling with unexplored).");
            }
        }
    }

    OcclusionMask::~OcclusionMask() {
        delete[] data;
    }

    OcclusionMask::OcclusionMask(OcclusionMask&& o) noexcept {
        data = o.data;
        size = o.size;
        o.data = nullptr;
        o.size = glm::ivec2(0);
    }

    OcclusionMask& OcclusionMask::operator=(OcclusionMask&& o) noexcept {
        data = o.data;
        size = o.size;
        o.data = nullptr;
        o.size = glm::ivec2(0);
        return *this;
    }

    int& OcclusionMask::operator()(const glm::ivec2& coords) {
        return operator()(coords.y, coords.x);
    }

    const int& OcclusionMask::operator()(const glm::ivec2& coords) const {
        return operator()(coords.y, coords.x);
    }

    int& OcclusionMask::operator()(int y, int x) {
        ASSERT_MSG((unsigned int)(y) < (unsigned int)(size.x) && (unsigned int)(x) < (unsigned int)(size.x), "Array index ({}, {}) is out of bounds.", y, x);
        return data[y * size.x + x];
    }

    const int& OcclusionMask::operator()(int y, int x) const {
        ASSERT_MSG((unsigned int)(y) < (unsigned int)(size.x) && (unsigned int)(x) < (unsigned int)(size.x), "Array index ({}, {}) is out of bounds.", y, x);
        return data[y * size.x + x];
    }

    //===== MapView =====

    MapView::MapView(const glm::ivec2& size, int scale_) : scale(scale_), tex(std::make_shared<Texture>(TextureParams::CustomData(size.x*scale_, size.y*scale_, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE), nullptr, "mapview")) {
        ENG_LOG_INFO("MAP VIEW - SIZE = ({}, {}), HANDLE: {}", size.x, size.y, tex->Handle());
    }

    void MapView::Update(const Map& map, bool forceRedraw) {
        ASSERT_MSG(tex != nullptr, "MapView is not properly initialized!");
        if((++counter) >= redraw_interval || forceRedraw) {
            counter = 0;
            
            rgba* data = Redraw(map);
            tex->UpdateData((void*)data);
            delete[] data;
        }
    }

    MapView::rgba* MapView::Redraw(const Map& map) const {
        constexpr static std::array<rgba, TileType::COUNT> tileColors = {
            rgba(50,102,15,255),        //GROUND1
            rgba(50,102,15,255),        //GROUND2
            rgba(118,69,4,255),         //MUD1
            rgba(118,69,4,255),         //MUD2
            rgba(34,70,11,255),         //WALL_BROKEN
            rgba(86,51,3,255),          //ROCK_BROKEN
            rgba(50,102,15,255),        //TREES_FELLED
            rgba(4,56,116,255),         //WATER
            rgba(78,65,60,255),         //ROCK
            rgba(117,106,104,255),      //WALL_HU
            rgba(117,106,104,255),      //WALL_HU_DAMAGED
            rgba(117,106,104,255),      //WALL_OC
            rgba(117,106,104,255),      //WALL_OC_DAMAGED
            rgba(14,45,0,255),          //TREES
        };

        constexpr int colorCount = 9;
        constexpr static std::array<rgba, colorCount> factionColors = {
            rgba(164,0,0,255),
            rgba(0,60,192,255),
            rgba(44,180,148,255),
            rgba(152,72,176,255),
            rgba(240,132,20,255),
            rgba(40,40,60,255),
            rgba(224,224,224,255),
            rgba(252,252,72,255),
            rgba(255,0,255,255),
        };

        glm::ivec2 size = map.Size();
        glm::ivec2 sz = size * scale;
        rgba* data = new rgba[sz.x * sz.y];

        for(int y = 0; y < sz.y; y++) {
            int Y = sz.y-1-y;
            for(int x = 0; x < sz.x; x++) {
                glm::ivec2 coords = glm::ivec2(x/scale, y/scale);
                const TileData& td = map(coords);

                rgba color;
                switch(td.occlusion) {
                    default:
                    case 0:     //unexplored bit
                    case 2:
                        color = rgba(0,0,0,255);
                        break;
                    case 3:     //explored & no fog
                        color = ObjectID::IsObject(td.id) ? (factionColors[td.colorIdx % colorCount]) : tileColors[td.tileType];
                        break;
                    case 1:     //explored, but with fog
                    {
                        rgba tmp[2] = { tileColors[td.tileType], rgba(0,0,0,255) };
                        color = ObjectID::IsObject(td.id) ? (factionColors[td.colorIdx % colorCount]) : tmp[int((x+y) % 2 == 0)];
                        break;
                    }
                }
                
                data[Y*sz.x + x] = color;
            }
        }
        return data;
    }

    //===== PlayerFactionController =====

    void PlayerFactionController::GUIRequestHandler::LinkController(const PlayerFactionControllerRef& ctrl) {
        playerController = ctrl;
        playerController->handler = this;
    }

    void PlayerFactionController::GUIRequestHandler::SignalKeyPress(int keycode, int modifiers, bool single_press) {
        ASSERT_MSG(playerController != nullptr, "PlayerController was not linked! Call LinkController()...");
        playerController->OnKeyPressed(keycode, modifiers, single_press);
    }

    PlayerFactionController::PlayerFactionController(FactionsFile::FactionEntry&& entry, const glm::ivec2& mapSize)
        : FactionController(std::move(entry), mapSize), mapview(MapView(mapSize)), occlusion(OcclusionMask(entry.occlusionData, mapSize)) {
        InitializeGUI();
    }

    void PlayerFactionController::Render() {
        //render GUI borders based on player's race
        RenderGUIBorders(bool(Race()), GUI_BORDERS_ZIDX);

        //render the game menu if activated
        if(is_menu_active) {
            menu.Render();
        }

        //render individual GUI elements
        game_panel.Render();
        text_prompt.Render();
        msg_bar.Render();
        resources.Render();
        price.Render();
        RenderMapView();

        switch(state) {
            case PlayerControllerState::OBJECT_SELECTION:
                RenderSelectionRectangle();
                break;
        }

        //render selection highlights
        selection.Render();
    }

    void PlayerFactionController::Update(Level& level) {
        //NEXT UP:
        //ingame menu:
        //  - add the GUI for all the submenus & switching between them
        //  - hookup the logic to the buttons
        //  - finish the keyboard shortcuts for all the menus
        //  - implement game pause
        //  - custom menu visuals (maybe race specific as well)
        //  - retrieve scenario objectives (to show in the menu)

        /*ingame menu data transfers:
            - data to transfer to menu object:
                - scenario objective (currently stored in RecapController, could move it tho), savefile names, config values (sound,preferences,...)
            - transfer from the menu object:
                - config values updates, signal to save/load a game, exit signal
        */


        //should probably also solve the shipyard issue (part on land, part on water)
        //research buttons - mostly figure out their data sources (will probably need to implement train & research command first)
        
        //return goods - why the fuck doesn't he go back to work?
        //add population tracking logic (as described below)

        //occlusion mask - download occlusion mask from map data when Save is issued

        //TODO: implement proper unit speed handling (including the values, so that it matches move speed  from the game)
        //TODO: maybe redo the object sizes (there's map size and graphics size)

        //TODO: might want to wrap menu into a custom class, since there're going to be more panels than one
        //game pausing logic - properly check the single player condition; properly freeze everything that needs to be freezed
        //ingame menu GUI & control logic

        /*population:
            - incrementing population counter on building construction/death seems kinda shitty and prone to errors
            - faction controller will track count for each building type
            - compute population from that (num_town_halls + 4*num_farms)
            - update whenever new building is registered
            - this way, the population will always match the buildings
        */

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

        selection.GroupsUpdate(level);
        resources.Update(level.factions.Player()->Resources(), level.factions.Player()->Population());

        int cursor_idx = CursorIconName::HAND_HU + int(bool(Race()));
        if(!is_menu_active) {
            //update text prompt - content of the button that is being hovered over
            GUI::ImageButton* btn = dynamic_cast<GUI::ImageButton*>(gui_handler.HoveringElement());
            if(btn != nullptr) {
                text_prompt.Setup(btn->Name(), btn->HighlightIdx());
                price.UpdateAlt(btn->Price());
            }
            else {
                text_prompt.Setup("");
                price.ClearAlt();
            }

            //update the GUI - selection & input processing
            gui_handler.Update(&game_panel);

            UpdateState(level, cursor_idx);
        }
        else {
            text_prompt.Setup("");
            price.ClearAlt();
            menu.Update(level, *this, gui_handler);
        }

        //selection data update (& gui updates that display selection)
        selection.Update(level, selectionTab, &actionButtons, ID());

        //unset any previous btn clicks (to detect the new one)
        actionButtons.ClearClick();
        nontarget_cmd_issued = false;
        
        //update cursor icon
        Resources::CursorIcons::SetIcon(cursor_idx);

        //update mapview texture
        mapview.Update(level.map);

        //update timing on the message bar
        msg_bar.Update();
    }

    void PlayerFactionController::SwitchMenu(bool active) {
        ASSERT_MSG(handler != nullptr, "PlayerFactionController not initialized properly!");
        is_menu_active = active;
        menu.OpenTab(GUI::IngameMenuTab::MAIN);
        handler->PauseRequest(active);
    }

    void PlayerFactionController::ChangeLevel(const std::string& filepath) {
        //TODO:

        //invoke level change on the stage controller (handler pointer)
        //there's probably going to some stage switching involved (trigger internally from the stage controller)
        //also switch to main menu if the file is missing or is invalid
    }

    void PlayerFactionController::OnKeyPressed(int keycode, int modifiers, bool single_press) {
        // ENG_LOG_TRACE("KEY PRESSED: {}", keycode);
        if(is_menu_active) {
            menu.OnKeyPressed(keycode, modifiers, single_press);
            return;
        }

        if(!single_press)
            return;

        //group management hotkeys dispatch
        if(keycode >= GLFW_KEY_0 && keycode <= GLFW_KEY_9) {
            selection.SelectionGroupsHotkey(keycode, modifiers);
        }

        //action buttons dispatch
        if((keycode >= GLFW_KEY_A && keycode <= GLFW_KEY_Z) || keycode == GLFW_KEY_ESCAPE) {
            char c = (keycode != GLFW_KEY_ESCAPE) ? char(keycode - GLFW_KEY_A + 'a') : char(255);
            actionButtons.DispatchHotkey(c);
        }

        switch(keycode) {
            case GLFW_KEY_MINUS:
            case GLFW_KEY_KP_SUBTRACT:
                //update game speed config value (should be singlepalyer only)
                break;
            case GLFW_KEY_EQUAL:
            case GLFW_KEY_KP_ADD:
                //update game speed config value (should be singlepalyer only)
                break;
            case GLFW_KEY_F1:
                menu.OpenTab(GUI::IngameMenuTab::HELP);
                is_menu_active = true;
                break;
            case GLFW_KEY_F5:
                menu.OpenTab(GUI::IngameMenuTab::OPTIONS);
                is_menu_active = true;
                break;
            case GLFW_KEY_F7:
                menu.OpenTab(GUI::IngameMenuTab::OPTIONS_SOUND);
                is_menu_active = true;
                break;
            case GLFW_KEY_F8:
                menu.OpenTab(GUI::IngameMenuTab::OPTIONS_SPEED);
                is_menu_active = true;
                break;
            case GLFW_KEY_F9:
                menu.OpenTab(GUI::IngameMenuTab::OPTIONS_PREFERENCES);
                is_menu_active = true;
                break;
            case GLFW_KEY_F10:
                SwitchMenu(true);
                break;
            case GLFW_KEY_F11:
                menu.OpenTab(GUI::IngameMenuTab::SAVE);
                is_menu_active = true;
                break;
            case GLFW_KEY_F12:
                menu.OpenTab(GUI::IngameMenuTab::LOAD);
                is_menu_active = true;
                break;
        }
    }

    void PlayerFactionController::UpdateState(Level& level, int& cursor_idx) {
        Camera& camera = Camera::Get();
        Input& input = Input::Get();

        glm::vec2 hover_coords = glm::ivec2(camera.GetMapCoords(input.mousePos_n * 2.f - 1.f) + 0.5f);
        ObjectID hover_id = level.map.ObjectIDAt(hover_coords);
        bool hovering_over_object = ObjectID::IsValid(hover_id) && !ObjectID::IsHarvestable(hover_id);

        glm::vec2 pos = input.mousePos_n;
        bool cursor_in_game_view = CursorInGameView(pos);
        bool cursor_in_map_view = CursorInMapView(pos);
        switch(state) {
            case PlayerControllerState::IDLE:       //default state
                //lmb.down in game view -> start selection
                //rmb.down in game view -> adaptive command for current selection
                //lmb.down in map view  -> center camera
                //rmb.down in map view  -> adaptive command for current selection
                //cursor at borders     -> move camera
                if(hovering_over_object) {
                    cursor_idx = CursorIconName::MAGNIFYING_GLASS;
                }

                if(actionButtons.ClickIdx() != -1) {
                    ResolveActionButtonClick();

                    if(nontarget_cmd_issued) {
                        selection.IssueTargetlessCommand(level, command_data);
                        command_data = glm::ivec2(-1);
                    }
                }
                else if(cursor_in_game_view) {
                    glm::vec2 coords = camera.GetMapCoords(input.mousePos_n * 2.f - 1.f) + 0.5f;
                    if(input.lmb.down()) {
                        //transition to object selection state
                        coords_start = coords_end = coords;
                        state = PlayerControllerState::OBJECT_SELECTION;
                    }
                    else if (input.rmb.down()) {
                        //issue adaptive command to current selection (target coords from game view click)
                        glm::vec2 target_pos = glm::ivec2(camera.GetMapCoords(input.mousePos_n * 2.f - 1.f) + 0.5f);
                        ObjectID target_id = level.map.ObjectIDAt(target_pos);
                        selection.IssueAdaptiveCommand(level, target_pos, target_id);
                    }
                }
                else if(cursor_in_map_view) {
                    if(input.lmb.down()) {
                        camera.Center(MapView2MapCoords(input.mousePos_n, level.map.Size()));
                        state = PlayerControllerState::CAMERA_CENTERING;
                    }
                    else if (input.rmb.down()) {
                        //issue adaptive command to current selection (target coords from map click)
                        glm::ivec2 target_pos = MapView2MapCoords(input.mousePos_n, level.map.Size());
                        ObjectID target_id = level.map.ObjectIDAt(target_pos);
                        selection.IssueAdaptiveCommand(level, target_pos, target_id);
                    }
                }
                else {
                    CameraPanning(pos);
                }
                break;
            case PlayerControllerState::OBJECT_SELECTION:          //selection in progress (lmb click or drag)
                input.ClampCursorPos(glm::vec2(GUI_WIDTH, BORDER_SIZE), glm::vec2(1.f-BORDER_SIZE, 1.f-BORDER_SIZE));
                cursor_idx = CursorIconName::CROSSHAIR;
                pos = input.mousePos_n;

                coords_end = camera.GetMapCoords(input.mousePos_n * 2.f - 1.f) + 0.5f;
                if(input.lmb.up()) {
                    selection.Select(level, coords_start, coords_end, ID());
                    state = PlayerControllerState::IDLE;
                }
                break;
            case PlayerControllerState::CAMERA_CENTERING:   //camera centering (lmb drag in the map view)
                camera.Center(MapView2MapCoords(input.mousePos_n, level.map.Size()));
                input.ClampCursorPos(MAP_A, MAP_B);
                if(input.lmb.up()) {
                    state = PlayerControllerState::IDLE;
                }
                break;
            case PlayerControllerState::COMMAND_TARGET_SELECTION:
                if(cursor_in_game_view || cursor_in_map_view) {
                    cursor_idx = CursorIconName::EAGLE_YELLOW + int(hovering_over_object);
                }

                //selection canceled by the cancel button (or selected unit died)
                if(!actionButtons.IsAtCancelPage() || selection.selected_count < 1) {
                    BackToIdle();
                }
                else {
                    if(actionButtons.ClickIdx() == 8) {
                        //cancel button clicked
                        BackToIdle();
                    }
                    else if(cursor_in_game_view) {
                        if(input.lmb.down()) {
                            //issue the command & go back to idle state
                            glm::vec2 target_pos = glm::ivec2(camera.GetMapCoords(input.mousePos_n * 2.f - 1.f) + 0.5f);
                            ObjectID target_id = level.map.ObjectIDAt(target_pos);
                            if(selection.IssueTargetedCommand(level, target_pos, target_id, command_data, msg_bar)) {
                                BackToIdle();
                            }
                        }
                        else if (input.rmb.down()) {
                            BackToIdle();
                        }
                        
                        //render building visualization
                        if(command_data.x == GUI::ActionButton_CommandType::BUILD) {
                            Unit* worker;
                            if(level.objects.GetUnit(selection.selection[0], worker)) {
                                RenderBuildingViz(level.map, worker->Position());
                            }
                            else {
                                ENG_LOG_WARN("Invalid selection for build command!");
                                BackToIdle();
                            }
                        }
                    }
                    else if(cursor_in_map_view) {
                        if(input.lmb.down()) {
                            //issue the command
                            glm::ivec2 target_pos = MapView2MapCoords(input.mousePos_n, level.map.Size());
                            ObjectID target_id = level.map.ObjectIDAt(target_pos);
                            if(selection.IssueTargetedCommand(level, target_pos, target_id, command_data, msg_bar)) {
                                BackToIdle();
                            }
                        }
                        else if (input.rmb.down()) {
                            //center the camera at the click pos
                            camera.Center(MapView2MapCoords(input.mousePos_n, level.map.Size()));
                        }
                    }

                    CameraPanning(pos);
                }
                break;
        }
    }

    void PlayerFactionController::ResolveActionButtonClick() {
        if(actionButtons.ClickIdx() < 0)
            return;
            
        const GUI::ActionButtonDescription& btn = actionButtons.ButtonData(actionButtons.ClickIdx());

        switch(btn.command_id) {
            case GUI::ActionButton_CommandType::DISABLED:
                ENG_LOG_WARN("Somehow, you managed to click on a disabled button.");
                break;
            case GUI::ActionButton_CommandType::PAGE_CHANGE:
                //page change -> change page (wow, who would've thought)
                actionButtons.ChangePage(btn.payload_id);
                break;
            case GUI::ActionButton_CommandType::UPGRADE:
            case GUI::ActionButton_CommandType::TRAIN:
            case GUI::ActionButton_CommandType::RESEARCH:
            case GUI::ActionButton_CommandType::STOP:
            case GUI::ActionButton_CommandType::STAND_GROUND:
            case GUI::ActionButton_CommandType::RETURN_GOODS:
                //commands without target, issue immediately on current selection
                command_data = glm::ivec2(btn.command_id, btn.payload_id);
                nontarget_cmd_issued = true;
                break;
            default:
                //targetable commands, move to target selection
                //keep track of the command (or button), watch out for selection changes -> selection cancel?
                //also do conditions check (enough resources), maybe do one after the target is selected as well
                state = PlayerControllerState::COMMAND_TARGET_SELECTION;
                actionButtons.ShowCancelPage();
                command_data = glm::ivec2(btn.command_id, btn.payload_id);
                break;
        }
    }

    void PlayerFactionController::CameraPanning(const glm::vec2& pos) {
        float m = 0.01f;
        float M = 0.99f;
        glm::ivec2 vec = glm::ivec2(int(pos.x > M) - int(pos.x < m), -int(pos.y > M) + int(pos.y < m));
        if(vec.x != 0 || vec.y != 0) {
            Camera::Get().Move(vec);
        }
    }

    bool PlayerFactionController::CursorInGameView(const glm::vec2& pos) const {
        return pos.x > GUI_WIDTH && pos.x < (1.f-BORDER_SIZE) && pos.y > BORDER_SIZE && pos.y < (1.f-BORDER_SIZE);
    }

    bool PlayerFactionController::CursorInMapView(const glm::vec2& pos) const {
        return pos.x > MAP_A.x && pos.x < MAP_B.x && pos.y > MAP_A.y && pos.y < MAP_B.y;
    }

    void PlayerFactionController::BackToIdle() {
        state = PlayerControllerState::IDLE;
        command_data = glm::ivec2(-1);
        actionButtons.ChangePage(0);
    }

    glm::ivec2 PlayerFactionController::MapView2MapCoords(const glm::vec2& pos, const glm::ivec2& mapSize) {
        glm::vec2 r = glm::clamp((pos - MAP_A) / (MAP_B - MAP_A), 0.f, 1.f);
        return glm::vec2(r.x * mapSize.x, (1.f-r.y) * mapSize.y);
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

    void PlayerFactionController::RenderBuildingViz(const Map& map, const glm::ivec2& worker_pos) {
        BuildingDataRef building = Resources::LoadBuilding(command_data.y, bool(Race()));
        SpriteGroup& sg = building->animData->GetGraphics(BuildingAnimationType::IDLE);
        glm::ivec2 sz = glm::ivec2(building->size);

        Camera& cam = Camera::Get();
        Input& input = Input::Get();
        glm::ivec2 corner = glm::ivec2(cam.GetMapCoords(input.mousePos_n * 2.f - 1.f) + 0.5f);
        float zIdx = -0.5f;
        int nav_type = building->navigationType;

        //building sprite
        sg.Render(glm::vec3(cam.map2screen(corner), zIdx), building->size * cam.Mult(), glm::ivec4(0), 0, 0);
        
        //colored overlay, signaling buildability
        for(int y = 0; y < sz.y; y++) {
            for(int x = 0; x < sz.x; x++) {
                glm::ivec2 pos = glm::ivec2(corner.x + x, corner.y + y);
                glm::vec4 clr = map.IsBuildable(pos, nav_type, worker_pos) ? glm::vec4(0.f, 0.62f, 0.f, 1.f) : glm::vec4(0.62f, 0.f, 0.f, 1.f);
                Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(pos), zIdx-1e-3f), cam.Mult(), clr, shadows));
            }
        }
    }

    void PlayerFactionController::RenderMapView() {
        float zIdx = GUI_BORDERS_ZIDX - 2e-2f;
        glm::vec2 pos = MAP_A * 2.f - 1.f;
        glm::vec2 size = (MAP_B - MAP_A) * 2.f;
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(pos.x, -pos.y-size.y, zIdx), size, glm::vec4(1.f), mapview.GetTexture()));
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

        shadows = TextureGenerator::ShadowsTexture(256,256,8);

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

        GUI::StyleRef text_style_small2 = std::make_shared<GUI::Style>();
        text_style_small2->textColor = glm::vec4(1.f);
        text_style_small2->font = font;
        text_style_small2->textScale = 0.7f;
        text_style_small2->color = glm::vec4(0.f);
        text_style_small2->highlightColor = glm::vec4(1.f, 0.f, 0.f, 1.f);

        GUI::StyleRef text_style_big = std::make_shared<GUI::Style>();
        text_style_big->textColor = textClr;
        text_style_big->font = font;
        text_style_big->color = glm::vec4(0.f);
        text_style_big->textScale = 1.f;

        //dbg only - to visualize the element size
        // text_style->color = glm::vec4(1.f);
        // text_style->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, false);
        // text_style_small->color = glm::vec4(1.f);
        // text_style_small->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, false);
        // text_style_small2->color = glm::vec4(1.f);
        // text_style_small2->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, false);

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

        actionButtons = GUI::ActionButtons(new GUI::ImageButtonGrid(glm::vec2(0.5f, 0.675f), glm::vec2(0.475f, 0.3f), 1.f, icon_btn_style, icon, 3, 3, glm::vec2(0.975f / 3.f), this, [](GUI::ButtonCallbackHandler* handler, int id){
            // ENG_LOG_TRACE("ActionButtons - Button [{}, {}] clicked", id % 3, id / 3);
            static_cast<PlayerFactionController*>(handler)->actionButtons.SetClickIdx(id);
        }));

        selectionTab = new GUI::SelectionTab(glm::vec2(0.5f, 0.f), glm::vec2(0.475f, 0.35f), 1.f, 
            borders_style, text_style, text_style_small,
            mana_bar_style, mana_bar_borders_size, progress_bar_style, progress_bar_borders_size, icon_btn_style,
            icon_btn_style, bar_style, bar_border_size, icon, this, [](GUI::ButtonCallbackHandler* handler, int id){
            // ENG_LOG_TRACE("SelectionTab - Button [{}, {}] clicked", id % 3, id / 3);
            static_cast<PlayerFactionController*>(handler)->selection.SelectFromSelection(id);
        });

        game_panel = GUI::Menu(glm::vec2(-1.f, 0.f), glm::vec2(GUI_WIDTH*2.f, 1.f), 0.f, std::vector<GUI::Element*>{
            new GUI::TextButton(glm::vec2(0.25f, -0.95f), glm::vec2(0.2f, 0.03f), 1.f, menu_btn_style, "Menu", this, [](GUI::ButtonCallbackHandler* handler, int id){
                static_cast<PlayerFactionController*>(handler)->SwitchMenu(true);
            }),
            new GUI::TextButton(glm::vec2(0.75f, -0.95f), glm::vec2(0.2f, 0.03f), 1.f, menu_btn_style, "Pause", this, [](GUI::ButtonCallbackHandler* handler, int id){
                ASSERT_MSG(static_cast<PlayerFactionController*>(handler)->handler != nullptr, "PlayerFactionController is not initialized properly!");
                static_cast<PlayerFactionController*>(handler)->handler->PauseToggleRequest();
            }),

            actionButtons.Buttons(),
            selectionTab,
        });

        float wr = Window::Get().Ratio();
        float b = BORDER_SIZE * wr;
        float w = 2.f * GUI_WIDTH;
        float xOff = 0.01f;
        resources = GUI::ResourceBar(glm::vec2(-1.f + GUI_WIDTH*2.f + xOff + (1.f - GUI_WIDTH),-1.f + b), glm::vec2(1.f - GUI_WIDTH - xOff, b), 1.f, text_style_small2, icon);
        price     = GUI::ResourceBar(glm::vec2( 1.f - GUI_WIDTH * 1.5f, 1.f - b), glm::vec2(GUI_WIDTH * 1.5f, b), 1.f, text_style_small2, icon, true);
        text_prompt = GUI::TextLabel(glm::vec2(-1.f + GUI_WIDTH*2.f + xOff + (1.f - GUI_WIDTH), 1.f - b), glm::vec2(1.f - GUI_WIDTH - xOff, b*0.9f), 1.f, text_style, "TEXT PROMPT", false);
        msg_bar     = GUI::PopupMessage(glm::vec2(-1.f + GUI_WIDTH*2.f + 2.f*xOff + (1.f - GUI_WIDTH), 1.f - b*5.f), glm::vec2(1.f - GUI_WIDTH - xOff, b*0.9f), 1.f, text_style);

        menu = GUI::IngameMenu(glm::vec2(GUI_WIDTH, 0.f), glm::vec2(1.5f * GUI_WIDTH, 0.666f), 0.f, this, menu_style, menu_style, text_style_big);
    }



    void PlayerFactionController::Inner_DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        // ImGui::ColorEdit4("shadows", (float*)&clr);
#endif
    }

    //==============================================================

    void RenderGUIBorders(bool isOrc, float z) {
        Window& window = Window::Get();

        //render GUI overlay (borders)
        float w = GUI_WIDTH*2.f;
        float b = BORDER_SIZE*2.f;

        //TODO: modify based on race (at least the color, if not using textures)
        glm::vec4 clr = isOrc ? glm::vec4(0.3f, 0.16f, 0.f, 1.f) : glm::vec4(0.33f, 0.33f, 0.33f, 1.f);

        //gui panel background
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(-1.f, -1.f, z), glm::vec2(GUI_WIDTH*2.f, 2.f), clr));

        //borders
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(1.f - b, -1.f, z-2e-3f), glm::vec2(b, 2.f), clr));
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(-1.f + w, -1.f, z-1e-3f), glm::vec2(2.f - w, b*window.Ratio()), clr));
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(-1.f + w,  1.f-b*window.Ratio(), z-1e-3f), glm::vec2(2.f - w, b*window.Ratio()), clr));
    }

    std::vector<GUI::StyleRef> SetupScrollMenuStyles(const FontRef& font, const glm::vec2& scrollMenuSize, int scrollMenuItems, float scrollButtonSize, const glm::vec2& smallBtnSize) {
        GUI::StyleRef s = nullptr;
        std::vector<GUI::StyleRef> res;

        //menu item style
        glm::vec2 buttonSize = glm::vec2(scrollMenuSize.x, scrollMenuSize.y / scrollMenuItems);
        glm::vec2 ts = glm::vec2(Window::Get().Size()) * buttonSize;
        float upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
        glm::ivec2 textureSize = ts * upscaleFactor;
        int shadingWidth = 2 * upscaleFactor;

        s = std::make_shared<GUI::Style>();
        s->font = font;
        s->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, 0, shadingWidth, 0, false);
        s->hoverTexture = s->texture;
        s->holdTexture = s->texture;
        s->highlightTexture = s->texture;
        s->highlightMode = GUI::HighlightMode::TEXT;
        s->textColor = textClr;
        s->textAlignment = GUI::TextAlignment::LEFT;
        s->textScale = 0.8f;
        // s->hoverColor = textClr;
        s->hoverColor = glm::vec4(1.f);
        // s->color = glm::vec4(0.f);
        res.push_back(s);

        s = std::make_shared<GUI::Style>();
        s->font = font;
        s->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, 0, shadingWidth, 1, false);
        s->hoverTexture = s->texture;
        s->holdTexture = s->texture;
        s->highlightTexture = s->texture;
        s->highlightMode = GUI::HighlightMode::TEXT;
        s->textColor = textClr;
        s->textAlignment = GUI::TextAlignment::LEFT;
        s->textScale = 0.8f;
        // s->hoverColor = textClr;
        s->hoverColor = glm::vec4(1.f);
        // s->color = glm::vec4(0.f);
        GUI::StyleRef disabled_btn = s;

        //small button style
        buttonSize = smallBtnSize;
        ts = glm::vec2(Window::Get().Size()) * buttonSize;
        upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
        textureSize = ts * upscaleFactor;
        shadingWidth = 2 * upscaleFactor;

        //scroll up button style
        buttonSize = glm::vec2(scrollButtonSize, scrollButtonSize);
        ts = glm::vec2(Window::Get().Size()) * buttonSize;
        upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
        textureSize = ts * upscaleFactor;
        shadingWidth = 2 * upscaleFactor;
        s = std::make_shared<GUI::Style>();
        s->texture = TextureGenerator::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, false, true);
        s->hoverTexture = s->texture;
        s->holdTexture = TextureGenerator::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, true, true);
        s->highlightMode = GUI::HighlightMode::NONE;
        res.push_back(s);

        s = std::make_shared<GUI::Style>();
        s->texture = TextureGenerator::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, false, false);
        s->hoverTexture = s->texture;
        s->holdTexture = TextureGenerator::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, true, false);
        s->highlightMode = GUI::HighlightMode::NONE;
        res.push_back(s);

        s = std::make_shared<GUI::Style>();
        GUI::StyleRef z = s;
        s->texture = TextureGenerator::ButtonTexture_Gem(textureSize.x, textureSize.y, shadingWidth, Window::Get().Ratio());
        s->hoverTexture = s->texture;
        s->holdTexture = s->texture;
        s->highlightMode = GUI::HighlightMode::NONE;


        //scroll slider style
        buttonSize = glm::vec2(scrollButtonSize, scrollMenuSize.y);
        ts = glm::vec2(Window::Get().Size()) * buttonSize;
        upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
        textureSize = ts * upscaleFactor;
        shadingWidth = 2 * upscaleFactor;
        s = std::make_shared<GUI::Style>();
        s->texture = TextureGenerator::ButtonTexture_Clear(textureSize.x, textureSize.y, shadingWidth, 0, 0, false);
        s->hoverTexture = s->texture;
        s->holdTexture = s->texture;
        s->highlightMode = GUI::HighlightMode::NONE;
        res.push_back(s);
        res.push_back(z);
        res.push_back(disabled_btn);

        return res;
    }

}//namespace eng
