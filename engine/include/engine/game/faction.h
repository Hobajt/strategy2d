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
    
    //===== FactionController =====

    //TODO: make abstract
    class FactionController {
    public:
        FactionController() = default;
        FactionController(const std::string& name);

        int ID() const { return id; }

        int GetColorIdx() const;

        void AddDropoffPoint(const Building& building);
        void RemoveDropoffPoint(const Building& building);
        const std::vector<buildingMapCoords>& DropoffPoints() const { return dropoff_points; }

        //Returns true if all the conditions for given building are met (including resources check).
        bool CanBeBuilt(int buildingID, bool orcBuildings) const;

        //Returns data object for particular type of building. Returns nullptr if the building cannot be built.
        BuildingDataRef FetchBuildingData(int buildingID, bool orcBuildings);

        void DBG_GUI();
    private:
        virtual void Inner_DBG_GUI() {}
    private:
        Techtree techtree;
        std::vector<buildingMapCoords> dropoff_points;

        int id = -1;
        std::string name = "unnamed_faction";

        static int idCounter;
    };
    using FactionControllerRef = std::shared_ptr<FactionController>;

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

    //===== FactionsFile =====

    //Struct for serialization.
    struct FactionsFile {
        struct FactionEntry {
            int controllerID;
            Techtree techtree;
            std::string name;
        };
    public:
        std::vector<FactionEntry> factions;
        std::vector<glm::ivec3> diplomacy;
    };

    //===== Factions =====

    class Factions {
    public:
        Factions() = default;
        Factions(const FactionsFile& data);

        const DiplomacyMatrix& Diplomacy() const { return diplomacy; }

        void DBG_GUI();
    private:
        std::vector<FactionControllerRef> factions;
        DiplomacyMatrix diplomacy;
        bool initialized = false;
    };
    
}//namespace eng
