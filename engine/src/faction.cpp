#include "engine/game/faction.h"
#include "engine/game/gameobject.h"

namespace eng {

    int FactionController::GetColorIdx() const {
        //TODO:
        return 0;
    }

    void FactionController::AddDropoffPoint(const Building& building) {
        //TODO: print out legit faction identifier
        ENG_LOG_TRACE("Faction {} - registered dropoff point '{}' (mask={})", 0, building, building.DropoffMask());

        dropoff_points.push_back({ building.MinPos(), building.MaxPos(), building.OID(), building.DropoffMask() });
        //TODO: maybe add some assertion checks? - coords overlap, multiple entries with the same ID, ...
    }

    void FactionController::RemoveDropoffPoint(const Building& building) {
        //TODO: print out legit faction identifier
        ENG_LOG_TRACE("Faction {} - removed dropoff point '{}' (mask={})", 0, building, building.DropoffMask());

        buildingMapCoords entry = { building.MinPos(), building.MaxPos(), building.OID(), building.DropoffMask() };
        auto pos = std::find(dropoff_points.begin(), dropoff_points.end(), entry);
        if(pos != dropoff_points.end()) {
            dropoff_points.erase(pos);
        }
        else {
            ENG_LOG_WARN("Dropoff point entry not found.");
        }
    }

}//namespace eng
