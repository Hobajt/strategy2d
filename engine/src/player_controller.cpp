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

#define CLICK_SELECTION_V2

namespace eng {

    constexpr float GUI_WIDTH = 0.25f;
    constexpr float BORDER_SIZE = 0.025f;

    constexpr float GUI_BORDERS_ZIDX = -0.89f;

    constexpr float SELECTION_ZIDX = GUI_BORDERS_ZIDX + 0.01f;
    constexpr float SELECTION_HIGHLIGHT_WIDTH = 0.025f;

    constexpr float DOUBLE_CLICK_DELAY = 0.2f;

    static glm::vec4 textClr = glm::vec4(0.92f, 0.77f, 0.20f, 1.f);

    constexpr glm::vec2 MAP_A = glm::vec2(0.025f,                0.05f);
    constexpr glm::vec2 MAP_B = glm::vec2(0.025f+GUI_WIDTH*0.8f, 0.3f);

    void format_wBonus(char* buf, size_t buf_size, const char* prefix, int val_bonus, glm::ivec2& out_highlight_idx);

    void RenderGUIBorders(bool isOrc, float z);
    std::vector<GUI::StyleRef> SetupScrollMenuStyles(const FontRef& font, const glm::vec2& scrollMenuSize, int scrollMenuItems, float scrollButtonSize, const glm::vec2& smallBtnSize);
    std::vector<GUI::StyleRef> SetupSliderStyles(const FontRef& font, const glm::vec2& scrollMenuSize, int scrollMenuItems, float scrollButtonSize, const glm::vec2& smallBtnSize);

    //===== GUI::SelectionTab =====

    GUI::SelectionTab::SelectionTab(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, 
        const StyleRef& borders_style_, const StyleRef& text_style, const StyleRef& text_style_small,
        const StyleRef& mana_bar_style_, const glm::vec2& mana_borders_size_, const StyleRef& progress_bar_style_, const glm::vec2& progress_borders_size_, const StyleRef& passive_btn_style_,
        const StyleRef& btn_style_, const StyleRef& bar_style_, const glm::vec2& bar_borders_size_, const Sprite& sprite_, ButtonCallbackHandler* handler_, ButtonCallbackType callback_)
        : Element(offset_, size_, zOffset_, Style::Default(), nullptr) {
        
        //icon button grid, when there's multiple objects selected
        btns = static_cast<ImageButtonGrid*>(AddChild(
            new ImageButtonGrid(
                glm::vec2(0.f, 0.f), glm::vec2(1.f, 1.f), 0.f, btn_style_, bar_style_, bar_borders_size_, sprite_, 
                3, 3, glm::vec2(0.975f / 3.f, 0.9f / 3.f), handler_, callback_), 
            true
        ));

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
        char buf_tmp[1024];
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

            bool is_upgraded_tower = object->NumID()[1] == BuildingType::GUARD_TOWER || object->NumID()[1] == BuildingType::CANNON_TOWER;
            if(selection.selection_type >= SelectionType::PLAYER_BUILDING && object->IsTransport()) {
                transport_load = static_cast<Unit*>(object)->Transport_CurrentLoad();
            }

            if(selection.selection_type >= SelectionType::PLAYER_BUILDING && (object->IsUnit() || is_upgraded_tower)) {
                glm::ivec2 highlight_idx = glm::ivec2(-1);

                if(transport_load != 0) {
                    Unit* unit = static_cast<Unit*>(object);
                    health->Enable(false);

                    snprintf(buf, sizeof(buf), "Level %d", unit->UnitLevel());
                    level_text->Setup(std::string(buf));

                    //fetch the IDs of units inside
                    transport_ids.clear();
                    level.objects.GetUnitsInsideObject(unit->OID(), transport_ids);
                    ASSERT_MSG(transport_ids.size() == transport_load, "GUI::SelectionTab::Update - transport ship load doesn't match the number of detected units onboard.");

                    //update the buttons to reflect each unit
                    int obj_count = std::min((int)transport_ids.size(), TRANSPORT_MAX_LOAD);
                    for(int i = 0; i < obj_count; i++) {
                        FactionObject* object = &level.objects.GetObject(transport_ids[i]);
                        btns->GetButton(3+i)->Setup(object->Name(), -1, object->Icon(), object->HealthPercentage());
                    }
                }
                else {
                    snprintf(buf_tmp, sizeof(buf_tmp), "Armor: %d", object->BaseArmor());
                    format_wBonus(buf, sizeof(buf), buf_tmp, object->BonusArmor(), highlight_idx);
                    stats[0]->Setup(buf, highlight_idx);

                    snprintf(buf_tmp, sizeof(buf_tmp), "Damage: %d-%d", object->BaseMinDamage(), object->BaseMaxDamage());
                    format_wBonus(buf, sizeof(buf), buf_tmp, object->BonusDamage(), highlight_idx);
                    stats[1]->Setup(buf, highlight_idx);

                    snprintf(buf_tmp, sizeof(buf_tmp), "Range: %d", object->BaseAttackRange());
                    format_wBonus(buf, sizeof(buf), buf_tmp, object->BonusAttackRange(), highlight_idx);
                    stats[2]->Setup(buf, highlight_idx);

                    snprintf(buf_tmp, sizeof(buf_tmp), "Sight: %d", object->BaseVisionRange());
                    format_wBonus(buf, sizeof(buf), buf_tmp, object->BonusVisionRange(), highlight_idx);
                    stats[3]->Setup(buf, highlight_idx);

                    if(object->IsUnit()) {
                        Unit* unit = static_cast<Unit*>(object);

                        snprintf(buf, sizeof(buf), "Level %d", unit->UnitLevel());
                        level_text->Setup(std::string(buf));

                        snprintf(buf, sizeof(buf), "Speed: %d", (int)unit->MoveSpeed());
                        stats[4]->Setup(buf);

                        if(unit->IsCaster()) {
                            stats[5]->Setup("Magic:");
                            snprintf(buf, sizeof(buf), "%d", unit->Mana());
                            mana_bar->Setup(unit->ManaPercentage(), buf);
                        }
                    }
                }
            }
            else {
                Building* building = static_cast<Building*>(object);
                int not_gnd = int(building->NavigationType() != NavigationBit::GROUND);
                if(building->IsResource()) {
                    //resource -> amount of resource left
                    snprintf(buf, sizeof(buf), "%s left: %d", GameResourceName(2*not_gnd), building->AmountLeft());
                    stats[1]->Setup(buf);

                    if(building->ProgressableAction()) {
                        //progressable action (construction, upgrade, research, training) -> display progress bar
                        production_bar->Setup(building->ActionProgress(), "% Complete");
                        progress_bar_flag = true;
                        if(building->Constructed()) {
                            //not construction -> show icon of the target & label
                            stats[1]->Setup(building->IsTraining() ? "Training:" : "Upgrading:");
                            production_icon->Setup("icon", -1, building->LastBtnIcon(), glm::ivec4(0));
                        }
                    }
                }
                else if(selection.selection_type >= SelectionType::PLAYER_BUILDING) {
                    if(building->ProgressableAction()) {
                        //progressable action (construction, upgrade, research, training) -> display progress bar
                        production_bar->Setup(building->ActionProgress(), "% Complete");
                        progress_bar_flag = true;
                        if(building->Constructed()) {
                            //not construction -> show icon of the target & label
                            stats[1]->Setup(building->IsTraining() ? "Training:" : "Upgrading:");
                            production_icon->Setup("icon", -1, building->LastBtnIcon(), glm::ivec4(0));
                        }
                    }
                    else {
                        //show additional building info (production/population stats)
                        int production_boost;
                        glm::ivec2 highlight_idx = glm::ivec2(-1);
                        switch(building->NumID()[1]) {
                            case BuildingType::TOWN_HALL:
                            case BuildingType::KEEP:
                            case BuildingType::CASTLE:
                                stats[0]->Setup("Production ");
                                for(int i = 0; i < 3; i++) {
                                    production_boost = building->Faction()->ProductionBoost(i);
                                    snprintf(buf_tmp, sizeof(buf_tmp), "%s: 100", GameResourceName(i));
                                    format_wBonus(buf, sizeof(buf), buf_tmp, production_boost, highlight_idx);
                                    stats[1+i]->Setup(buf, highlight_idx);
                                }
                                break;
                            case BuildingType::OIL_REFINERY:
                            case BuildingType::LUMBER_MILL:
                                stats[1]->Setup("Production ");
                                production_boost = building->Faction()->ProductionBoost(1+not_gnd);
                                snprintf(buf_tmp, sizeof(buf_tmp), "%s: 100", GameResourceName(1+not_gnd));
                                format_wBonus(buf, sizeof(buf), buf_tmp, production_boost, highlight_idx);
                                stats[2]->Setup(buf, highlight_idx);
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
                if(transport_load == 0)
                    health->Setup(std::string(buf));

                if(!object->IsUnit()) {
                    if(progress_bar_flag) {
                        Building* building = static_cast<Building*>(object);
                        if(building->ProgressableAction()) {
                            production_bar->SetValue(building->ActionProgress());
                        }
                        else {
                            //refresh the entire gui if the action is finished
                            Update(level, selection);
                        }
                    }
                }
                else {
                    Unit* unit = static_cast<Unit*>(object);
                    if(unit->IsCaster()) {
                        snprintf(buf, sizeof(buf), "%d", unit->Mana());
                        mana_bar->Setup(unit->ManaPercentage(), buf);
                    }
                }
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

    ObjectID GUI::SelectionTab::GetTransportID(int btnID) const {
        int id = btnID-3;
        ASSERT_MSG(((unsigned)id < transport_ids.size()), "SelectionTab::GetTransportID - id {} out of bounds (max {})", id, (int)transport_ids.size());
        return transport_ids.at(id);
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

        progress_bar_flag = false;
        transport_load = 0;
        transport_ids.clear();
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
        if(selection.selection_type < SelectionType::PLAYER_BUILDING || selection.selected_count < 1) {
            ChangePage(0);
            return;
        }

        if(selection.selection_type == SelectionType::PLAYER_UNIT) {
            //scan through the selection & obtain properties that define how the GUI will look
            bool can_attack = false;
            bool all_workers = true;
            bool carrying_goods = false;
            bool can_attack_ground = false;
            bool all_water_units = true;
            bool all_transports = true;
            bool transport_has_load = false;
            bool all_casters = true;
            int att_upgrade_src = UnitUpgradeSource::NONE;
            int def_upgrade_src = UnitUpgradeSource::NONE;
            FactionControllerRef faction = nullptr;
            bool isOrc = false;

            for(size_t i = 0; i < selection.selected_count; i++) {
                Unit& unit = level.objects.GetUnit(selection.selection[i]);

                all_workers         &= unit.IsWorker();
                all_water_units     &= (unit.NavigationType() == NavigationBit::WATER);
                can_attack          |= unit.CanAttack();
                carrying_goods      |= unit.IsWorker() && (unit.CarryStatus() != WorkerCarryState::NONE);
                can_attack_ground   |= unit.IsSiege();
                all_transports      &= unit.IsTransport();
                transport_has_load  |= !unit.Transport_IsEmpty();
                all_casters         &= unit.IsCaster() && (unit.AttackUpgradeSource() == UnitUpgradeSource::CASTER);

                if(i == 0) {
                    att_upgrade_src = unit.AttackUpgradeSource();
                    def_upgrade_src = unit.DefenseUpgradeSource();
                    faction = unit.Faction();
                    isOrc = unit.IsOrc();
                }
                else {
                    if(att_upgrade_src != unit.AttackUpgradeSource())  att_upgrade_src = UnitUpgradeSource::NONE;
                    if(def_upgrade_src != unit.DefenseUpgradeSource()) def_upgrade_src = UnitUpgradeSource::NONE;
                }
            }

            att_upgrade_src = (att_upgrade_src == UnitUpgradeSource::BLACKSMITH_BALLISTA) ? UnitUpgradeSource::BLACKSMITH : att_upgrade_src;
            def_upgrade_src = (def_upgrade_src >  UnitUpgradeSource::FOUNDRY) ? UnitUpgradeSource::BLACKSMITH : def_upgrade_src;

            //add default unit commands for unit selection
            p[0] = ActionButtonDescription::Move(isOrc, all_water_units);
            p[1] = ActionButtonDescription::Stop(isOrc, def_upgrade_src, faction->UnitUpgradeTier(false, def_upgrade_src, isOrc));
            if(!all_transports && !all_casters) {
                p[3] = ActionButtonDescription::Patrol(isOrc, all_water_units);
                p[4] = ActionButtonDescription::StandGround(isOrc);
            }
            else if(transport_has_load && selection.selected_count == 1) {
                p[2] = ActionButtonDescription::TransportUnload(isOrc);
            }

            //attack command only if there's at least one unit that can attack
            if(can_attack) {
                p[2] = ActionButtonDescription::Attack(isOrc, att_upgrade_src, faction->UnitUpgradeTier(true, att_upgrade_src, isOrc));
            }

            //repair & harvest if they're all workers
            if(all_workers) {
                if(!all_water_units)
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

        buildingAction_inProgress = false;
        if(selection.selection_type == SelectionType::PLAYER_BUILDING) {
            Building& building = level.objects.GetBuilding(selection.selection[0]);
            buildingAction_inProgress = building.ProgressableAction();
        }

        //display specialized buttons only if there's no more than 1 object in the selection
        if(selection.selected_count == 1) {
            //retrieve object_data reference & page descriptions from that
            FactionObject& object = level.objects.GetObject(selection.selection[0]);
            const ButtonDescriptions& desc = object.GetButtonDescriptions();

            bool selection_isOrc = level.objects.GetObject(selection.selection[0]).IsOrc();
            PlayerFactionControllerRef player = level.factions.Player();

            int p_idx = 0;
            for(const ButtonDescriptions::PageEntry& page_desc : desc.pages) {
                if(p_idx > 0) {
                    ResetPage(p_idx);
                    pages[p_idx][8] = ActionButtonDescription::CancelButton();
                }
                PageData& current_page = pages[p_idx++];

                for(const auto& [pos_idx, btn_data] : page_desc) {
                    GUI::ActionButtonDescription btn = btn_data;
                    //override only after conditions are checked - to allow multiple button definitions on the same slot
                    if(player->ActionButtonSetup(btn, selection_isOrc)) {
                        current_page[pos_idx] = btn;
                    }
                }
            }
        }
        
        if(!buildingAction_inProgress)
            ChangePage(0);
        else
            ShowCancelPage();
    }

    void GUI::ActionButtons::ValuesUpdate(Level& level, const PlayerSelection& selection) {
        if(selection.selected_count < 1)
            return;

        //full refresh if building action was finished
        if(selection.selection_type == SelectionType::PLAYER_BUILDING && buildingAction_inProgress) {
            Building& building = level.objects.GetBuilding(selection.selection[0]);
            if(!building.ProgressableAction()) {
                Update(level, selection);
                return;
            }
        }
        
        PageData& pg = pages[page];
        FactionObject& obj = level.objects.GetObject(selection.selection[0]);
        FactionControllerRef player = level.factions.Player();
        
        //iterate through all the buttons (in active page) & enable/disable them based on whether the button action conditions are met
        for(size_t i = 0; i < pg.size(); i++) {
            ActionButtonDescription btn = pg[i];
            bool conditions_met = (btn.command_id != ActionButton_CommandType::DISABLED);
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

        //display gold/wood/oil price
        bool values_displayed = false;
        for(size_t i = 0; i < 3; i++) {
            if(price[i] <= 0) {
                text[i]->Enable(false);
                continue;
            }
            values_displayed = true;
            snprintf(buf, sizeof(buf), "%d", price[i]);
            text[i]->Setup(IconIdx(i), std::string(buf));
        }

        //display mana cost instead
        if(!values_displayed && price[3] > 0) {
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
        std::vector<GUI::StyleRef> slider_styles = SetupSliderStyles(text_style->font, scrollMenuSize, scrollMenuItems, scrollBtnSize, smallBtnSize);
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

        menus.insert({ IngameMenuTab::END_SCENARIO, Menu(offset, size, zOffset, bg_style, std::vector<GUI::Element*>{
                new GUI::TextLabel(glm::vec2(0.f, -0.85f), glm::vec2(bw, bh), 1.f, text_style, "End Scenario"),
                new GUI::TextButton(glm::vec2(0.f, -0.65f), glm::vec2(bw, bh), 1.f, btn_style, "Load Custom", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->SetupConfirmScreen("Are you sure you\nwant to load a\ncustom scenario?", "Load Custom", glm::ivec2(0, 1), GLFW_KEY_L);
                }, glm::vec2(0, 1)),
                new GUI::TextButton(glm::vec2(0.f, -0.45f), glm::vec2(bw, bh), 1.f, btn_style, "Surrender", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->SetupConfirmScreen("Are you sure you\nwant to surrender\nto your enemies?", "Surrender", glm::ivec2(0, 1), GLFW_KEY_S);
                }, glm::vec2(0, 1)),
                new GUI::TextButton(glm::vec2(0.f, -0.25f), glm::vec2(bw, bh), 1.f, btn_style, "Quit to Menu", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->SetupConfirmScreen("Are you sure you\nwant to quit to\nthe main menu?", "Quit to Menu", glm::ivec2(0, 1), GLFW_KEY_Q);
                }, glm::vec2(0, 1)),
                new GUI::TextButton(glm::vec2(0.f, -0.05f), glm::vec2(bw, bh), 1.f, btn_style, "Exit Program", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->SetupConfirmScreen("Are you sure you\nwant to exit\nthe game?", "Exit Program", glm::ivec2(1, 2), GLFW_KEY_X);
                }, glm::vec2(1, 2)),
                new GUI::TextButton(glm::vec2( 0.f, 0.7f), glm::vec2(bw, bh), 1.f, btn_style, "Previous (Esc)", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::MAIN);
                }, glm::ivec2(16,19)),
            })
        });

        menus.insert({ IngameMenuTab::CONFIRM_SCREEN, Menu(offset, size, zOffset, bg_style, std::vector<GUI::Element*>{
                new GUI::ScrollText(glm::vec2(0.f, -0.7f), glm::vec2(bw, 0.25f), 1.f, text_style, ""),
                new GUI::TextButton(glm::vec2(0.f, -0.35f), glm::vec2(bw, bh), 1.f, btn_style, "Confirm", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->EndGame();
                }, glm::vec2(0, 1)),
                new GUI::TextButton(glm::vec2( 0.f, 0.7f), glm::vec2(bw, bh), 1.f, btn_style, "Cancel (Esc)", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::MAIN);
                }, glm::ivec2(8,11)),
            })
        });
        confirm_label = dynamic_cast<ScrollText*>(menus.at(IngameMenuTab::CONFIRM_SCREEN).GetChild(0));
        confirm_btn = dynamic_cast<TextButton*>(menus.at(IngameMenuTab::CONFIRM_SCREEN).GetChild(1));

        menus.insert({ IngameMenuTab::GAME_OVER_SCREEN, Menu(offset, glm::vec2(size.x, size.y * 0.333f), zOffset, bg_style, std::vector<GUI::Element*>{
                new GUI::ScrollText(glm::vec2(0.f, -0.7f), glm::vec2(bw, 0.25f), 1.f, text_style, "End message"),
                new GUI::TextButton(glm::vec2(0.f, -0.35f), glm::vec2(bw, bh*3.f), 1.f, btn_style, "Confirm", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->GameOver();
                }, glm::vec2(0, 1))
            })
        });
        gameover_label = dynamic_cast<ScrollText*>(menus.at(IngameMenuTab::GAME_OVER_SCREEN).GetChild(0));
        gameover_btn = dynamic_cast<TextButton*>(menus.at(IngameMenuTab::GAME_OVER_SCREEN).GetChild(1));

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

                new GUI::TextLabel(glm::vec2(0.f, -0.7f), glm::vec2(bw, bh), 1.f, text_style, "Master Volume", false),
                new GUI::ValueSlider(glm::vec2(0.f, -0.6f), glm::vec2(bw, bh*0.5f), 1.f, scrollBtnSize, slider_styles, glm::vec2(0.f, 100.f), 100),
                new GUI::TextLabel(glm::vec2(0.f, -0.5f), glm::vec2(bw, bh), 1.f, text_style, "Digital Volume", false),
                new GUI::ValueSlider(glm::vec2(0.f, -0.4f), glm::vec2(bw, bh*0.5f), 1.f, scrollBtnSize, slider_styles, glm::vec2(0.f, 100.f), 100),
                new GUI::TextLabel(glm::vec2(0.f, -0.3f), glm::vec2(bw, bh), 1.f, text_style, "Music Volume", false),
                new GUI::ValueSlider(glm::vec2(0.f, -0.2f), glm::vec2(bw, bh*0.5f), 1.f, scrollBtnSize, slider_styles, glm::vec2(0.f, 100.f), 100),
                new GUI::TextButton(glm::vec2(-0.5f, 0.7f), glm::vec2(bw*0.45f, bh), 1.f, btn_style, "Cancel", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::MAIN);
                }, glm::ivec2(0,1)),
                new GUI::TextButton(glm::vec2( 0.5f, 0.7f), glm::vec2(bw*0.45f, bh), 1.f, btn_style, "OK", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    IngameMenu* menu = static_cast<IngameMenu*>(handler);
                    Config::Audio().UpdateSounds(menu->sound_master->Value(), menu->sound_digital->Value(), menu->sound_music->Value(), true);
                    menu->OpenTab(IngameMenuTab::MAIN);
                }, glm::ivec2(0,1)),
            })
        });
        sound_digital = (ValueSlider*)menus.at(IngameMenuTab::OPTIONS_SOUND).GetChild(4);
        sound_master = (ValueSlider*)menus.at(IngameMenuTab::OPTIONS_SOUND).GetChild(2);
        sound_music = (ValueSlider*)menus.at(IngameMenuTab::OPTIONS_SOUND).GetChild(6);

        menus.insert({ IngameMenuTab::OPTIONS_SPEED, Menu(offset, size, zOffset, bg_style, std::vector<GUI::Element*>{
                new GUI::TextLabel(glm::vec2(0.f, -0.85f), glm::vec2(bw, bh), 1.f, text_style, "Speed Settings"),

                new GUI::TextLabel(glm::vec2(0.f, -0.7f), glm::vec2(bw, bh), 1.f, text_style, "Game Speed", false),
                new GUI::ValueSlider(glm::vec2(0.f, -0.6f), glm::vec2(bw, bh*0.5f), 1.f, scrollBtnSize, slider_styles, glm::vec2(0.6f, 6.f), 10),
                new GUI::TextLabel(glm::vec2(0.f, -0.5f), glm::vec2(bw, bh), 1.f, text_style, "Mouse Scroll", false),
                new GUI::ValueSlider(glm::vec2(0.f, -0.4f), glm::vec2(bw, bh*0.5f), 1.f, scrollBtnSize, slider_styles, glm::vec2(0.1f, 10.f), 10),
                new GUI::TextLabel(glm::vec2(0.f, -0.3f), glm::vec2(bw, bh), 1.f, text_style, "Key Scroll", false),
                new GUI::ValueSlider(glm::vec2(0.f, -0.2f), glm::vec2(bw, bh*0.5f), 1.f, scrollBtnSize, slider_styles, glm::vec2(0.1f, 10.f), 10),
                new GUI::TextButton(glm::vec2(-0.5f, 0.7f), glm::vec2(bw*0.45f, bh), 1.f, btn_style, "Cancel", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::MAIN);
                }, glm::ivec2(0,1)),
                new GUI::TextButton(glm::vec2( 0.5f, 0.7f), glm::vec2(bw*0.45f, bh), 1.f, btn_style, "OK", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    IngameMenu* menu = static_cast<IngameMenu*>(handler);
                    Config::UpdateSpeeds(menu->speed_game->Value(), menu->speed_mouse->Value(), menu->speed_keys->Value(), true);
                    menu->OpenTab(IngameMenuTab::MAIN);
                }, glm::ivec2(0,1)),
            })
        });
        speed_game = (ValueSlider*)menus.at(IngameMenuTab::OPTIONS_SPEED).GetChild(2);
        speed_mouse = (ValueSlider*)menus.at(IngameMenuTab::OPTIONS_SPEED).GetChild(4);
        speed_keys = (ValueSlider*)menus.at(IngameMenuTab::OPTIONS_SPEED).GetChild(6);

        menus.insert({ IngameMenuTab::OPTIONS_PREFERENCES, Menu(offset, size, zOffset, bg_style, std::vector<GUI::Element*>{
                new GUI::TextLabel(glm::vec2(0.f, -0.85f), glm::vec2(bw, bh), 1.f, text_style, "Preferences"),

                new GUI::TextLabel(glm::vec2(0.f, -0.7f), glm::vec2(bw, bh), 1.f, text_style, "Fog of War:", false),
                new GUI::Radio(glm::vec2(-0.5f, -0.6f), glm::vec2(0.05f, 0.0375f), 1.f, slider_styles[4], slider_styles[3], { "On", "Off" }, 0),
                new GUI::TextButton(glm::vec2(-0.5f, 0.7f), glm::vec2(bw*0.45f, bh), 1.f, btn_style, "Cancel", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    static_cast<IngameMenu*>(handler)->OpenTab(IngameMenuTab::MAIN);
                }, glm::ivec2(0,1)),
                new GUI::TextButton(glm::vec2( 0.5f, 0.7f), glm::vec2(bw*0.45f, bh), 1.f, btn_style, "OK", this, [](GUI::ButtonCallbackHandler* handler, int id){
                    IngameMenu* menu = static_cast<IngameMenu*>(handler);
                    Config::UpdatePreferences(menu->fog_of_war->IsSelected(0), true);
                    menu->OpenTab(IngameMenuTab::MAIN);
                }, glm::ivec2(0,1)),
            })
        });
        fog_of_war = (Radio*)menus.at(IngameMenuTab::OPTIONS_PREFERENCES).GetChild(2);

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
                level.Save(Config::Saves::FullPath(textInput->Text(), true));
                ctrl.SwitchMenu(false);
                break;
            case MenuAction::LOAD:
                ASSERT_MSG(list_load != nullptr, "Scroll menu GUI element not set.");
                if(list_load->ItemCount() > 0) {
                    ctrl.ChangeLevel(Config::Saves::FullPath(list_load->CurrentSelection()));
                }
                ctrl.SwitchMenu(false);
                break;
            case MenuAction::DELETE:
                ASSERT_MSG(list_save != nullptr, "Scroll menu GUI element not set.");
                if(list_save->ItemCount() > 0) {
                    if(Config::Saves::Delete(list_save->CurrentSelection())) {
                        list_save->UpdateContent(Config::Saves::Scan(), true);
                    }
                }
                break;
            case MenuAction::QUIT_MISSION:
                ctrl.QuitMission();
                break;
            case MenuAction::GAME_OVER:
                ctrl.GameOver(game_won);
                break;
        }
        action = MenuAction::NONE;
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
            case IngameMenuTab::END_SCENARIO:
                switch(keycode) {
                    case GLFW_KEY_L:
                        SetupConfirmScreen("Are you sure you\nwant to load a\ncustom scenario?", "Load Custom", glm::ivec2(0, 1), GLFW_KEY_L);
                        break;
                    case GLFW_KEY_S:
                        SetupConfirmScreen("Are you sure you\nwant to surrender\nto your enemies?", "Surrender", glm::ivec2(0, 1), GLFW_KEY_S);
                        break;
                    case GLFW_KEY_Q:
                        SetupConfirmScreen("Are you sure you\nwant to quit to\nthe main menu?", "Quit to Menu", glm::ivec2(0, 1), GLFW_KEY_Q);
                        break;
                    case GLFW_KEY_X:
                        SetupConfirmScreen("Are you sure you\nwant to exit\nthe game?", "Exit Program", glm::ivec2(0, 1), GLFW_KEY_X);
                        break;
                    case GLFW_KEY_ESCAPE:
                        OpenTab(IngameMenuTab::MAIN);
                        break;
                }
                break;
            case IngameMenuTab::CONFIRM_SCREEN:
                if(keycode == confirm_keycode) {
                    confirm_btn->Invoke();
                }
                else if (keycode == GLFW_KEY_ESCAPE) {
                    OpenTab(IngameMenuTab::END_SCENARIO);
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
                    textInput->SetText((list_save->ItemCount() > 0) ? list_save->CurrentSelection() : "savegame01");
                    EnableDeleteButton(true);
                    break;
                case IngameMenuTab::LOAD:
                    list_load->UpdateContent(Config::Saves::Scan(), true);
                    break;
                case IngameMenuTab::OPTIONS_SOUND:
                    sound_digital->SetValue(Config::Audio().volume_digital);
                    sound_master->SetValue(Config::Audio().volume_master);
                    sound_music->SetValue(Config::Audio().volume_music);
                    break;
                case IngameMenuTab::OPTIONS_SPEED:
                    speed_game->SetValue(Config::GameSpeed());
                    speed_mouse->SetValue(Config::Map_MouseSpeed());
                    speed_keys->SetValue(Config::Map_KeySpeed());
                    break;
                case IngameMenuTab::OPTIONS_PREFERENCES:
                    fog_of_war->Select(1 - int(Config::FogOfWar()));
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

        sound_digital = m.sound_digital;
        sound_master = m.sound_master;
        sound_music = m.sound_music;

        speed_game = m.speed_game;
        speed_mouse = m.speed_mouse;
        speed_keys = m.speed_keys;

        fog_of_war = m.fog_of_war;

        confirm_label = m.confirm_label;
        confirm_btn = m.confirm_btn;
        confirm_keycode = m.confirm_keycode;

        gameover_label = m.gameover_label;
        gameover_btn = m.gameover_btn;
        gameover_keycode = m.gameover_keycode;
        game_won = m.game_won;

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

    void GUI::IngameMenu::SetupConfirmScreen(const std::string& label, const std::string& btn, const glm::ivec2& highlightIdx, int keycode) {
        confirm_label->UpdateText(label);
        confirm_btn->Text(btn, highlightIdx);
        confirm_keycode = keycode;
        OpenTab(IngameMenuTab::CONFIRM_SCREEN);
    }

    void GUI::IngameMenu::SetGameOverState(int conditionID) {
        switch(conditionID) {
            case EndConditionType::LOSE:
                gameover_label->UpdateText("You have been defeated!");
                gameover_btn->Text("Defeat", glm::ivec2(0,1));
                gameover_keycode = GLFW_KEY_D;
                OpenTab(IngameMenuTab::GAME_OVER_SCREEN);
                game_won = false;
                break;
            case EndConditionType::WIN:
                gameover_label->UpdateText("You are victorious!");
                gameover_btn->Text("Victory", glm::ivec2(0,1));
                gameover_keycode = GLFW_KEY_V;
                OpenTab(IngameMenuTab::GAME_OVER_SCREEN);
                game_won = true;
                break;
        }
    }

    void GUI::IngameMenu::EndGame() {
        switch(confirm_keycode) {
            // //TODO:
            // case GLFW_KEY_L: //load custom
            //     break;
            // case GLFW_KEY_S: //surrender
            //     break;
            case GLFW_KEY_Q:
                action = MenuAction::QUIT_MISSION;
                break;
            case GLFW_KEY_X:
                Window::Get().Close();
                break;
            default:
                ENG_LOG_WARN("NOT IMPLEMENTED YET");
                break;
        }
    }

    void GUI::IngameMenu::GameOver() {
        action = MenuAction::GAME_OVER;
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


        SelectionData prev_selected = selection;

        int object_count = 0;
        int selection_mode = 0;
        glm::ivec3 num_id = glm::ivec3(-1);

#ifdef CLICK_SELECTION_V2
        //selection based on object bounding boxes intersections
        std::pair<glm::vec2, glm::vec2> selection_aabb = { m, M };
        std::vector<ClickSelectionEntry> selectionData = level.objects.ClickSelectionDataFromIDs(level.map.ObjectsInArea(im-1, iM+1, true));
        for(auto& entry : selectionData) {
            if(!AABB_intersection(entry.aabb, selection_aabb))
                continue;

            //under which category does current object belong; reset object counting when more important category is picked
            int object_mode = ObjectSelectionType(entry.id, entry.factionID, playerFactionID);
            if(selection_mode < object_mode) {
                selection_mode = object_mode;
                object_count = 0;
            }

            if(object_mode == selection_mode) {
                if((selection_mode < 3 && object_count < 1) || (selection_mode == 3 && object_count < selection.size())) {
                    if(!AlreadySelected(entry.id, object_count)) {
                        selection[object_count++] = entry.id;
                        num_id = entry.num_id;
                    }
                }
            }
        }
#else
        //selection based on map tiles readthrough
        for(int y = im.y; y <= iM.y; y++) {
            for(int x = im.x; x <= iM.x; x++) {

                for(int i = 0; i < 2; i++) {
                    glm::ivec2 coords = glm::ivec2(x,y);
                    if(i != 0 || level.map(coords).IsEmptyWaterTile())
                        coords = make_even(coords);
                    const TileData& td = level.map(coords);

                    if(!td.IsVisible() || !ObjectID::IsObject(td.info[i].id) || !ObjectID::IsValid(td.info[i].id) || td.info[i].IsUntouchable(playerFactionID))
                        continue;
                    
                    //under which category does current object belong; reset object counting when more important category is picked
                    int object_mode = ObjectSelectionType(td.info[i].id, td.info[i].factionId, playerFactionID);
                    if(selection_mode < object_mode) {
                        selection_mode = object_mode;
                        object_count = 0;
                    }

                    if(object_mode == selection_mode) {
                        if((selection_mode < 3 && object_count < 1) || (selection_mode == 3 && object_count < selection.size())) {
                            if(!AlreadySelected(td.info[i].id, object_count)) {
                                selection[object_count++] = td.info[i].id;
                                num_id = td.info[i].num_id;
                            }
                        }
                    }
                }
            }
        }
#endif

        bool force_update = false;
        if(object_count == 1 && selection_mode >= SelectionType::PLAYER_BUILDING) {
            Input& input = Input::Get();
            
            float current_select_time = input.CurrentTime();
            if(input.ctrl || ((current_select_time - last_select_time) < DOUBLE_CLICK_DELAY && selection[0] == prev_selection && !input.shift)) {
                //select units of the same type (currently visible on screen)
                auto [a,b] = Camera::Get().RectangleCoords();
                object_count = level.map.SelectByType(num_id, selection, a, b);
            }
            else if(input.shift && selection_mode == SelectionType::PLAYER_UNIT && selection_type == selection_mode && selected_count > 0 && selected_count < 9) {
                //only add this unit to the previous selection
                ObjectID newly_selected = selection[0];

                int idx = -1;
                for(int i = 0; i < selected_count; i++) {
                    if(prev_selected[i] == newly_selected)
                        idx = i;
                }
                
                if(idx < 0) {
                    //add to selection
                    object_count = selected_count+1;
                    selection_mode = selection_type;
                    selection = prev_selected;
                    selection[object_count-1] = newly_selected;
                }
                else {
                    //remove from selection
                    object_count = selected_count-1;
                    selection_mode = selection_type;
                    for(int i = idx; i < selected_count; i++) {
                        prev_selected[i] = prev_selected[i+1];
                    }
                    selection = prev_selected;
                    force_update = true;
                }
            }

            last_select_time = current_select_time;
        }
        prev_selection = selection[0];

        //selection among traversable objects (which are not part of regular map tiles data)
        iM += 1;
        for(auto& to : level.map.TraversableObjects()) {
            if(!ObjectID::IsObject(to.id) || !ObjectID::IsValid(to.id))
                    continue;
            
            FactionObject* obj;
            if(!level.objects.GetObject(to.id, obj))
                continue;

            glm::ivec2 pm = glm::ivec2(obj->MinPos());
            glm::ivec2 pM = glm::ivec2(obj->MaxPos()) + 1;
            if(!((pm.x <= M.x && pM.x >= m.x) && (pm.y <= M.y && pM.y >= m.y)))
                continue;
            
            int object_mode = ObjectSelectionType(to.id, obj->FactionIdx(), playerFactionID);
            if(selection_mode < object_mode) {
                selection_mode = object_mode;
                object_count = 0;
            }

            if(object_mode == selection_mode) {
                if((selection_mode < 3 && object_count < 1) || (selection_mode == 3 && object_count < selection.size())) {
                    selection[object_count++] = to.id;
                }
            }
        }
        
        //dont update values if there was nothing selected (keep the old selection)
        if(object_count != 0 || force_update) {
            selected_count = object_count;
            selection_type = selection_mode;
            update_flag = true;
            ENG_LOG_FINE("PlayerSelection::Select - mode {}, count: {}", selection_type, selected_count);

            if(object_count != 0) {
                FactionObject& object = level.objects.GetObject(selection[0]);
                if(object.Sound_Yes().valid && ((selection_type > SelectionType::ENEMY_UNIT) || (selection_type == SelectionType::ENEMY_BUILDING && ((Building&)object).IsGatherable()))) {
                    Audio::Play(object.Sound_Yes().Random());
                }
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
                location[new_count] = { object->RenderPosition(), object->RenderSize() };
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

                glm::vec2 center_pos = glm::vec2(0.f);

                int new_count = 0;
                for(size_t i = 0; i < s_count; i++) {
                    FactionObject* object = nullptr;
                    if(level.objects.GetObject(s_data[i], object)) {
                        s_data[new_count] = s_data[i];
                        center_pos += object->Positionf();

                        if(new_count == 0 && s_type > SelectionType::ENEMY_UNIT && object->Sound_Yes().valid) {
                            Audio::Play(object->Sound_Yes().Random());
                        }

                        new_count++;
                    }

                }
                s_count = new_count;
                center_pos = center_pos / float(s_count);

                if(s_count > 0) {
                    //repeated click - center on the group as well
                    bool centered = false;
                    float group_update_time = Input::CurrentTime();
                    if(last_group_select_idx == group_update_idx && (group_update_time - last_group_select_time) < DOUBLE_CLICK_DELAY) {
                        Camera::Get().Center(center_pos);
                        centered = true;
                    }
                    last_group_select_idx = group_update_idx;
                    last_group_select_time = group_update_time;

                    selection = s_data;
                    selection_type = s_type;
                    selected_count = s_count;
                    ENG_LOG_FINE("PlayerSelection::Select - mode {}, count: {} (centered: {})", selection_type, selected_count, centered);
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

    bool PlayerSelection::IssueTargetedCommand(Level& level, const glm::ivec2& target_pos, const glm::ivec2& target_pos_sharpened, const ObjectID& target_id_, const glm::ivec2& cmd_data, GUI::PopupMessage& msg_bar) {
        int command_id = cmd_data.x;
        int payload_id = cmd_data.y;
        ObjectID target_id = target_id_;
        PlayerFactionControllerRef player = level.factions.Player();

        //targeted commands can only be issued on player owned units (player buildings have no targeted commands)
        if(selection_type != SelectionType::PLAYER_UNIT)
            return false;
        
        ASSERT_MSG(!GUI::ActionButton_CommandType::IsTargetless(command_id), "Attempting to issue targetless command using a wrong function.");

        static const char* cmd_names[10] = { "Cast", "Build", "Move", "Patrol", "Attack", "AttackGround", "Harvest", "Gather", "Repair", "???" };
        const char* cmd_name = cmd_names[9];
        Command cmd;
        bool conditions_met = true;

        if(GUI::ActionButton_CommandType::IsSingleUser(command_id) && selected_count == 1) {
            Unit& unit = level.objects.GetUnit(selection[0]);

            switch(command_id) {
                case GUI::ActionButton_CommandType::CAST:
                    if(!unit.Faction()->CastConditionCheck(unit, payload_id)) {
                        ENG_LOG_FINE("Targeted command - '{}' - conditions not met - (pos: ({}, {}), payload: {})", cmd_name, target_pos.x, target_pos.y, payload_id);
                        msg_bar.DisplayMessage("You cannot cast that");
                        return false;
                    }
                    cmd = Command::Cast(payload_id, target_id, target_pos);
                    cmd_name = cmd_names[0];
                    break;
                case GUI::ActionButton_CommandType::BUILD:
                {
                    //build site check
                    BuildingDataRef bd = Resources::LoadBuilding(payload_id, bool(unit.Race()));
                    if(!level.map.IsAreaBuildable(target_pos, glm::ivec2(bd->size), bd->navigationType, unit.Position(), bd->coastal, bd->requires_foundation)) {
                        ENG_LOG_FINE("Targeted command - '{}' - invalid position - (pos: ({}, {}), payload: {})", cmd_name, target_pos.x, target_pos.y, payload_id);
                        msg_bar.DisplayMessage("You cannot build there");
                        Audio::Play(SoundEffect::Error().Random());
                        return false;
                    }

                    //resources & techtree conditions checks
                    int buildStatus = unit.Faction()->CanBeBuilt(payload_id, bool(unit.Race()), glm::ivec3(bd->cost));
                    if(buildStatus != 0) {
                        ENG_LOG_FINE("Targeted command - '{}' - conditions not met - (pos: ({}, {}), payload: {}, err: {})", cmd_name, target_pos.x, target_pos.y, payload_id, buildStatus);
                        msg_bar.DisplayMessage(FactionController::BuildAction_ErrorMessage(buildStatus-1));
                        Audio::Play(SoundEffect::Error().Random());
                        return false;
                    }
                    
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

                if(level.map.IsUntouchable(target->Position(), target->NavigationType() == NavigationBit::AIR)) {
                    target = nullptr;
                    target_friendly = false;
                    target_id = ObjectID();
                }
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
                    if(!ObjectID::IsAttackable(target_id) || !level.map.IsWithinBounds(target_pos_sharpened))
                        valid_target = false;
                    cmd = Command::Attack(target_id, target_pos_sharpened);
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

    void PlayerSelection::IssueAdaptiveCommand(Level& level, const glm::ivec2& target_pos, const glm::ivec2& target_pos_sharpened, const ObjectID& target_id_) {
        //adaptive commands can only be issued on player owned units
        if(selection_type != SelectionType::PLAYER_UNIT)
            return;
        ObjectID target_id = target_id_;
        ENG_LOG_FINE("Adaptive command - pos: ({}, {}), ID: {}", target_pos.x, target_pos.y, target_id);

        FactionObject* target_object = nullptr;
        if(ObjectID::IsObject(target_id)) {
            level.objects.GetObject(target_id, target_object);
            if(level.map.IsUntouchable(target_object->Position(), target_object->NavigationType() == NavigationBit::AIR)) {
                target_object = nullptr;
                target_id = ObjectID();
            }
        }

        bool harvestable = ObjectID::IsHarvestable(target_id);
        bool attackable = ObjectID::IsAttackable(target_id);
        bool enemy = (target_object == nullptr || level.factions.Diplomacy().AreHostile(target_object->FactionIdx(), level.factions.Player()->ID()));
        bool repairable = !enemy && target_object->IsRepairable();

        for(size_t i = 0; i < selected_count; i++) {
            Unit& unit = level.objects.GetUnit(selection[i]);
            bool gatherable = (ObjectID::IsObject(target_id) && target_object->IsGatherable(unit.NavigationType()));
            bool transport = (ObjectID::IsObject(target_id) && target_object->IsTransport());

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
                unit.IssueCommand(Command::Attack(target_id, target_pos_sharpened));
            }
            else if(transport && !enemy) {
                ENG_LOG_FINE("Adaptive command - Unit[{}] - Enter transport", i);
                unit.IssueCommand(Command::EnterTransport(target_id));
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

    void PlayerSelection::IssueTargetlessCommand(Level& level, const glm::ivec2& cmd_data, const ObjectID& command_target, GUI::PopupMessage& msg_bar) {
        int command_id = cmd_data.x;
        int payload_id = cmd_data.y;
        ASSERT_MSG(GUI::ActionButton_CommandType::IsTargetless(command_id) || (command_id == GUI::ActionButton_CommandType::CAST && SpellID::TargetlessCast(payload_id)), "Attempting to issue command that requires targets using a wrong function.");

        bool building_command = GUI::ActionButton_CommandType::IsBuildingCommand(command_id);
        ASSERT_MSG((building_command && selection_type == SelectionType::PLAYER_BUILDING) || (!building_command && selection_type == SelectionType::PLAYER_UNIT), "Invalid selection, cannot invoke targetless commands.");

        if(building_command) {
            Building& building = level.objects.GetBuilding(selection[0]);

            //price check
            PlayerFactionControllerRef player = level.factions.Player();
            glm::ivec3 price = building.ActionPrice(command_id, payload_id);
            glm::ivec3 resources = player->Resources();

            char buf[256];
            if(!Config::Hack_NoPrices()) {
                for(int idx = 0; idx < 3; idx++) {
                    if(price[idx] > resources[idx]) {
                        snprintf(buf, sizeof(buf), "Not enough %s.", GameResourceName(idx));
                        msg_bar.DisplayMessage(buf);
                        return;
                    }
                }

                //population check
                if(command_id == GUI::ActionButton_CommandType::TRAIN && !Config::Hack_NoPopLimit()) {
                    glm::ivec2 pop = player->Population();
                    if(pop[0] >= pop[1]) {
                        snprintf(buf, sizeof(buf), "Not enough food.");
                        msg_bar.DisplayMessage(buf);
                        return;
                    }
                }

                player->PayResources(price);
            }
            
            switch(command_id) {
                case GUI::ActionButton_CommandType::UPGRADE:
                    ENG_LOG_FINE("Targetless command - Upgrade (payload={})", payload_id);
                    building.IssueAction(BuildingAction::Upgrade(payload_id));
                    building.LastBtnIcon() = last_click_icon;
                    update_flag = true;
                    break;
                case GUI::ActionButton_CommandType::RESEARCH:
                    ENG_LOG_FINE("Targetless command - Research (payload={})", payload_id);
                    building.IssueAction(BuildingAction::TrainOrResearch(false, payload_id, building.TrainOrResearchTime(false, payload_id)));
                    building.LastBtnIcon() = last_click_icon;
                    update_flag = true;
                    break;
                case GUI::ActionButton_CommandType::TRAIN:
                    ENG_LOG_FINE("Targetless command - Train (payload={})", payload_id);
                    building.IssueAction(BuildingAction::TrainOrResearch(true, payload_id, building.TrainOrResearchTime(true, payload_id)));
                    building.LastBtnIcon() = last_click_icon;
                    update_flag = true;
                    break;
            }
        }
        else {
            static const char* cmd_names[6] = { "Stop", "StandGround", "ReturnGoods", "TransportUnload", "Cast", "???" };
            const char* cmd_name = cmd_names[5];
            Command cmd;
            switch(command_id) {
                case GUI::ActionButton_CommandType::CAST:
                {
                    if(!(GUI::ActionButton_CommandType::IsSingleUser(command_id) && selected_count == 1)) {
                        ENG_LOG_FINE("Targetless command - '{}' - invalid GUI conditions for cast - (payload: {})", cmd_name, payload_id);
                        return;
                    }

                    Unit& unit = level.objects.GetUnit(selection[0]);
                    if(!unit.Faction()->CastConditionCheck(unit, payload_id)) {
                        ENG_LOG_FINE("Targetless command - '{}' - conditions not met - (payload: {})", cmd_name, payload_id);
                        msg_bar.DisplayMessage("You cannot cast that");
                        return;
                    }

                    ENG_LOG_FINE("Targetless command - Cast");
                    cmd = Command::Cast(payload_id, ObjectID(), unit.Position() + DirectionVector(unit.Orientation()));
                    cmd_name = cmd_names[4];
                    break;
                }
                case GUI::ActionButton_CommandType::STOP:
                    ENG_LOG_FINE("Targetless command - Stop");
                    cmd = Command::Idle();
                    cmd_name = cmd_names[0];
                    break;
                case GUI::ActionButton_CommandType::STAND_GROUND:
                    ENG_LOG_FINE("Targetless command - StandGround");
                    cmd = Command::StandGround();
                    cmd_name = cmd_names[1];
                    break;
                case GUI::ActionButton_CommandType::RETURN_GOODS:
                    ENG_LOG_FINE("Targetless command - ReturnGoods");
                    cmd = Command::ReturnGoods();
                    cmd_name = cmd_names[2];              
                    break;
                case GUI::ActionButton_CommandType::TRANSPORT_UNLOAD:
                    ENG_LOG_FINE("Targetless command - TransportUnload ({}, {})", payload_id, command_target);
                    if(payload_id != 0)
                        cmd = Command::TransportUnload(command_target);
                    else
                        cmd = Command::TransportUnload();
                    cmd_name = cmd_names[3];
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

    void PlayerSelection::CancelBuildingAction(Level& level) {
        if(selection_type != SelectionType::PLAYER_BUILDING) {
            ENG_LOG_WARN("Cancel building action - invalid selection (requires player owned building selected).");
            return;
        }

        Building& building = level.objects.GetBuilding(selection[0]);
        if(!building.ProgressableAction()) {
            ENG_LOG_WARN("Cancel building action - nothing to cancel.");
            return;
        }

        ENG_LOG_FINE("Canceling building action (action type={})", building.BuildActionType());
        building.CancelAction();
    }

    void PlayerSelection::ConditionalUpdate(const FactionObject& obj) {
        update_flag |= (selection[0] == obj.OID());
    }

    void PlayerSelection::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        if(ImGui::CollapsingHeader("PlayerSelection")) {
            ImGui::Text("Type: %d, Count: %d", selection_type, selected_count);

            for(int i = 0; i < selected_count; i++) {
                ObjectID id = selection.at(i);
                ImGui::Text("ID[%d] - (%d, %d, %d)", i, (int)id.type, (int)id.idx, (int)id.id);
            }
        }
#endif
    }

    bool PlayerSelection::AlreadySelected(const ObjectID& id, int object_count) {
        for(int j = 0; j < object_count; j++) {
            if(selection[j] == id) {
                return true;
            }
        }
        return false;
    }

    //===== OcclusionMask =====

    OcclusionMask::OcclusionMask(const std::vector<uint32_t>& occlusionData, const glm::ivec2& size_) : size(size_) {
        int area = size.x * size.y;
        data = new int[area];
        for(int i = 0; i < area; i++)
            data[i] = 0;

        Import(occlusionData);
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

    std::vector<uint32_t> OcclusionMask::Export() {
        std::vector<uint32_t> res;

        uint32_t val = 0;
        int i = 0;
        for(int y = 0; y < size.y; y++) {
            for(int x = 0; x < size.x; x++) {
                val |= (int(data[y * size.x + x] != 0) << i);

                if((++i) == 32) {
                    res.push_back(val);
                    val = 0;
                    i = 0;
                }
            }
        }

        return res;
    }

    void OcclusionMask::Import(const std::vector<uint32_t>& vals) {
        if(vals.size() == 0)
            return;
        if((vals.size() * 32) < (size.x * size.y))
            ENG_LOG_WARN("OcclusionMask - loaded data don't cover the entire map (filling with unexplored).");

        int i = 0;
        int j = 0;
        uint32_t val = vals[j++];
        for(int y = 0; y < size.y; y++) {
            if(j >= vals.size())
                break;
            
            for(int x = 0; x < size.x; x++) {
                data[y*size.x + x] = ((val & (1 << i)) != 0) ? -1 : 0;

                if((++i) == 32) {
                    if(j >= vals.size())
                        break;
                    val = vals[j++];
                    i = 0;
                }
            }
        }
    }

    void OcclusionMask::DBG_Print() const {
        ENG_LOG_INFO("OcclusionMask ({}x{}):", size.x, size.y);
        for(int y = 0; y < size.y; y++) {
            for(int x = 0; x < size.x; x++) {
                printf("%3d ", operator()(y,x));
            }
            printf("\n");
        }
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

            // static int counter = 0;
            // char buf[256];
            // Image img = Image(tex->Width(), tex->Height(), 4, data);
            // snprintf(buf, sizeof(buf), "mapview_%02d.png", counter++);
            // img.Write(buf);


            tex->UpdateData((void*)data);
            delete[] data;
        }
    }

    MapView::rgba* MapView::Redraw(const Map& map) const {
        constexpr static std::array<std::array<rgba, TileType::COUNT>,2> tileColors = {
            std::array<rgba, TileType::COUNT> {
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
            },
            std::array<rgba, TileType::COUNT> {
                rgba(138,138,155,255),        //GROUND1
                rgba(138,138,155,255),        //GROUND2
                rgba(24,84,136,255),         //MUD1
                rgba(24,84,136,255),         //MUD2
                rgba(34,70,11,255),         //WALL_BROKEN
                rgba(86,51,3,255),          //ROCK_BROKEN
                rgba(50,102,15,255),        //TREES_FELLED
                rgba(4,56,116,255),         //WATER
                rgba(78,65,60,255),         //ROCK
                rgba(117,106,104,255),      //WALL_HU
                rgba(117,106,104,255),      //WALL_HU_DAMAGED
                rgba(117,106,104,255),      //WALL_OC
                rgba(117,106,104,255),      //WALL_OC_DAMAGED
                rgba(0,68,48,255),          //TREES
            }
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
                switch(td.vis.occlusion) {
                    default:
                    case 0:     //unexplored bit
                    case 2:
                        color = rgba(0,0,0,255);
                        break;
                    case 3:     //explored & no fog
                        if(ObjectID::IsObject(td.info[1].id) && !td.info[1].IsUntouchable(map.PlayerFactionID()))
                            color = factionColors[td.info[1].colorIdx % colorCount];
                        else if(ObjectID::IsObject(td.info[0].id) && !td.info[0].IsUntouchable(map.PlayerFactionID()))
                            color = factionColors[td.info[0].colorIdx % colorCount];
                        else
                            color = tileColors[tilesetIdx][td.tileType];
                        break;
                    case 1:     //explored, but with fog (show only buildings)
                    {
                        rgba tmp[2] = { tileColors[tilesetIdx][td.tileType], rgba(0,0,0,255) };
                        if(ObjectID::IsObject(td.info[1].id) && td.info[1].id.type == ObjectType::BUILDING)
                            color = factionColors[td.info[1].colorIdx % colorCount];
                        else if(ObjectID::IsObject(td.info[0].id) && td.info[1].id.type == ObjectType::BUILDING)
                            color = factionColors[td.info[0].colorIdx % colorCount];
                        else
                            color = tmp[int((x+y) % 2 == 0)];
                        break;
                    }
                }
                
                data[Y*sz.x + x] = color;
            }
        }

        //render the camera view rectangle
        auto [start, end] = Camera::Get().RectangleCoords();
        start = glm::clamp(start * scale, glm::ivec2(0), sz-1);
        end   = glm::clamp(end   * scale, glm::ivec2(0), sz-1);

        int a = sz.y-1-start.y;
        int b = sz.y-1-end.y;
        for(int x = start.x; x < end.x; x++) {
            data[a*sz.x+x] = data[b*sz.x+x] = rgba(255);
        }

        a = start.x;
        b = end.x;
        for(int y = start.y; y < end.y; y++) {
            int Y = (sz.y-1-y);
            data[Y*sz.x+a] = data[Y*sz.x+b] = rgba(255);
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
        : FactionController(std::move(entry), mapSize, FactionControllerID::LOCAL_PLAYER), mapview(MapView(mapSize)), occlusion(OcclusionMask(entry.occlusionData, mapSize)) {
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

        RenderBuildingViz();

        switch(state) {
            case PlayerControllerState::OBJECT_SELECTION:
                RenderSelectionRectangle();
                break;
        }

        //render selection highlights
        selection.Render();
    }

    void PlayerFactionController::Update(Level& level) {

        selection.GroupsUpdate(level);
        resources.Update(level.factions.Player()->Resources(), level.factions.Player()->Population());
        mapview.SetTilesetIdx(level.map.GetTileset()->GetType());

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
        mapview.Update(level.map, first_tick);
        first_tick = false;

        //update timing on the message bar
        msg_bar.Update();
    }

    void PlayerFactionController::SignalGUIUpdate() {
        selection.update_flag = true;
    }

    void PlayerFactionController::SignalGUIUpdate(const FactionObject& obj) {
        selection.ConditionalUpdate(obj);
    }
    
    void PlayerFactionController::Update_Paused(Level& level) {
        if(!is_menu_active) {
            selectionTab->Interactable(false);
            actionButtons.Buttons()->Interactable(false);

            //update the GUI panel (but only the 2 buttons - menu & pause)
            gui_handler.Update(&game_panel);

            selectionTab->Interactable(true);
            actionButtons.Buttons()->Interactable(true);
        }
        else {
            menu.Update(level, *this, gui_handler);
        }

        //hide text prompt contents
        text_prompt.Setup("");
        price.ClearAlt();

        //unset any previous btn clicks (to detect the new one)
        actionButtons.ClearClick();
        nontarget_cmd_issued = false;

        //update cursor icon
        Resources::CursorIcons::SetIcon(CursorIconName::HAND_HU + int(bool(Race())));
        
        //update timing on the message bar
        msg_bar.Update();
    }

    void PlayerFactionController::SwitchMenu(bool active, int tabID) {
        ASSERT_MSG(handler != nullptr, "PlayerFactionController not initialized properly!");
        is_menu_active = active;
        menu.OpenTab(tabID);
        handler->PauseRequest(active);
    }

    void PlayerFactionController::ChangeLevel(const std::string& filename) {
        handler->ChangeLevel(filename);
    }

    void PlayerFactionController::QuitMission() {
        handler->QuitMission();
    }

    void PlayerFactionController::GameOver(bool game_won) {
        handler->GameOver(game_won);
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
                handler->PauseRequest(true);
                is_menu_active = true;
                break;
            case GLFW_KEY_F5:
                menu.OpenTab(GUI::IngameMenuTab::OPTIONS);
                handler->PauseRequest(true);
                is_menu_active = true;
                break;
            case GLFW_KEY_F7:
                menu.OpenTab(GUI::IngameMenuTab::OPTIONS_SOUND);
                handler->PauseRequest(true);
                is_menu_active = true;
                break;
            case GLFW_KEY_F8:
                menu.OpenTab(GUI::IngameMenuTab::OPTIONS_SPEED);
                handler->PauseRequest(true);
                is_menu_active = true;
                break;
            case GLFW_KEY_F9:
                menu.OpenTab(GUI::IngameMenuTab::OPTIONS_PREFERENCES);
                handler->PauseRequest(true);
                is_menu_active = true;
                break;
            case GLFW_KEY_F10:
                SwitchMenu(true);
                break;
            case GLFW_KEY_F11:
                menu.OpenTab(GUI::IngameMenuTab::SAVE);
                handler->PauseRequest(true);
                is_menu_active = true;
                break;
            case GLFW_KEY_F12:
                menu.OpenTab(GUI::IngameMenuTab::LOAD);
                handler->PauseRequest(true);
                is_menu_active = true;
                break;
        }
    }

    void PlayerFactionController::UpdateState(Level& level, int& cursor_idx) {
        Camera& camera = Camera::Get();
        Input& input = Input::Get();

        //hovering over an object decisionmaking
        bool hover_airborne = false;
        glm::ivec2 hover_coords = glm::ivec2(camera.GetMapCoords(input.mousePos_n * 2.f - 1.f) + 0.5f);
        ObjectID hover_id = level.map.ObjectIDAt(hover_coords, hover_airborne);
        bool hovering_over_object = ObjectID::IsValid(hover_id) && !ObjectID::IsHarvestable(hover_id) && !level.map.IsUntouchable(hover_coords, hover_airborne);

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

                if (nontarget_cmd_issued) {
                    selection.IssueTargetlessCommand(level, command_data, command_target, msg_bar);
                    command_data = glm::ivec2(-1);
                    command_target = ObjectID();
                }
                else if(actionButtons.ClickIdx() != -1) {
                    ResolveActionButtonClick();

                    if(nontarget_cmd_issued) {
                        selection.IssueTargetlessCommand(level, command_data, command_target, msg_bar);
                        command_data = glm::ivec2(-1);
                        command_target = ObjectID();
                    }
                    else if(actionButtons.BuildingActionCancelled()) {
                        selection.CancelBuildingAction(level);
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
                        glm::ivec2 target_pos = glm::ivec2(camera.GetMapCoords(input.mousePos_n * 2.f - 1.f) + 0.5f);
                        glm::ivec2 target_pos_sharpened  = target_pos;
                        ObjectID target_id = level.map.ObjectIDAt(target_pos_sharpened);
                        selection.IssueAdaptiveCommand(level, target_pos, target_pos_sharpened, target_id);
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
                        glm::ivec2 target_pos_sharpened  = target_pos;
                        ObjectID target_id = level.map.ObjectIDAt(target_pos_sharpened);
                        selection.IssueAdaptiveCommand(level, target_pos, target_pos_sharpened, target_id);
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
                            glm::ivec2 target_pos = glm::ivec2(camera.GetMapCoords(input.mousePos_n * 2.f - 1.f) + 0.5f);
                            glm::ivec2 target_pos_sharpened  = target_pos;
                            ObjectID target_id = level.map.ObjectIDAt(target_pos_sharpened);
                            if(selection.IssueTargetedCommand(level, target_pos, target_pos_sharpened, target_id, command_data, msg_bar)) {
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
                                Queue_RenderBuildingViz(level.map, worker->Position(), command_data.y);
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
                            glm::ivec2 target_pos_sharpened  = target_pos;
                            ObjectID target_id = level.map.ObjectIDAt(target_pos_sharpened);
                            if(selection.IssueTargetedCommand(level, target_pos, target_pos_sharpened, target_id, command_data, msg_bar)) {
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
        selection.last_click_icon = btn.icon;

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
            case GUI::ActionButton_CommandType::TRANSPORT_UNLOAD:
                //commands without target, issue immediately on current selection
                command_data = glm::ivec2(btn.command_id, btn.payload_id);
                command_target = ObjectID();
                nontarget_cmd_issued = true;
                break;
            case GUI::ActionButton_CommandType::CAST:
                if(SpellID::TargetlessCast(btn.payload_id)) {
                    command_data = glm::ivec2(btn.command_id, btn.payload_id);
                    command_target = ObjectID();
                    nontarget_cmd_issued = true;
                    break;
                }
            case GUI::ActionButton_CommandType::BUILD:
            {
                //targetable command with resources check
                int resourcesCheckResult = ResourcesCheck(glm::ivec3(btn.price));
                if(resourcesCheckResult == 0) {
                    state = PlayerControllerState::COMMAND_TARGET_SELECTION;
                    actionButtons.ShowCancelPage();
                    command_data = glm::ivec2(btn.command_id, btn.payload_id);
                    command_target = ObjectID();
                }
                else {
                    msg_bar.DisplayMessage(FactionController::BuildAction_ErrorMessage(resourcesCheckResult-1));
                    Audio::Play(SoundEffect::Error().Random());
                }
                break;
            }
            default:
                //targetable commands, move to target selection
                //keep track of the command (or button), watch out for selection changes -> selection cancel?
                //also do conditions check (enough resources), maybe do one after the target is selected as well
                state = PlayerControllerState::COMMAND_TARGET_SELECTION;
                actionButtons.ShowCancelPage();
                command_data = glm::ivec2(btn.command_id, btn.payload_id);
                command_target = ObjectID();
                break;
        }
    }

    void PlayerFactionController::CameraPanning(const glm::vec2& pos) {
        if(!Config::CameraPanning())
            return;

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
        command_target = ObjectID();
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

    void PlayerFactionController::Queue_RenderBuildingViz(const Map& map, const glm::ivec2& worker_pos, int id) {
        buildingViz_id = id;
        buildingViz_workerPos = worker_pos;

        BuildingDataRef building = Resources::LoadBuilding(buildingViz_id, bool(Race()));
        SpriteGroup& sg = building->animData->GetGraphics(BuildingAnimationType::IDLE);
        glm::ivec2 sz = glm::ivec2(building->size);

        Camera& cam = Camera::Get();
        Input& input = Input::Get();
        glm::ivec2 corner = glm::ivec2(cam.GetMapCoords(input.mousePos_n * 2.f - 1.f) + 0.5f);
        float zIdx = SELECTION_ZIDX;
        int nav_type = building->navigationType;
        bool coast_check = map.CoastCheck(corner, sz, building->coastal);
        bool foundation_check = (!building->requires_foundation || map.BuildingFoundationCheck(corner, BuildingType::OIL_PATCH));
        bool precondtions = coast_check && foundation_check;

        buildingViz_check.clear();
        for(int y = 0; y < sz.y; y++) {
            for(int x = 0; x < sz.x; x++) {
                glm::ivec2 pos = glm::ivec2(corner.x + x, corner.y + y);
                buildingViz_check.push_back((map.IsBuildable(pos, nav_type, buildingViz_workerPos) && precondtions));
            }
        }
    }

    void PlayerFactionController::RenderBuildingViz() {
        if(buildingViz_id < 0)
            return;

        BuildingDataRef building = Resources::LoadBuilding(buildingViz_id, bool(Race()));
        SpriteGroup& sg = building->animData->GetGraphics(BuildingAnimationType::IDLE);
        glm::ivec2 sz = glm::ivec2(building->size);

        Camera& cam = Camera::Get();
        Input& input = Input::Get();
        glm::ivec2 corner = glm::ivec2(cam.GetMapCoords(input.mousePos_n * 2.f - 1.f) + 0.5f);
        float zIdx = SELECTION_ZIDX;

        //building sprite
        sg.Render(glm::vec3(cam.map2screen(corner), zIdx), building->size * cam.Mult(), glm::ivec4(0), 0, 0);
        
        //colored overlay, signaling buildability
        int i = 0;
        for(int y = 0; y < sz.y; y++) {
            for(int x = 0; x < sz.x; x++) {
                glm::ivec2 pos = glm::ivec2(corner.x + x, corner.y + y);
                glm::vec4 clr = buildingViz_check.at(i++) ? glm::vec4(0.f, 0.62f, 0.f, 1.f) : glm::vec4(0.62f, 0.f, 0.f, 1.f);
                Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(pos), zIdx-1e-3f), cam.Mult(), clr, shadows));
            }
        }

        buildingViz_id = -1;
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

        shadows = TextureGenerator::GetTexture(TextureGenerator::Params::ShadowsTexture(256,256,8));

        //general style for menu buttons
        GUI::StyleRef menu_btn_style = std::make_shared<GUI::Style>();
        menu_btn_style->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, false));
        menu_btn_style->hoverTexture = menu_btn_style->texture;
        menu_btn_style->holdTexture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, true));
        menu_btn_style->highlightTexture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonHighlightTexture(textureSize.x, textureSize.y, borderWidth));
        menu_btn_style->textColor = textClr;
        menu_btn_style->font = font;
        menu_btn_style->holdOffset = glm::ivec2(borderWidth);       //= texture border width

        //style for the menu background
        GUI::StyleRef menu_style = std::make_shared<GUI::Style>();
        menu_style->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, false));
        menu_style->hoverTexture = menu_style->texture;
        menu_style->holdTexture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, true));
        menu_style->highlightTexture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonHighlightTexture(textureSize.x, textureSize.y, borderWidth));
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
        // text_style->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, false));
        // text_style_small->color = glm::vec4(1.f);
        // text_style_small->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, false));
        // text_style_small2->color = glm::vec4(1.f);
        // text_style_small2->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, 0, false));

        buttonSize = glm::vec2(0.25f, 0.25f);
        ts = glm::vec2(Window::Get().Size()) * buttonSize;
        upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
        textureSize = ts * upscaleFactor;
        borderWidth = 7 * upscaleFactor;

        //style for icon buttons
        GUI::StyleRef icon_btn_style = std::make_shared<GUI::Style>();
        icon_btn_style->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, glm::uvec3(200), glm::uvec3(20), glm::uvec3(240), glm::uvec3(125)));
        icon_btn_style->hoverTexture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear_2borders(textureSize.x, textureSize.y, borderWidth, borderWidth, glm::uvec3(200), glm::uvec3(20), glm::uvec3(240), glm::uvec3(125), glm::uvec3(125), borderWidth / 2));
        icon_btn_style->holdTexture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, glm::uvec3(170), glm::uvec3(20), glm::uvec3(170), glm::uvec3(110)));
        icon_btn_style->highlightTexture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonHighlightTexture(textureSize.x, textureSize.y, borderWidth / 2, glm::u8vec4(66, 245, 84, 255)));
        icon_btn_style->textColor = textClr;
        icon_btn_style->font = font;
        icon_btn_style->holdOffset = glm::ivec2(borderWidth);

        buttonSize = glm::vec2(0.25f, 0.25f);
        ts = glm::vec2(Window::Get().Size()) * buttonSize;
        upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
        textureSize = ts * upscaleFactor;
        borderWidth = 3 * upscaleFactor;
        GUI::StyleRef borders_style = std::make_shared<GUI::Style>();
        borders_style->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, glm::u8vec4(0,0,0,0), glm::u8vec4(20,20,20,255), glm::u8vec4(240,240,240,255), glm::u8vec4(125,125,125,255)));

        //style for icon buttons bar
        buttonSize = glm::vec2(0.25f, 0.25f * GUI::ImageButtonWithBar::BarHeightRatio());
        ts = glm::vec2(Window::Get().Size()) * buttonSize;
        upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
        textureSize = ts * upscaleFactor;
        borderWidth = 7 * upscaleFactor;
        glm::vec2 bar_border_size = 7.f / ts;

        GUI::StyleRef bar_style = std::make_shared<GUI::Style>();
        bar_style->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, borderWidth, borderWidth, glm::uvec3(70), glm::uvec3(20), glm::uvec3(70), glm::uvec3(70)));

        buttonSize = glm::vec2(0.25f, 0.25f * GUI::ImageButtonWithBar::BarHeightRatio());
        ts = glm::vec2(Window::Get().Size()) * buttonSize;
        upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
        textureSize = ts * upscaleFactor;
        borderWidth = 7 * upscaleFactor;
        glm::vec2 mana_bar_borders_size = 7.f / ts;
        GUI::StyleRef mana_bar_style = std::make_shared<GUI::Style>();
        mana_bar_style->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear_2borders(textureSize.x, textureSize.y, borderWidth, borderWidth, glm::uvec3(120), glm::uvec3(20), glm::uvec3(120), glm::uvec3(120), glm::uvec3(240), borderWidth/2));

        glm::vec2 progress_bar_borders_size = 7.f / ts;
        GUI::StyleRef progress_bar_style = std::make_shared<GUI::Style>();
        progress_bar_style->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear_3borders(textureSize.x, textureSize.y, borderWidth, borderWidth, glm::uvec3(120), glm::uvec3(20), glm::uvec3(120), glm::uvec3(120), glm::uvec3(252,208,61), 2*borderWidth/3, glm::uvec3(20), borderWidth/3));

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
            PlayerFactionController* ctrl = static_cast<PlayerFactionController*>(handler);
            if(ctrl->selectionTab->DefaultButtonMode())
                ctrl->selection.SelectFromSelection(id);
            else {
                //issue the transport unload command
                ctrl->command_data = glm::ivec2(GUI::ActionButton_CommandType::TRANSPORT_UNLOAD, 1);
                ctrl->command_target = ctrl->selectionTab->GetTransportID(id);
                ctrl->nontarget_cmd_issued = true;
            }
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
        selection.DBG_GUI();
        // ImGui::ColorEdit4("shadows", (float*)&clr);
#endif
    }

    void PlayerFactionController::UpdateOcclusions(Level& level) {
        level.map.DownloadOcclusionMask(occlusion, ID());
    }

    void PlayerFactionController::DisplayEndDialog(int condition_id) {
        //pause the game & display the end game menu
        menu.SetGameOverState(condition_id);
        SwitchMenu(true, GUI::IngameMenuTab::GAME_OVER_SCREEN);
    }

    std::vector<uint32_t> PlayerFactionController::ExportOcclusion() {
        return occlusion.Export();
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
        s->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, 0, shadingWidth, 0, false));
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
        s->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, 0, shadingWidth, glm::u8vec3(140), glm::u8vec3(30), glm::u8vec3(200), glm::u8vec3(100)));
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
        s->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, false, true));
        s->hoverTexture = s->texture;
        s->holdTexture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, true, true));
        s->highlightMode = GUI::HighlightMode::NONE;
        res.push_back(s);

        s = std::make_shared<GUI::Style>();
        s->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, false, false));
        s->hoverTexture = s->texture;
        s->holdTexture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, true, false));
        s->highlightMode = GUI::HighlightMode::NONE;
        res.push_back(s);

        s = std::make_shared<GUI::Style>();
        GUI::StyleRef z = s;
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
        s = std::make_shared<GUI::Style>();
        s->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, shadingWidth, 0, 0, false));
        s->hoverTexture = s->texture;
        s->holdTexture = s->texture;
        s->highlightMode = GUI::HighlightMode::NONE;
        res.push_back(s);
        res.push_back(z);
        res.push_back(disabled_btn);

        return res;
    }

    std::vector<GUI::StyleRef> SetupSliderStyles(const FontRef& font, const glm::vec2& scrollMenuSize, int scrollMenuItems, float scrollButtonSize, const glm::vec2& smallBtnSize) {
        GUI::StyleRef s = nullptr;
        std::vector<GUI::StyleRef> res;

        //menu item style
        glm::vec2 buttonSize = glm::vec2(scrollMenuSize.x, scrollMenuSize.y / scrollMenuItems);
        glm::vec2 ts = glm::vec2(Window::Get().Size()) * buttonSize;
        float upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
        glm::ivec2 textureSize = ts * upscaleFactor;
        int shadingWidth = 2 * upscaleFactor;

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
        s->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, false, true));
        s->hoverTexture = s->texture;
        s->holdTexture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, true, true));
        s->highlightMode = GUI::HighlightMode::NONE;
        res.push_back(s);

        s = std::make_shared<GUI::Style>();
        s->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, false, false));
        s->hoverTexture = s->texture;
        s->holdTexture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Triangle(textureSize.x, textureSize.y, shadingWidth, true, false));
        s->highlightMode = GUI::HighlightMode::NONE;
        res.push_back(s);

        s = std::make_shared<GUI::Style>();
        GUI::StyleRef z = s;
        s->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Gem2(textureSize.x, textureSize.y, shadingWidth, Window::Get().Ratio()));
        s->hoverTexture = s->texture;
        s->holdTexture = s->texture;
        s->highlightMode = GUI::HighlightMode::NONE;
        s->font = font;
        s->textScale = 0.6f;
        s->textColor = textClr;

        s = std::make_shared<GUI::Style>();
        GUI::StyleRef w = s;
        s->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Gem(textureSize.x, textureSize.y, shadingWidth, Window::Get().Ratio(), glm::u8vec4(110,110,110,255)));
        s->hoverTexture = s->texture;
        s->holdTexture = s->texture;
        s->highlightMode = GUI::HighlightMode::NONE;
        s->font = font;
        s->textScale = 0.6f;
        s->textColor = textClr;

        //scroll slider style
        buttonSize = glm::vec2(scrollButtonSize, scrollMenuSize.y);
        ts = glm::vec2(Window::Get().Size()) * buttonSize;
        upscaleFactor = std::max(1.f, 128.f / std::min(ts.x, ts.y));  //upscale the smaller side to 128px
        textureSize = ts * upscaleFactor;
        shadingWidth = 2 * upscaleFactor;
        s = std::make_shared<GUI::Style>();
        s->texture = TextureGenerator::GetTexture(TextureGenerator::Params::ButtonTexture_Clear(textureSize.x, textureSize.y, shadingWidth, 0, 0, false));
        s->hoverTexture = s->texture;
        s->holdTexture = s->texture;
        s->highlightMode = GUI::HighlightMode::NONE;
        res.push_back(s);
        res.push_back(z);
        res.push_back(w);

        return res;
    }

    void format_wBonus(char* buf, size_t buf_size, const char* prefix, int val_bonus, glm::ivec2& out_highlight_idx) {
        size_t len = strlen(prefix);
        out_highlight_idx = glm::ivec2(-1);
        if(val_bonus != 0) {
            snprintf(buf, buf_size, "%s+%d", prefix, val_bonus);
            const char* ptr = strchr(buf, ':');
            if(ptr != nullptr) {
                out_highlight_idx = glm::ivec2(len - int(ptr-buf) - 1, 99);
            }
        }
        else {
            strncpy(buf, prefix, len);
            buf[len] = '\0';
        }
    }

}//namespace eng
