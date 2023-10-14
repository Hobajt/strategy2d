#pragma once

#include <memory>
#include <vector>
#include <tuple>

#include "engine/game/map.h"

namespace eng {

    class Level;
    class Building;

    //===== Techtree =====

    class Techtree {
    public:
        void DBG_GUI();
    private:

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
        public:
            FactionEntry() = default;
            FactionEntry(int controllerID, int race, const std::string& name, int colorIdx);
        };
    public:
        std::vector<FactionEntry> factions;
        std::vector<glm::ivec3> diplomacy;
    };
    
    //===== FactionController =====

    class FactionController;
    using FactionControllerRef = std::shared_ptr<FactionController>;

    //TODO: make abstract
    class FactionController {
    public:
        //Factory method, creates proper controller type (based on controllerID).
        static FactionControllerRef CreateController(FactionsFile::FactionEntry&& entry);
    public:
        FactionController() = default;
        FactionController(FactionsFile::FactionEntry&& entry);

        int ID() const { return id; }
        int Race() const { return race; }

        int GetColorIdx() const { return colorIdx; }

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

        void DBG_GUI();
    private:
        virtual void Inner_DBG_GUI() {}
    private:
        Techtree techtree;
        std::vector<buildingMapCoords> dropoff_points;

        int id = -1;
        int colorIdx = 0;
        std::string name = "unnamed_faction";

        int race = 0;
        glm::vec3 resources = glm::vec3(0);
        glm::vec2 population = glm::vec2(0);

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
        Factions(FactionsFile&& data);

        const DiplomacyMatrix& Diplomacy() const { return diplomacy; }

        PlayerFactionControllerRef Player() { return player; }

        FactionControllerRef operator[](int i);
        const FactionControllerRef operator[](int i) const;

        void Update(Level& level);

        void DBG_GUI();
    private:
        std::vector<FactionControllerRef> factions;
        DiplomacyMatrix diplomacy;
        bool initialized = false;

        PlayerFactionControllerRef player = nullptr;
    };
    
}//namespace eng
