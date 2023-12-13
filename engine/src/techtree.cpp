#include "engine/game/techtree.h"

#include "engine/game/object_data.h"
#include "engine/game/gameobject.h"
#include "engine/game/resources.h"
#include "engine/utils/dbg_gui.h"

//represents column count in the icons spritesheet
#define ICON_MAX_X 10

namespace eng {

    glm::ivec2 Icon_AddOffset(const glm::ivec2& icon, int offset) {
        int x = icon.x + offset;
        return glm::ivec2(x % ICON_MAX_X, icon.y + (x / ICON_MAX_X));
    }

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

    void TechtreeData::RecomputeBonuses() {
        attack_bonus[TechtreeAttackBonusType::MELEE]    = Resources::LoadResearchBonus(ResearchType::MELEE_ATTACK, research[ResearchType::MELEE_ATTACK]);
        attack_bonus[TechtreeAttackBonusType::NAVAL]    = Resources::LoadResearchBonus(ResearchType::NAVAL_ATTACK, research[ResearchType::NAVAL_ATTACK]);
        attack_bonus[TechtreeAttackBonusType::RANGED]   = Resources::LoadResearchBonus(ResearchType::RANGED_ATTACK, research[ResearchType::RANGED_ATTACK]);
        attack_bonus[TechtreeAttackBonusType::SIEGE]    = Resources::LoadResearchBonus(ResearchType::SIEGE_ATTACK, research[ResearchType::SIEGE_ATTACK]);

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
        glm::ivec3{ TechtreeAttackBonusType::SIEGE,     TechtreeArmorBonusType::NONE,   TechtreeLevelCounter::SIEGE },     //ballista
        glm::ivec3{ TechtreeAttackBonusType::MELEE,     TechtreeArmorBonusType::MELEE,  TechtreeLevelCounter::MELEE },     //knight
        glm::ivec3{ TechtreeAttackBonusType::MELEE,     TechtreeArmorBonusType::MELEE,  TechtreeLevelCounter::PALA },      //paladin
        glm::ivec3{ TechtreeAttackBonusType::NONE,      TechtreeArmorBonusType::NONE,   TechtreeLevelCounter::NONE },      //tanker
        glm::ivec3{ TechtreeAttackBonusType::NAVAL,     TechtreeArmorBonusType::NAVAL,  TechtreeLevelCounter::NAVAL },     //destroyer
        glm::ivec3{ TechtreeAttackBonusType::NAVAL,     TechtreeArmorBonusType::NAVAL,  TechtreeLevelCounter::NAVAL },     //battleship
        glm::ivec3{ TechtreeAttackBonusType::NAVAL,     TechtreeArmorBonusType::NAVAL,  TechtreeLevelCounter::NAVAL },     //submarine
        glm::ivec3{ TechtreeAttackBonusType::NONE,      TechtreeArmorBonusType::NONE,   TechtreeLevelCounter::NONE },      //roflcopter
        glm::ivec3{ TechtreeAttackBonusType::NONE,      TechtreeArmorBonusType::NONE,   TechtreeLevelCounter::NONE },      //demosquad
        glm::ivec3{ TechtreeAttackBonusType::NONE,      TechtreeArmorBonusType::NONE,   TechtreeLevelCounter::MAGE },      //mage
        glm::ivec3{ TechtreeAttackBonusType::NONE,      TechtreeArmorBonusType::NONE,   TechtreeLevelCounter::NONE },      //gryphon
    };

    int Techtree::UnitLevel(int unit_type, bool isOrc) const {
        return data[int(isOrc)].level_counters[idx_LUT[unit_type].z];
    }

    int Techtree::BonusArmor(int unit_type, bool isOrc) const {
        return data[int(isOrc)].armor_bonus[idx_LUT[unit_type].y];
    }

    int Techtree::BonusDamage(int unit_type, bool isOrc) const {
        return data[int(isOrc)].attack_bonus[idx_LUT[unit_type].x];
    }

    int Techtree::BonusVision(int unit_type, bool isOrc) const {
        return (unit_type == UnitType::ARCHER) * data[int(isOrc)].vision_bonus;
    }

    int Techtree::BonusRange(int unit_type, bool isOrc) const {
        return (unit_type == UnitType::ARCHER) * data[int(isOrc)].range_bonus;
    }

    bool Techtree::SetupResearchButtonVisuals(GUI::ActionButtonDescription& btn, bool isOrc) const {
        ASSERT_MSG(((unsigned)btn.payload_id) < ((unsigned)ResearchType::COUNT), "Techtree::SetupResearchButtonVisuals - invalid research type");

        int race = int(isOrc);
        int type = btn.payload_id;
        int level = data[race].research[btn.payload_id];

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

    void Techtree::RecalculateBoth() {
        RecalculateRace(false);
        RecalculateRace(true);
    }

    void Techtree::RecalculateRace(bool isOrc) {
        data[int(isOrc)].RecomputeUnitLevels();
        data[int(isOrc)].RecomputeBonuses();
    }

    //TODO: when exporting to JSON, only need to store the research array (other values can be recomputed on the fly)

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

        ImGui::Separator();
        ImGui::Text("Unit levels:");
        if (ImGui::BeginTable("table2", TechtreeLevelCounter::COUNT, flags)) {
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


}//namespace eng
