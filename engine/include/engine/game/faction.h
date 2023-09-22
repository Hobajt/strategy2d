#pragma once

#include <memory>
#include <vector>
#include <tuple>

#include "engine/game/map.h"

namespace eng {

    class Level;
    class Building;

    class Diplomacy {};

    class FactionController {
    public:
        int GetColorIdx() const;

        void AddDropoffPoint(const Building& building);
        void RemoveDropoffPoint(const Building& building);
        const std::vector<buildingMapCoords>& DropoffPoints() const { return dropoff_points; }

        //Returns true if all the conditions for given building are met (including resources check).
        bool CanBeBuilt(int buildingID, bool orcBuildings) const;

        //Returns data object for particular type of building. Returns nullptr if the building cannot be built.
        BuildingDataRef FetchBuildingData(int buildingID, bool orcBuildings);
    private:
        std::vector<buildingMapCoords> dropoff_points;
    };
    using FactionControllerRef = std::shared_ptr<FactionController>;
    
}//namespace eng
