#pragma once

#include <memory>
#include <vector>
#include <tuple>

#include "engine/game/map.h"
#include "engine/game/gameobject.h"

namespace eng {

    namespace ResearchType {
        enum {
            MELEE_ATTACK, MELEE_DEFENSE, NAVAL_ATTACK, NAVAL_DEFENSE, RANGED_ATTACK, SIEGE_ATTACK,
            LM_RANGER_UPGRADE, LM_SIGHT, LM_RANGE, LM_UNIQUE,
            PALA_UPGRADE, PALA_HEAL, PALA_EXORCISM,
            MAGE_SLOW, MAGE_SHIELD, MAGE_INVIS, MAGE_POLY, MAGE_BLIZZARD,
            COUNT
        };
    }

    class Level;
    class Building;

    //can setup enum for the IDs as well (PLAYER_LOCAL, PLAYER_REMOTE1, PLAYER_REMOTEn, AI_EASY, ...)
    namespace FactionControllerID {
        enum { INVALID = 0, LOCAL_PLAYER, NATURE };

        int RandomAIMindset();
    }

    //===== Techtree =====

    class Techtree {
    public:
        void DBG_GUI();
    private:
        std::array<uint8_t, ResearchType::COUNT> research;
        std::array<uint8_t, 4> level_counters;      //melee_upgrade_count, naval_upgrade_count, siege_upgrade_count, lm_upgrade_count, pala_upgrade_count, mage_upgrade_count
    };

    namespace ResourceBit { enum { GOLD = 1, WOOD = 2, OIL = 4 }; }
    const char* GameResourceName(int res_idx);

    //===== FactionsFile =====

    //Struct for serialization.
    struct FactionsFile {
        struct FactionEntry {
            int controllerID;
            int colorIdx;
            int race;
            std::string name;
            Techtree techtree;
            std::vector<uint32_t> occlusionData;
        public:
            FactionEntry() = default;
            FactionEntry(int controllerID, int race, const std::string& name, int colorIdx);
        };
    public:
        std::vector<FactionEntry> factions;
        std::vector<glm::ivec3> diplomacy;
    };

    struct FactionStats {
        int unitCount = 0;
        int buildingCount = 0;
        std::array<int, BuildingType::COUNT> buildings = {0};
        std::array<int, 3> mainHallCounts = {0};
        int tier = 0;
    };
    
    //===== FactionController =====

    class FactionController;
    using FactionControllerRef = std::shared_ptr<FactionController>;

    //TODO: make abstract
    class FactionController {
    public:
        //Factory method, creates proper controller type (based on controllerID).
        static FactionControllerRef CreateController(FactionsFile::FactionEntry&& entry, const glm::ivec2& mapSize);
    public:
        FactionController() = default;
        FactionController(FactionsFile::FactionEntry&& entry, const glm::ivec2& mapSize, int controllerID);

        int ID() const { return id; }
        int Race() const { return race; }
        int ControllerID() const { return controllerID; }

        virtual int GetColorIdx(const glm::ivec3& num_id) const { return colorIdx; }

        FactionsFile::FactionEntry Export();

        //triggered when spawning objects, to increment faction's stats & counters (such as population)
        void ObjectAdded(const FactionObjectDataRef& data);
        void ObjectRemoved(const FactionObjectDataRef& data);
        void ObjectUpgraded(const FactionObjectDataRef& old_data, const FactionObjectDataRef& new_data);

        void AddDropoffPoint(const Building& building);
        void RemoveDropoffPoint(const Building& building);
        const std::vector<buildingMapCoords>& DropoffPoints() const { return dropoff_points; }

        //Returns true if all the conditions for given building are met (including resources check).
        bool CanBeBuilt(int buildingID, bool orcBuildings) const;

        //Returns data object for particular type of building. Returns nullptr if the building cannot be built.
        BuildingDataRef FetchBuildingData(int buildingID, bool orcBuildings);

        //Lookup research tier for unit upgrades.
        int UnitUpgradeTier(bool attack_upgrade, int upgrade_type);

        virtual void Update(Level& level) {}

        glm::ivec3 Resources() const { return resources; }
        glm::ivec2 Population() const { return population; }

        int ProductionBoost(int res_idx);

        bool ButtonConditionCheck(const FactionObject& src, const GUI::ActionButtonDescription& btn) const;

        bool CastConditionCheck(const Unit& src, int payload_id) const;

        void DBG_GUI();
    private:
        virtual void Inner_DBG_GUI() {}
        virtual std::vector<uint32_t> ExportOcclusion() { return {}; }

        void PopulationUpdate();
    private:
        Techtree techtree;
        FactionStats stats;
        std::vector<buildingMapCoords> dropoff_points;

        int id = -1;
        int colorIdx = 0;
        std::string name = "unnamed_faction";
        int controllerID = -1;

        int race = 0;
        glm::ivec3 resources = glm::ivec3(0);
        glm::ivec2 population = glm::ivec2(0);

        static int idCounter;
    };

    //===== DiplomacyMatrix =====

    //Describes relationships between individual factions.
    class DiplomacyMatrix {
    public:
        DiplomacyMatrix() = default;

        DiplomacyMatrix(int factionCount);
        DiplomacyMatrix(int factionCount, const std::vector<glm::ivec3>& relations);
        ~DiplomacyMatrix();

        //copy disabled
        DiplomacyMatrix(const DiplomacyMatrix&) = delete;
        DiplomacyMatrix& operator=(const DiplomacyMatrix&) const = delete;

        //move enabled
        DiplomacyMatrix(DiplomacyMatrix&&) noexcept;
        DiplomacyMatrix& operator=(DiplomacyMatrix&&) noexcept;

        bool AreHostile(int f1, int f2) const;

        std::vector<glm::ivec3> Export() const;
        
        void DBG_GUI();
    private:
        int& operator()(int y, int x);
        const int& operator()(int y, int x) const;

        void Move(DiplomacyMatrix&&) noexcept;
        void Release() noexcept;
    private:
        int* relation = nullptr;
        int factionCount = 0;
    };

    //===== Factions =====

    class PlayerFactionController;
    using PlayerFactionControllerRef = std::shared_ptr<PlayerFactionController>;

    class Factions {
    public:
        Factions() = default;
        Factions(FactionsFile&& data, const glm::ivec2& mapSize);

        const DiplomacyMatrix& Diplomacy() const { return diplomacy; }

        PlayerFactionControllerRef Player() { return player; }

        FactionControllerRef operator[](int i);
        const FactionControllerRef operator[](int i) const;

        std::vector<FactionControllerRef>::iterator begin() { return factions.begin(); }
        std::vector<FactionControllerRef>::iterator end() { return factions.end(); }
        std::vector<FactionControllerRef>::const_iterator begin() const { return factions.cbegin(); }
        std::vector<FactionControllerRef>::const_iterator end() const { return factions.cend(); }

        void Update(Level& level);

        bool IsInitialized() const { return initialized; }

        FactionsFile Export();

        void DBG_GUI();
    private:
        std::vector<FactionControllerRef> factions;
        DiplomacyMatrix diplomacy;
        bool initialized = false;

        PlayerFactionControllerRef player = nullptr;
    };
    
}//namespace eng
