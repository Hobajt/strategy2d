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

    //===== FactionsFile =====

    //Struct for serialization.
    struct FactionsFile {
        struct FactionEntry {
            int controllerID;
            int colorIdx;
            std::string name;
            Techtree techtree;
        };
    public:
        std::vector<FactionEntry> factions;
        std::vector<glm::ivec3> diplomacy;
    };
    
    //===== FactionController =====

    //TODO: make abstract
    class FactionController {
    public:
        FactionController() = default;
        FactionController(FactionsFile::FactionEntry&& entry);

        int ID() const { return id; }

        int GetColorIdx() const { return colorIdx; }

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
        int colorIdx = 0;
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

    //===== Factions =====

    class Factions {
    public:
        Factions() = default;
        Factions(FactionsFile&& data);

        const DiplomacyMatrix& Diplomacy() const { return diplomacy; }

        FactionControllerRef operator[](int i);
        const FactionControllerRef operator[](int i) const;

        void DBG_GUI();
    private:
        std::vector<FactionControllerRef> factions;
        DiplomacyMatrix diplomacy;
        bool initialized = false;
    };
    
}//namespace eng
