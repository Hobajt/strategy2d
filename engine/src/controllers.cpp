#include "engine/game/controllers.h"

namespace eng {

    //===== NatureFactionController =====

    NatureFactionController::NatureFactionController(FactionsFile::FactionEntry&& entry, const glm::ivec2& mapSize) : FactionController(std::move(entry), mapSize, FactionControllerID::NATURE) {}

    int NatureFactionController::GetColorIdx(const glm::ivec3& num_id) const {
        switch(num_id[0]) {
            case ObjectType::BUILDING:
                switch(num_id[1]) {
                    case 1: //gold_mine
                        return 7;
                }
        }
        return 5;
    }

}//namespace eng
