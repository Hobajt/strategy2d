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

    struct ResearchID {
        int type;
        int level;
    public:
        ResearchID() : type(-1), level(-1) {}
        ResearchID(int type_, int level_) : type(type_), level(level_) {}

        std::string to_string() const;
        friend std::ostream& operator<<(std::ostream& os, const ResearchID& id);
        friend bool operator==(const ResearchID& lhs, const ResearchID& rhs);
    };

    struct ResearchInfo {
        ResearchID id;
        std::string name;
        glm::ivec2 icon;
        glm::ivec4 price;
        int value;

        bool has_hotkey;
        int hotkey_idx;
        char hotkey;
    };

    //===== Techtree =====

    class Techtree {
    public:
        int UnitLevel(int unit_type) const;

        int BonusArmor(int unit_type) const;
        int BonusDamage(int unit_type) const;
        int BonusVision(int unit_type) const;
        int BonusRange(int unit_type) const;

        void SetupResearchButtonVisuals(GUI::ActionButtonDescription& btn) const;


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


//Hash function for eng::ResearchID
template<>
struct std::hash<eng::ResearchID> {
    size_t operator()(const eng::ResearchID& id) const {
        return std::hash<int>()(id.type) ^ (std::hash<int>()(id.level) << 1);
    }
};
