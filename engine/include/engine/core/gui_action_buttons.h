#pragma once

#include <string>
#include "engine/utils/mathdefs.h"

namespace eng::GUI {

    namespace ActionButton_Visuals { enum { CANCEL, ATTACK, MOVE, STOP, PATROL, STAND_GROUND }; }

        namespace ActionButton_CommandType {
            enum { 
                DISABLED = -2, PAGE_CHANGE = -1, 
                MOVE, STOP, STAND_GROUND, PATROL, ATTACK, CAST, ATTACK_GROUND,
                HARVEST, RETURN_GOODS, BUILD, REPAIR,
                TRAIN_OR_RESEARCH, UPGRADE
            };
        }

        struct ActionButtonDescription {
            int command_id = ActionButton_CommandType::DISABLED;
            int payload_id = -1;     //relevant only when command_id \in { PAGE_CHANGE, CAST, BUILD, TRAIN_OR_RESEARCH }

            bool has_hotkey;
            char hotkey;
            int hotkey_idx;
            
            std::string name;
            glm::ivec2 icon;
            glm::ivec4 price;
        public:
            ActionButtonDescription();
            ActionButtonDescription(int cID, int pID, bool has_hk, char hk, int hk_idx, const std::string& name, const glm::ivec2& icon, const glm::ivec4& price = glm::ivec4(0));

            static ActionButtonDescription Move(bool isOrc, bool water_units);
            static ActionButtonDescription Patrol(bool isOrc, bool water_units);
            static ActionButtonDescription StandGround(bool isOrc);
            static ActionButtonDescription AttackGround();
            static ActionButtonDescription Stop(bool isOrc, int upgrade_type, int upgrade_tier);
            static ActionButtonDescription Attack(bool isOrc, int upgrade_type, int upgrade_tier);

            static ActionButtonDescription Repair();
            static ActionButtonDescription Harvest(bool water_units);
            static ActionButtonDescription ReturnGoods(bool isOrc, bool water_units);

            static ActionButtonDescription CancelButton();

            static int GetIconIdx(bool attack, int ugprade_type, int upgrade_tier, bool isOrc);

            static glm::ivec4 MissingVisuals() { return glm::ivec4(-69); }
        };

}//namespace eng::GUI
