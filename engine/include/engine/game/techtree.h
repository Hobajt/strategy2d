#pragma once

#include <array>
#include <string>

#include "engine/utils/mathdefs.h"
#include "engine/core/gui_action_buttons.h"
#include "engine/game/object_data.h"

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

        //NOTE: dependency when updating this enum -> SpellID depends on the fact, that last N entries are all spells -> Techtree::SetupSpellButton
    }

    namespace SpellID {
        enum {
            //research required
            HEAL, EXORCISM, SLOW, FLAME_SHIELD, INVISIBILITY, POLYMORPH, BLIZZARD, 
            BLOODLUST, RUNES, HASTE, RAISE_DEAD, TORNADO, UNHOLY_ARMOR, DEATH_AND_DECAY,

            //researchless
            HOLY_VISION, EYE, FIREBALL, DEATH_COIL,
            COUNT,

            INVALID_PRICE = 9999
        };

        //converts spellID to race and ResearchType.
        std::pair<int,int> TechtreeInfo(int spellID);
        bool IsResearchless(int spellID);

        int Price(int spellID);
        void SetPrice(int spellID, int price);

        bool RequiresTarget(int spellID);
        bool TargetlessCast(int spellID);
        bool IsContinuous(int spellID);

        bool SimplePrice(int spellID);
        bool TargetConditionCheck(int spellID, Level& level, const ObjectID& targetID);

        int CastingRange(int spellID);

        const char* SpellName(int spellID);

        int Spell2Buff(int spellID);
    }

    namespace TechtreeLevelCounter { enum { NONE, MELEE, NAVAL, SIEGE, RANGED, PALA, MAGE, COUNT }; }
    namespace TechtreeAttackBonusType { enum { NONE, MELEE, NAVAL, SIEGE, RANGED, COUNT }; }
    namespace TechtreeArmorBonusType { enum { NONE, MELEE, NAVAL, COUNT }; }

    //===== ResearchInfo =====

    struct ResearchVisuals {
        int type;
        glm::ivec2 icon[2];
        std::string name[2];

        bool has_hotkey;
        int hotkey_idx[2];
        char hotkey[2];

        int levels;
    };

    struct ResearchData {
        glm::ivec4 price[2];
        int value;
    };

    struct ResearchInfo {
        ResearchVisuals viz;
        ResearchData data;
    };

    struct TechtreeData {
        std::array<uint8_t, ResearchType::COUNT> research;                      //levels of inidividual researches

        std::array<uint8_t, TechtreeLevelCounter::COUNT> level_counters;        //precomputed values for unit level computation (sum of all their upgrades)
        std::array<uint8_t, TechtreeAttackBonusType::COUNT> attack_bonus;       //precomputed attack bonus (melee, naval, siege, ranged)
        std::array<uint8_t, TechtreeArmorBonusType::COUNT> armor_bonus;         //precomputed armor bonus (melee, naval)
        int vision_bonus;
        int range_bonus;
    public:
        void RecomputeUnitLevels();
        void RecomputeBonuses(bool isOrc);
    };

    //===== Techtree =====

    class Techtree {
    public:
        int UnitLevel(int unit_type, bool isOrc) const;

        int BonusArmor(int unit_type, bool isOrc) const;
        int BonusDamage(int unit_type, bool isOrc) const;
        int BonusVision(int unit_type, bool isOrc) const;
        int BonusRange(int unit_type, bool isOrc) const;

        //Updates price & visuals of the research button (to correspond with current researched level).
        bool SetupResearchButton(GUI::ActionButtonDescription& btn, bool isOrc) const;
        //Overrides the button in case it's an archer/knight unit and their upgrade has already been researched.
        bool SetupTrainButton(GUI::ActionButtonDescription& btn, bool isOrc) const;
        //Checks if given spell is already researched.
        bool SetupSpellButton(GUI::ActionButtonDescription& btn, bool isOrc) const;

        bool ResearchDependenciesMet(int research_type, bool isOrc) const;
        bool ResearchConstrained(int research_type, int current_level) const;
        bool TrainingConstrained(int unit_type) const;
        bool BuildingConstrained(int building_type) const;

        bool CastConditionCheck(int spellID, int unitMana) const;

        //Changes provided unit_type if the upgarde was researched.
        bool ApplyUnitUpgrade(int& unit_type, bool isOrc) const;

        int UnitUpgradeTier(bool attack_upgrade, int upgrade_type, bool isOrc) const;

        glm::ivec3 ResearchPrice(int research_type, bool isOrc, bool forCancel = true) const;
        float ResearchTime(int research_type, bool isOrc) const;

        bool IncrementResearch(int research_type, bool isOrc, int* out_level = nullptr);
        void RecalculateRace(bool isorc);
        void RecalculateBoth();

        int GetResearch(int type, bool isOrc);
        std::array<uint8_t, ResearchType::COUNT>& ResearchData(bool isOrc) { return data[int(isOrc)].research; }
        const std::array<uint8_t, ResearchType::COUNT>& ResearchData(bool isOrc) const { return data[int(isOrc)].research; }

        bool HasResearchLimits() const;
        std::array<uint8_t, ResearchType::COUNT>& ResearchLimits() { return research_limits; }
        const std::array<uint8_t, ResearchType::COUNT>& ResearchLimits() const { return research_limits; }

        bool HasBuildingLimits() const;
        std::array<bool, BuildingType::COUNT>& BuildingLimits() { return building_limits; }
        const std::array<bool, BuildingType::COUNT>& BuildingLimits() const { return building_limits; }

        bool HasUnitLimits() const;
        std::array<bool, UnitType::COUNT>& UnitLimits() { return unit_limits; }
        const std::array<bool, UnitType::COUNT>& UnitLimits() const { return unit_limits; }

        void DBG_GUI();
        void EditableGUI();
    private:
        std::array<TechtreeData, 2> data;
        std::array<uint8_t, ResearchType::COUNT> research_limits;
        std::array<bool, BuildingType::COUNT> building_limits;
        std::array<bool, UnitType::COUNT> unit_limits;
    };

}//namespace eng
