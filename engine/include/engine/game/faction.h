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
    private:
        std::vector<buildingMapCoords> dropoff_points;
    };
    using FactionControllerRef = std::shared_ptr<FactionController>;
    
}//namespace eng
