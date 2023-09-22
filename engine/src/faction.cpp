#include "engine/game/faction.h"
#include "engine/game/gameobject.h"

#include "engine/game/resources.h"

namespace eng {

    int FactionController::GetColorIdx() const {
        //TODO:
        return 0;
    }

    void FactionController::AddDropoffPoint(const Building& building) {
        //TODO: maybe add some assertion checks? - coords overlap, multiple entries with the same ID, ...
        dropoff_points.push_back({ building.MinPos(), building.MaxPos(), building.OID(), building.DropoffMask() });

        //TODO: print out legit faction identifier
        ENG_LOG_TRACE("Faction {} - registered dropoff point '{}' (mask={}, count={})", 0, building, building.DropoffMask(), dropoff_points.size());
    }

    void FactionController::RemoveDropoffPoint(const Building& building) {
        buildingMapCoords entry = { building.MinPos(), building.MaxPos(), building.OID(), building.DropoffMask() };
        auto pos = std::find(dropoff_points.begin(), dropoff_points.end(), entry);
        if(pos != dropoff_points.end()) {
            dropoff_points.erase(pos);
        }
        else {
            ENG_LOG_WARN("Dropoff point entry not found.");
        }

        //TODO: print out legit faction identifier
        ENG_LOG_TRACE("Faction {} - removed dropoff point '{}' (mask={}, count={})", 0, building, building.DropoffMask(), dropoff_points.size());
    }

    bool FactionController::CanBeBuilt(int buildingID, bool orcBuildings) const {
        //have a precomputed bitmap - condition check for each building type
        //have values for both races, in case you somehow obtain worker of the other race
        //do resources check on the fly
        return true;
    }

    BuildingDataRef FactionController::FetchBuildingData(int buildingID, bool orcBuildings) {
        if(CanBeBuilt(buildingID, orcBuildings))
            return Resources::LoadBuilding(buildingID, orcBuildings);
        else
            return nullptr;
    }

}//namespace eng
