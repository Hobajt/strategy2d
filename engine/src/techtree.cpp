#include "engine/game/techtree.h"

#include "engine/game/object_data.h"
#include "engine/game/gameobject.h"
#include "engine/game/resources.h"
#include "engine/utils/dbg_gui.h"

namespace eng {

    //===== ResearchID =====

    std::string ResearchID::to_string() const { 
        char buf[512];
        snprintf(buf, sizeof(buf), "(%d, %d)", type, level);
        return std::string(buf);
    }

    std::ostream& operator<<(std::ostream& os, const ResearchID& id) {
        os << id.to_string();
        return os;
    }

    bool operator==(const ResearchID& lhs, const ResearchID& rhs) {
        return lhs.type == rhs.type && lhs.level == rhs.level;
    }

    //===== Techtree =====

    int Techtree::UnitLevel(int unit_type) const {
        //TODO:
        return 0;
    }

    //TODO: fillup properly
    constexpr static std::array<glm::ivec2, UnitType::COUNT> idx_LUT = {
        glm::ivec2{ TechtreeAttackBonusType::NONE, TechtreeArmorBonusType::NONE },      //peasant
        glm::ivec2{ TechtreeAttackBonusType::MELEE, TechtreeArmorBonusType::MELEE },      //footman
        glm::ivec2{ TechtreeAttackBonusType::NONE, TechtreeArmorBonusType::NONE },      //archer
        glm::ivec2{ TechtreeAttackBonusType::NONE, TechtreeArmorBonusType::NONE },      //ballista
        glm::ivec2{ TechtreeAttackBonusType::NONE, TechtreeArmorBonusType::NONE },      //knight
        glm::ivec2{ TechtreeAttackBonusType::NONE, TechtreeArmorBonusType::NONE },      //paladin
        glm::ivec2{ TechtreeAttackBonusType::NONE, TechtreeArmorBonusType::NONE },      //tanker
        glm::ivec2{ TechtreeAttackBonusType::NONE, TechtreeArmorBonusType::NONE },      //destroyer
        glm::ivec2{ TechtreeAttackBonusType::NONE, TechtreeArmorBonusType::NONE },      //battleship
        glm::ivec2{ TechtreeAttackBonusType::NONE, TechtreeArmorBonusType::NONE },      //submarine
        glm::ivec2{ TechtreeAttackBonusType::NONE, TechtreeArmorBonusType::NONE },      //roflcopter
        glm::ivec2{ TechtreeAttackBonusType::NONE, TechtreeArmorBonusType::NONE },      //demosquad
        glm::ivec2{ TechtreeAttackBonusType::NONE, TechtreeArmorBonusType::NONE },      //mage
        glm::ivec2{ TechtreeAttackBonusType::NONE, TechtreeArmorBonusType::NONE },      //gryphon
    };

    int Techtree::BonusArmor(int unit_type) const {
        return armor_bonus[idx_LUT[unit_type].y];
    }

    int Techtree::BonusDamage(int unit_type) const {
        return attack_bonus[idx_LUT[unit_type].x];
    }

    int Techtree::BonusVision(int unit_type) const {
        return (unit_type == UnitType::ARCHER) * vision_bonus;
    }

    int Techtree::BonusRange(int unit_type) const {
        return (unit_type == UnitType::ARCHER) * range_bonus;
    }


    void Techtree::SetupResearchButtonVisuals(GUI::ActionButtonDescription& btn) const {
        ASSERT_MSG(((unsigned)btn.payload_id) < ((unsigned)ResearchType::COUNT), "Techtree::SetupResearchButtonVisuals - invalid payload_id");
        ResearchID id = ResearchID(btn.payload_id, research[btn.payload_id]+1);
        ResearchInfo info = {};
        if(Resources::TryLoadResearchInfo(id, info)) {
            btn.name = info.name;
            btn.has_hotkey = info.has_hotkey;
            btn.hotkey = info.hotkey;
            btn.hotkey_idx = info.hotkey_idx;
            btn.price = info.price;
            btn.icon = info.icon;
        }
    }

    //TODO: when exporting to JSON, only need to store the research array (other values can be recomputed on the fly)

    void Techtree::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX;
        // flags |= ImGuiTableFlags_NoHostExtendY;
        flags |= ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV;
        float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        float cell_width = TEXT_BASE_WIDTH * 3.f;

        ImGui::Text("Research levels:");
        if (ImGui::BeginTable("table1", ResearchType::COUNT, flags)) {
            for(int x = 0; x < ResearchType::COUNT; x++)
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width);

            ImGuiStyle& style = ImGui::GetStyle();
            ImGui::TableNextRow();
            for(int i = 0; i < ResearchType::COUNT; i++) {
                ImGui::TableNextColumn();
                ImGui::Text("%d", research[i]);
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
                ImGui::Text("%d", level_counters[i]);
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
                    ImGui::Text("%d", attack_bonus[i]);
                else
                    ImGui::Text("-");
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Armor:");
            for(int i = 0; i < colCount; i++) {
                ImGui::TableNextColumn();
                if(i < TechtreeArmorBonusType::COUNT)
                    ImGui::Text("%d", armor_bonus[i]);
                else
                    ImGui::Text("-");
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Vision:");
            ImGui::TableNextColumn();
            ImGui::Text("%d", vision_bonus);
            for(int i = 1; i < colCount; i++) {
                ImGui::TableNextColumn();
                ImGui::Text("-");
            }

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Att. Range:");
            ImGui::TableNextColumn();
            ImGui::Text("%d", range_bonus);
            for(int i = 1; i < colCount; i++) {
                ImGui::TableNextColumn();
                ImGui::Text("-");
            }
            
            ImGui::EndTable();
        }


        
#endif
    }    

}//namespace eng
