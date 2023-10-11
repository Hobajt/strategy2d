#include "engine/core/gui_action_buttons.h"
#include "engine/game/object_data.h"

namespace eng::GUI {

    GUI::ActionButtonDescription::ActionButtonDescription() : name("INVALID"), price(glm::ivec4(0)), icon(glm::ivec2(0)) {}

    GUI::ActionButtonDescription::ActionButtonDescription(int cID, int pID, bool has_hk, char hk, int hk_idx, const std::string& name_, const glm::ivec2& icon_, const glm::ivec4& price_)
        : command_id(cID), payload_id(pID), has_hotkey(has_hk), hotkey(hk), hotkey_idx(hk_idx), name(name_), icon(icon_), price(price_) {}

    GUI::ActionButtonDescription GUI::ActionButtonDescription::Move(bool isOrc, bool water_units) {
        glm::ivec2 icon = water_units ? glm::ivec2(6, 15) : glm::ivec2(3, 8);
        icon.x += int(isOrc);
        return ActionButtonDescription(ActionButton_CommandType::MOVE, -1, true, 'm', 0, "MOVE", icon);
    }

    GUI::ActionButtonDescription GUI::ActionButtonDescription::Patrol(bool isOrc, bool water_units) {
        glm::ivec2 icon = water_units ? glm::ivec2(6, 17) : glm::ivec2(0, 17);
        icon.x += int(isOrc);
        return ActionButtonDescription(ActionButton_CommandType::PATROL, -1, true, 'p', 0, "PATROL", icon);
    }

    GUI::ActionButtonDescription GUI::ActionButtonDescription::StandGround(bool isOrc) {
        glm::ivec2 icon = glm::ivec2(2 + int(isOrc), 17);
        return ActionButtonDescription(ActionButton_CommandType::STAND_GROUND, -1, true, 't', 1, "STAND GROUND", icon);
    }

    GUI::ActionButtonDescription GUI::ActionButtonDescription::AttackGround() {
        return ActionButtonDescription(ActionButton_CommandType::ATTACK_GROUND, -1, true, 'g', 7, "ATTACK GROUND", glm::ivec2(4, 17));
    }

    GUI::ActionButtonDescription GUI::ActionButtonDescription::Stop(bool isOrc, int upgrade_type, int upgrade_tier) {
        int idx = GetIconIdx(false, upgrade_type, upgrade_tier, isOrc);
        glm::ivec2 icon = glm::ivec2(idx % 10, idx / 10);
        return ActionButtonDescription(ActionButton_CommandType::STOP, -1, true, 's', 0, "STOP", icon);
    }

    GUI::ActionButtonDescription GUI::ActionButtonDescription::Attack(bool isOrc, int upgrade_type, int upgrade_tier) {
        int idx = GetIconIdx(true, upgrade_type, upgrade_tier, isOrc);
        glm::ivec2 icon = glm::ivec2(idx % 10, idx / 10);
        return ActionButtonDescription(ActionButton_CommandType::ATTACK, -1, true, 'a', 0, "ATTACK", icon);
    }

    GUI::ActionButtonDescription GUI::ActionButtonDescription::Repair() {
        return ActionButtonDescription(ActionButton_CommandType::REPAIR, -1, true, 'r', 0, "REPAIR", glm::ivec2(5,8));
    }

    GUI::ActionButtonDescription GUI::ActionButtonDescription::Harvest(bool water_units) {
        if(!water_units)
            return ActionButtonDescription(ActionButton_CommandType::HARVEST, -1, true, 'h', 0, "HARVEST LUMBER/MINE GOLD", glm::ivec2(6,8));
        else
            return ActionButtonDescription(ActionButton_CommandType::REPAIR, -1, true, 'h', 0, "HAUL OIL", glm::ivec2(1,16));
    }

    GUI::ActionButtonDescription GUI::ActionButtonDescription::ReturnGoods(bool isOrc, bool water_units) {
        glm::ivec2 icon = water_units ? glm::ivec2(8, 15) : (isOrc ? glm::ivec2(9, 8) : glm::ivec2(0, 9));
        return ActionButtonDescription(ActionButton_CommandType::RETURN_GOODS, -1, true, 'g', 13, "RETURN WITH GOODS", icon);
    }

    GUI::ActionButtonDescription GUI::ActionButtonDescription::CancelButton() {
        return ActionButtonDescription(ActionButton_CommandType::PAGE_CHANGE, 0, false, '/', -1, "Cancel", glm::ivec2(1, 9));
    }

    int GUI::ActionButtonDescription::GetIconIdx(bool attack, int upgrade_type, int upgrade_tier, bool isOrc) {
        constexpr static std::array<glm::ivec2, UnitUpgradeSource::COUNT> vals = {
            glm::ivec2(116, 164),     //NONE
            glm::ivec2(116, 164),     //BLACKSMITH
            glm::ivec2(144, 150),     //FOUNDRY
            glm::ivec2( 99, 164),     //CASTER
            glm::ivec2(124, 164),     //LUMBERMILL
            glm::ivec2(116, 164),     //BLACKSMITH_BALLISTA
        };
        return vals.at(upgrade_type)[1-int(attack)] + 3*int(isOrc) + upgrade_tier * int(upgrade_type > 0 && upgrade_type != UnitUpgradeSource::BLACKSMITH_BALLISTA);
    }

}//namespace eng::GUI
