#pragma once

#include <array>
#include <string>

#include "engine/utils/mathdefs.h"
#include "engine/core/gui_action_buttons.h"

namespace eng {

    struct ObjectID;

    namespace ResearchType {
        enum {
            MELEE_ATTACK, MELEE_DEFENSE, NAVAL_ATTACK, NAVAL_DEFENSE, RANGED_ATTACK, SIEGE_ATTACK,
            LM_RANGER_UPGRADE, LM_SIGHT, LM_RANGE, LM_UNIQUE,
            PALA_UPGRADE, PALA_HEAL, PALA_EXORCISM,
            MAGE_SLOW, MAGE_SHIELD, MAGE_INVIS, MAGE_POLY, MAGE_BLIZZARD,
            COUNT
        };
    }

    namespace TechtreeLevelCounter { enum { MELEE, NAVAL, SIEGE, RANGED, PALA, MAGE, COUNT }; }

    namespace TechtreeAttackBonusType { enum { NONE, MELEE, NAVAL, SIEGE, RANGED, COUNT }; }
    namespace TechtreeArmorBonusType { enum { NONE, MELEE, NAVAL, COUNT }; }

    //===== ResearchInfo =====

    struct ResearchVisuals {
        int type;
        glm::ivec2 icon[2];
        std::string name[2];

        bool has_hotkey;
        int hotkey_idx;
        char hotkey;
    };

    struct ResearchData {
        glm::ivec4 price;
        int value;
    };

    struct ResearchInfo {
        ResearchVisuals viz;
        ResearchData data;
    };

    //===== Techtree =====

    class Techtree {
    public:
        int UnitLevel(int unit_type) const;

        int BonusArmor(int unit_type) const;
        int BonusDamage(int unit_type) const;
        int BonusVision(int unit_type) const;
        int BonusRange(int unit_type) const;

        void SetupResearchButtonVisuals(GUI::ActionButtonDescription& btn, bool isOrc) const;

        glm::ivec3 ResearchPrice(int research_type) const;
        float ResearchTime(int research_type) const;

        void DBG_GUI();
    private:
        std::array<uint8_t, ResearchType::COUNT> research;                      //levels of inidividual researches

        std::array<uint8_t, TechtreeLevelCounter::COUNT> level_counters;        //precomputed values for unit level computation (sum of all their upgrades)
        std::array<uint8_t, TechtreeAttackBonusType::COUNT> attack_bonus;       //precomputed attack bonus (melee, naval, siege, ranged)
        std::array<uint8_t, TechtreeArmorBonusType::COUNT> armor_bonus;         //precomputed armor bonus (melee, naval)
        int vision_bonus;
        int range_bonus;
    };

}//namespace eng
