#include "engine/game/techtree.h"

#include "engine/game/object_data.h"
#include "engine/game/gameobject.h"
#include "engine/game/resources.h"
#include "engine/utils/dbg_gui.h"
#include "engine/game/level.h"

//represents column count in the icons spritesheet
#define ICON_MAX_X 10
#define MARKSMANSHIP_BOOST 3
#define SPELL_RESEARCH_OFFSET 11

namespace eng {

    glm::ivec2 Icon_AddOffset(const glm::ivec2& icon, int offset) {
        int x = icon.x + offset;
        return glm::ivec2(x % ICON_MAX_X, icon.y + (x / ICON_MAX_X));
    }

    namespace SpellID {

        static std::array<int, SpellID::COUNT> init_spell_prices() {
            std::array<int, SpellID::COUNT> arr = {};
            for(int& v : arr) v = INVALID_PRICE;
            return arr;
        }

        static std::array<int, SpellID::COUNT> spell_price = init_spell_prices();

        static std::array<bool, SpellID::COUNT> is_targeted = {
            true, false, true, true, true, true, false,
            true, false, true, false, false, true, false,
            false, false, false, false,
            false
        };

        static std::array<int, SpellID::COUNT> casting_range = {
            6,10,10,6,6,10,12,
            6,10,6,6,12,6,12,
            9999,9999,10,10,
            0
        };

        std::pair<int,int> TechtreeInfo(int spellID) {
            int race = spellID / SpellID::BLOODLUST;
            int researchType = SPELL_RESEARCH_OFFSET + (spellID % SpellID::BLOODLUST);
            return { race, researchType };
        }

        bool IsResearchless(int spellID) {
            return spellID >= SpellID::HOLY_VISION;
        }

        int Price(int spellID) {
            return spell_price.at(spellID);
        }

        void SetPrice(int spellID, int price) {
            spell_price.at(spellID) = price;
        }

        bool RequiresTarget(int spellID) {
            return is_targeted.at(spellID);
        }

        bool TargetlessCast(int spellID) {
            return spellID == SpellID::EYE;
        }

        bool IsContinuous(int spellID) {
            return spellID == SpellID::BLIZZARD || spellID == SpellID::DEATH_AND_DECAY;
        }

        bool SimplePrice(int spellID) {
            return spellID != SpellID::HEAL && spellID != SpellID::RAISE_DEAD && spellID != SpellID::EXORCISM;
        }

        bool TargetConditionCheck(int spellID, Level& level, const ObjectID& targetID) {
            switch(spellID) {
                case SpellID::FLAME_SHIELD:
                {
                    Unit* target = nullptr;
                    if(level.objects.GetUnit(targetID, target)) {
                        return target->NavigationType() == NavigationBit::GROUND;
                    }
                    return false;
                }
                default:
                    return true;
            }
        }

        bool SoundOnSpawn(int spellID) {
            return spellID != SpellID::HEAL && spellID != SpellID::DEATH_COIL && spellID != SpellID::EXORCISM;
        }

        int CastingRange(int spellID) {
            return casting_range[spellID];
        }

        const char* SpellName(int spellID) {
            static std::array<const char*, SpellID::COUNT> spell_names = {
                "HEAL", "EXORCISM", "SLOW", "FLAME_SHIELD", "INVISIBILITY", "POLYMORPH", "BLIZZARD",
                "BLOODLUST", "RUNES", "HASTE", "RAISE_DEAD", "TORNADO", "UNHOLY_ARMOR", "DEATH_AND_DECAY",
                "HOLY_VISION", "EYE", "FIREBALL", "DEATH_COIL", "DEMOLISH"
            };
            return spell_names.at(spellID);
        }

        int Spell2Buff(int spellID) {
            static std::array<int, SpellID::COUNT> tab = {
                -1, -1, UnitEffectType::SLOW, -1, UnitEffectType::INVISIBILITY, -1, -1, 
                UnitEffectType::BLOODLUST, -1, UnitEffectType::HASTE, -1, -1, UnitEffectType::UNHOLY_ARMOR, -1,
                -1, -1, -1, -1
            };
            return tab.at(spellID);
        }

    }//namespace SpellID

    //===== TechtreeData =====

    void TechtreeData::RecomputeUnitLevels() {
        level_counters[TechtreeLevelCounter::NONE]  = 1;
        level_counters[TechtreeLevelCounter::MELEE] = 1 + research[ResearchType::MELEE_ATTACK] + research[ResearchType::MELEE_DEFENSE];
        level_counters[TechtreeLevelCounter::NAVAL] = 1 + research[ResearchType::NAVAL_ATTACK] + research[ResearchType::NAVAL_DEFENSE];
        level_counters[TechtreeLevelCounter::SIEGE] = 1 + research[ResearchType::SIEGE_ATTACK];

        level_counters[TechtreeLevelCounter::RANGED] = 1 + 
            research[ResearchType::RANGED_ATTACK] +
            research[ResearchType::LM_RANGER_UPGRADE] +
            research[ResearchType::LM_RANGE] +
            research[ResearchType::LM_SIGHT] +
            research[ResearchType::LM_UNIQUE];
        
        level_counters[TechtreeLevelCounter::PALA] =
            level_counters[TechtreeLevelCounter::MELEE] +
            research[ResearchType::PALA_UPGRADE] +
            research[ResearchType::PALA_EXORCISM] +
            research[ResearchType::PALA_HEAL];
        
        level_counters[TechtreeLevelCounter::MAGE] = 1 + 
            research[ResearchType::MAGE_SLOW] +
            research[ResearchType::MAGE_SHIELD] +
            research[ResearchType::MAGE_INVIS] +
            research[ResearchType::MAGE_POLY] +
            research[ResearchType::MAGE_BLIZZARD];
    }

    void TechtreeData::RecomputeBonuses(bool isOrc) {
        attack_bonus[TechtreeAttackBonusType::MELEE]    = Resources::LoadResearchBonus(ResearchType::MELEE_ATTACK, research[ResearchType::MELEE_ATTACK]);
        attack_bonus[TechtreeAttackBonusType::NAVAL]    = Resources::LoadResearchBonus(ResearchType::NAVAL_ATTACK, research[ResearchType::NAVAL_ATTACK]);
        attack_bonus[TechtreeAttackBonusType::SIEGE]    = Resources::LoadResearchBonus(ResearchType::SIEGE_ATTACK, research[ResearchType::SIEGE_ATTACK]);
        attack_bonus[TechtreeAttackBonusType::RANGED]   = Resources::LoadResearchBonus(ResearchType::RANGED_ATTACK, research[ResearchType::RANGED_ATTACK]) + MARKSMANSHIP_BOOST * int(research[ResearchType::LM_UNIQUE] != 0 && !isOrc);

        armor_bonus[TechtreeArmorBonusType::MELEE] = Resources::LoadResearchBonus(ResearchType::MELEE_DEFENSE, research[ResearchType::MELEE_DEFENSE]);
        armor_bonus[TechtreeArmorBonusType::NAVAL] = Resources::LoadResearchBonus(ResearchType::NAVAL_DEFENSE, research[ResearchType::NAVAL_DEFENSE]);

        vision_bonus    = Resources::LoadResearchBonus(ResearchType::LM_SIGHT, research[ResearchType::LM_SIGHT]);
        range_bonus     = Resources::LoadResearchBonus(ResearchType::LM_RANGE, research[ResearchType::LM_RANGE]);
    }

    //===== Techtree =====

    constexpr static std::array<glm::ivec3, UnitType::COUNT> idx_LUT = {
        glm::ivec3{ TechtreeAttackBonusType::NONE,      TechtreeArmorBonusType::NONE,   TechtreeLevelCounter::NONE },      //peasant
        glm::ivec3{ TechtreeAttackBonusType::MELEE,     TechtreeArmorBonusType::MELEE,  TechtreeLevelCounter::MELEE },     //footman
        glm::ivec3{ TechtreeAttackBonusType::RANGED,    TechtreeArmorBonusType::NONE,   TechtreeLevelCounter::RANGED },    //archer
        glm::ivec3{ TechtreeAttackBonusType::RANGED,    TechtreeArmorBonusType::NONE,   TechtreeLevelCounter::RANGED },    //ranger
        glm::ivec3{ TechtreeAttackBonusType::SIEGE,     TechtreeArmorBonusType::NONE,   TechtreeLevelCounter::SIEGE },     //ballista
        glm::ivec3{ TechtreeAttackBonusType::MELEE,     TechtreeArmorBonusType::MELEE,  TechtreeLevelCounter::MELEE },     //knight
        glm::ivec3{ TechtreeAttackBonusType::MELEE,     TechtreeArmorBonusType::MELEE,  TechtreeLevelCounter::PALA },      //paladin
        glm::ivec3{ TechtreeAttackBonusType::NONE,      TechtreeArmorBonusType::NONE,   TechtreeLevelCounter::NONE },      //tanker
        glm::ivec3{ TechtreeAttackBonusType::NAVAL,     TechtreeArmorBonusType::NAVAL,  TechtreeLevelCounter::NAVAL },     //destroyer
        glm::ivec3{ TechtreeAttackBonusType::NAVAL,     TechtreeArmorBonusType::NAVAL,  TechtreeLevelCounter::NAVAL },     //battleship
        glm::ivec3{ TechtreeAttackBonusType::NAVAL,     TechtreeArmorBonusType::NAVAL,  TechtreeLevelCounter::NAVAL },     //submarine
        glm::ivec3{ TechtreeAttackBonusType::NONE,      TechtreeArmorBonusType::NONE,   TechtreeLevelCounter::NONE },      //transport
        glm::ivec3{ TechtreeAttackBonusType::NONE,      TechtreeArmorBonusType::NONE,   TechtreeLevelCounter::NONE },      //roflcopter
        glm::ivec3{ TechtreeAttackBonusType::NONE,      TechtreeArmorBonusType::NONE,   TechtreeLevelCounter::NONE },      //demosquad
        glm::ivec3{ TechtreeAttackBonusType::NONE,      TechtreeArmorBonusType::NONE,   TechtreeLevelCounter::MAGE },      //mage
        glm::ivec3{ TechtreeAttackBonusType::NONE,      TechtreeArmorBonusType::NONE,   TechtreeLevelCounter::NONE },      //gryphon
    };

    int Techtree::UnitLevel(int unit_type, bool isOrc) const {
        return (unit_type < UnitType::COUNT) ? data[int(isOrc)].level_counters[idx_LUT[unit_type].z] : 1;
    }

    int Techtree::BonusArmor(int unit_type, bool isOrc) const {
        return (unit_type < UnitType::COUNT) ? data[int(isOrc)].armor_bonus[idx_LUT[unit_type].y] : 0;
    }

    int Techtree::BonusDamage(int unit_type, bool isOrc) const {
        return (unit_type < UnitType::COUNT) ? data[int(isOrc)].attack_bonus[idx_LUT[unit_type].x] : 0;
    }

    int Techtree::BonusVision(int unit_type, bool isOrc) const {
        return (unit_type < UnitType::COUNT) ? ((unit_type == UnitType::RANGER) * data[int(isOrc)].vision_bonus) : 0;
    }

    int Techtree::BonusRange(int unit_type, bool isOrc) const {
        return (unit_type < UnitType::COUNT) ? ((unit_type == UnitType::RANGER) * data[int(isOrc)].range_bonus) : 0;
    }

    bool Techtree::SetupResearchButton(GUI::ActionButtonDescription& btn, bool isOrc) const {
        ASSERT_MSG(btn.command_id == GUI::ActionButton_CommandType::RESEARCH, "Techtree::ButtonSetup - invalid command type detected ({})", btn.command_id);
        ASSERT_MSG(((unsigned)btn.payload_id) < ((unsigned)ResearchType::COUNT), "Techtree::ButtonSetup - invalid research type.");

        int race = int(isOrc);
        int type = btn.payload_id;
        int level = data[race].research[btn.payload_id];

        if(!ResearchDependenciesMet(btn.payload_id, isOrc) || ResearchConstrained(btn.payload_id, level))
            return false;

        ResearchInfo info = {};
        if(Resources::LoadResearchInfo(type, level+1, info)) {
            btn.name        = info.viz.name[race];
            btn.has_hotkey  = info.viz.has_hotkey;
            btn.hotkey      = info.viz.hotkey[race];
            btn.hotkey_idx  = info.viz.hotkey_idx[race];
            btn.price       = info.data.price[race];
            btn.icon        = Icon_AddOffset(info.viz.icon[race], level);
            std::transform(btn.name.begin(), btn.name.end(), btn.name.begin(), ::toupper);
            return true;
        }
        return false;
    }

    bool Techtree::SetupTrainButton(GUI::ActionButtonDescription& btn, bool isOrc) const {
        ASSERT_MSG(btn.command_id == GUI::ActionButton_CommandType::TRAIN, "Techtree::ButtonSetup - invalid command type detected ({})", btn.command_id);
        int race = int(isOrc);
        if(btn.payload_id == UnitType::ARCHER && data[race].research[ResearchType::LM_RANGER_UPGRADE]) {
            btn.payload_id  = UnitType::RANGER;
            btn.icon        = glm::ivec2(btn.icon.x + 2, btn.icon.y);
            btn.has_hotkey  = true;
            if(isOrc) {
                btn.name        = "berserker";
                btn.hotkey      = 'b';
                btn.hotkey_idx  = 0;
            }
            else {
                btn.name        = "ranger";
                btn.hotkey      = 'a';
                btn.hotkey_idx  = 1;
            }
            return true;
        }
        else if(btn.payload_id == UnitType::KNIGHT && data[race].research[ResearchType::PALA_UPGRADE]) {
            btn.payload_id  = UnitType::PALADIN;
            btn.icon        = glm::ivec2(int(isOrc), 1);
            btn.has_hotkey  = true;
            if(isOrc) {
                btn.name        = "ogre-mage";
                btn.hotkey      = 'o';
                btn.hotkey_idx  = 0;
            }
            else {
                btn.name        = "paladin";
                btn.hotkey      = 'p';
                btn.hotkey_idx  = 0;
            }
            return true;
        }
        return false;
    }

    bool Techtree::SetupSpellButton(GUI::ActionButtonDescription& btn, bool isOrc) const {
        ASSERT_MSG(btn.command_id == GUI::ActionButton_CommandType::CAST, "Techtree::ButtonSetup - invalid command type detected ({})", btn.command_id);
        
        auto[race, research] = SpellID::TechtreeInfo(btn.payload_id);

        if(btn.payload_id <= SpellID::DEATH_AND_DECAY)
            return (data[race].research[research] > 0);
        else
            return true;
    }

    bool Techtree::ResearchDependenciesMet(int research_type, bool isOrc) const {
        switch(research_type) {
            case ResearchType::LM_RANGE:
            case ResearchType::LM_SIGHT:
            case ResearchType::LM_UNIQUE:
                return data[int(isOrc)].research[ResearchType::LM_RANGER_UPGRADE] > 0;
            case ResearchType::PALA_HEAL:
            case ResearchType::PALA_EXORCISM:
                return data[int(isOrc)].research[ResearchType::PALA_UPGRADE] > 0;
            default:
                return true;
        }
    }

    bool Techtree::ResearchConstrained(int research_type, int current_level) const {
        int max_level = Resources::LoadResearchLevels(research_type) - research_limits[research_type];
        return current_level >= max_level;
    }

    bool Techtree::TrainingConstrained(int unit_type) const {
        return unit_limits[unit_type];
    }

    bool Techtree::BuildingConstrained(int building_type) const {
        return building_limits[building_type];
    }

    bool Techtree::CastConditionCheck(int spellID, int unitMana) const {
        auto [race, research] = SpellID::TechtreeInfo(spellID);
        return (SpellID::IsResearchless(spellID) || data[race].research[research] > 0) && unitMana >= SpellID::Price(spellID);
    }

    bool Techtree::ApplyUnitUpgrade(int& unit_type, bool isOrc) const {
        int race = int(isOrc);
        if(unit_type == UnitType::ARCHER && data[race].research[ResearchType::LM_RANGER_UPGRADE]) {
            unit_type = UnitType::RANGER;
            return true;
        }
        else if(unit_type == UnitType::KNIGHT && data[race].research[ResearchType::PALA_UPGRADE]) {
            unit_type = UnitType::PALADIN;
            return true;
        }
        return false;
    }

    int Techtree::UnitUpgradeTier(bool attack_upgrade, int upgrade_type, bool isOrc) const {
        switch(upgrade_type) {
            case UnitUpgradeSource::BLACKSMITH:
            case UnitUpgradeSource::BLACKSMITH_BALLISTA:
                return data[int(isOrc)].research[ResearchType::MELEE_ATTACK + int(!attack_upgrade)];
            case UnitUpgradeSource::FOUNDRY:
                return data[int(isOrc)].research[ResearchType::NAVAL_ATTACK + int(!attack_upgrade)];
            case UnitUpgradeSource::LUMBERMILL:
                return data[int(isOrc)].research[attack_upgrade ? ResearchType::RANGED_ATTACK : ResearchType::MELEE_DEFENSE];
            default:
                return 0;
        }
    }

    glm::ivec3 Techtree::ResearchPrice(int research_type, bool isOrc, bool forCancel) const {
        ASSERT_MSG(((unsigned)research_type) < ((unsigned)ResearchType::COUNT), "Techtree::ResearchPrice - invalid research type");
        ResearchInfo info = {};
        int race = int(isOrc);
        int level = data[race].research[research_type] + int(forCancel);
        if(Resources::LoadResearchInfo(research_type, level, info)) {
            return info.data.price[race];
        }
        else {
            ENG_LOG_WARN("Techtree::ResearchPrice - research definition for (type={}, level={}) not found", research_type, level);
            return glm::ivec3(-1);
        }
    }

    float Techtree::ResearchTime(int research_type, bool isOrc) const {
        //TODO:
        return 100.f;
    }

    bool Techtree::IncrementResearch(int research_type, bool isOrc, int* out_level) {
        ASSERT_MSG(((unsigned)research_type) < ((unsigned)ResearchType::COUNT), "Techtree::ResearchPrice - invalid research type");
        int level = data[int(isOrc)].research[research_type];
        bool success = false;

        ResearchInfo info = {};
        if(Resources::LoadResearchInfo(research_type, level+1, info)) {
            data[int(isOrc)].research[research_type]++;
            success = true;

            RecalculateRace(isOrc);
        }

        if(out_level != nullptr)
            *out_level = data[int(isOrc)].research[research_type];
        return success;
    }

    void Techtree::RecalculateRace(bool isOrc) {
        data[int(isOrc)].RecomputeUnitLevels();
        data[int(isOrc)].RecomputeBonuses(isOrc);
    }

    void Techtree::RecalculateBoth() {
        RecalculateRace(false);
        RecalculateRace(true);
    }

    int Techtree::GetResearch(int research_type, bool isOrc) {
        ASSERT_MSG(((unsigned)research_type) < ((unsigned)ResearchType::COUNT), "Techtree::ResearchPrice - invalid research type");
        return data[int(isOrc)].research[research_type];
    }

    bool Techtree::HasResearchLimits() const {
        bool has_limits = false;
        for(int i = 0; i < ResearchType::COUNT; i++)
            has_limits |= research_limits[i] != 0;
        return has_limits;
    }

    bool Techtree::HasBuildingLimits() const {
        bool has_limits = false;
        for(int i = 0; i < BuildingType::COUNT; i++)
            has_limits |= building_limits[i];
        return has_limits;
    }

    bool Techtree::HasUnitLimits() const {
        bool has_limits = false;
        for(int i = 0; i < UnitType::COUNT; i++)
            has_limits |= unit_limits[i];
        return has_limits;
    }

    void Techtree::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX;
        // flags |= ImGuiTableFlags_NoHostExtendY;
        flags |= ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV;
        float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        float cell_width = TEXT_BASE_WIDTH * 3.f;

        static int race_idx = 0;
        ImGui::Text("Race:");
        ImGui::SameLine();
        if(ImGui::Button(race_idx == 0 ? "Human" : "Orc")) {
            race_idx = 1-race_idx;
        }

        ImGui::Text("Research levels:");
        if (ImGui::BeginTable("table1", ResearchType::COUNT, flags)) {
            for(int x = 0; x < ResearchType::COUNT; x++)
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width);

            ImGuiStyle& style = ImGui::GetStyle();
            ImGui::TableNextRow();
            for(int i = 0; i < ResearchType::COUNT; i++) {
                ImGui::TableNextColumn();
                ImGui::Text("%d", data[race_idx].research[i]);
            }
            
            
            ImGui::EndTable();
        }

        ImGui::Text("Limits:");
        if (ImGui::BeginTable("table2", ResearchType::COUNT+1, flags)) {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width*4);
            for(int x = 0; x < ResearchType::COUNT; x++)
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Research");
            for(int i = 0; i < ResearchType::COUNT; i++) {
                ImGui::TableNextColumn();
                int max_value = Resources::LoadResearchLevels(i);
                int display_value = max_value - research_limits[i];
                ImGui::Text("%d", display_value);
            }
            ImGui::EndTable();
        }
        if (ImGui::BeginTable("table3__", BuildingType::COUNT+1, flags)) {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width*4);
            for(int x = 0; x < BuildingType::COUNT; x++)
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Buildings");
            for(int i = 0; i < BuildingType::COUNT; i++) {
                ImGui::TableNextColumn();
                ImGui::Text("%d", (int)building_limits[i]);
            }
            ImGui::EndTable();
        }
        if (ImGui::BeginTable("table4", UnitType::COUNT+1, flags)) {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width*4);
            for(int x = 0; x < UnitType::COUNT; x++)
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Units");
            for(int i = 0; i < UnitType::COUNT; i++) {
                ImGui::TableNextColumn();
                ImGui::Text("%d", (int)unit_limits[i]);
            }
            ImGui::EndTable();
        }

        ImGui::Separator();
        ImGui::Text("Unit levels:");
        if (ImGui::BeginTable("table5", TechtreeLevelCounter::COUNT, flags)) {
            for(int x = 0; x < TechtreeLevelCounter::COUNT; x++)
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width);

            ImGuiStyle& style = ImGui::GetStyle();
            ImGui::TableNextRow();
            for(int i = 0; i < TechtreeLevelCounter::COUNT; i++) {
                ImGui::TableNextColumn();
                ImGui::Text("%d", data[race_idx].level_counters[i]);
            }
            
            ImGui::EndTable();
        }

        ImGui::Separator();
        ImGui::Text("Bonuses:");
        int colCount = std::max((int)TechtreeAttackBonusType::COUNT, (int)TechtreeArmorBonusType::COUNT);
        if (ImGui::BeginTable("table3", colCount+1, flags)) {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width*5);
            for(int x = 0; x < colCount; x++)
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width);

            ImGuiStyle& style = ImGui::GetStyle();
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Attack:");
            for(int i = 0; i < colCount; i++) {
                ImGui::TableNextColumn();
                if(i < TechtreeAttackBonusType::COUNT)
                    ImGui::Text("%d", data[race_idx].attack_bonus[i]);
                else
                    ImGui::Text("-");
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Armor:");
            for(int i = 0; i < colCount; i++) {
                ImGui::TableNextColumn();
                if(i < TechtreeArmorBonusType::COUNT)
                    ImGui::Text("%d", data[race_idx].armor_bonus[i]);
                else
                    ImGui::Text("-");
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Vision:");
            ImGui::TableNextColumn();
            ImGui::Text("%d", data[race_idx].vision_bonus);
            for(int i = 1; i < colCount; i++) {
                ImGui::TableNextColumn();
                ImGui::Text("-");
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Att. Range:");
            ImGui::TableNextColumn();
            ImGui::Text("%d", data[race_idx].range_bonus);
            for(int i = 1; i < colCount; i++) {
                ImGui::TableNextColumn();
                ImGui::Text("-");
            }
            
            ImGui::EndTable();
        }

        if(ImGui::Button("Recalculate")) {
            RecalculateBoth();
        }
#endif
    }

    void Techtree::EditableGUI() {
#ifdef ENGINE_ENABLE_GUI
        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX;
        // flags |= ImGuiTableFlags_NoHostExtendY;
        flags |= ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV;
        float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        float cell_width = TEXT_BASE_WIDTH * 3.f;
        char buf[64];

        ImU32 clr_green  = ImGui::GetColorU32(ImVec4(0.1f, 1.0f, 0.1f, 1.0f));
        ImU32 clr_gray   = ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
        ImU32 clr_red    = ImGui::GetColorU32(ImVec4(1.0f, 0.1f, 0.1f, 1.0f));

        ImGui::Text("Research levels:");
        if (ImGui::BeginTable("table1", ResearchType::COUNT+1, flags)) {
            for(int x = 0; x < ResearchType::COUNT+1; x++)
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width);

            ImGuiStyle& style = ImGui::GetStyle();
            for(int race_idx = 0; race_idx < 2; race_idx++) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text(race_idx ? "HU" : "OC");
                for(int i = 0; i < ResearchType::COUNT; i++) {
                    ImGui::TableNextColumn();
                    snprintf(buf, sizeof(buf), "%d###btn%d_%d", data[race_idx].research[i], race_idx, i);
                    if(ImGui::Button(buf, ImVec2(-FLT_MIN, 0.f))) {
                        data[race_idx].research[i]++;
                        if(data[race_idx].research[i] > Resources::LoadResearchLevels(i))
                            data[race_idx].research[i] = 0;
                    }
                }
            }
            ImGui::EndTable();
        }

        ImGui::Text("Limits:");
        if (ImGui::BeginTable("table2", ResearchType::COUNT+1, flags)) {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width*4);
            for(int x = 0; x < ResearchType::COUNT; x++)
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Research");
            for(int i = 0; i < ResearchType::COUNT; i++) {
                ImGui::TableNextColumn();
                int max_value = Resources::LoadResearchLevels(i);
                int display_value = max_value - research_limits[i];
                snprintf(buf, sizeof(buf), "%d###btn_r_%d", display_value, i);
                if(ImGui::Button(buf, ImVec2(-FLT_MIN, 0.f))) {
                    research_limits[i]++;
                    if(research_limits[i] > max_value)
                        research_limits[i] = 0;
                }
            }
            ImGui::EndTable();
        }
        if (ImGui::BeginTable("table3", BuildingType::COUNT+1, flags)) {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width*4);
            for(int x = 0; x < BuildingType::COUNT; x++)
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Buildings");
            for(int i = 0; i < BuildingType::COUNT; i++) {
                ImGui::TableNextColumn();
                int display_value = (int)building_limits[i];
                snprintf(buf, sizeof(buf), "%d###btn_b_%d", display_value, i);
                if(ImGui::Button(buf, ImVec2(-FLT_MIN, 0.f))) {
                    building_limits[i] = !building_limits[i];
                }
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, building_limits[i] ? clr_red : clr_gray);
            }
            ImGui::EndTable();
        }
        if (ImGui::BeginTable("table5", UnitType::COUNT+1, flags)) {
            ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width*4);
            for(int x = 0; x < UnitType::COUNT; x++)
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width);
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Units");
            for(int i = 0; i < UnitType::COUNT; i++) {
                ImGui::TableNextColumn();
                int display_value = (int)unit_limits[i];
                snprintf(buf, sizeof(buf), "%d###btn_u_%d", display_value, i);
                if(ImGui::Button(buf, ImVec2(-FLT_MIN, 0.f))) {
                    unit_limits[i] = !unit_limits[i];
                }
                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, unit_limits[i] ? clr_red : clr_gray);
            }
            ImGui::EndTable();
        }
#endif
    }


}//namespace eng
