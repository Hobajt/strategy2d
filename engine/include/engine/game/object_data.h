#pragma once

#include "engine/utils/mathdefs.h"
#include "engine/core/animator.h"

#include <string>
#include <memory>
#include <ostream>

namespace eng {

    //===== ObjectID =====

    //Unique identifier for any object within the ObjectPool.
    struct ObjectID {
        int type;
        int idx;
        int id;
    };

    //for logging purposes
    std::ostream& operator<<(std::ostream& os, const ObjectID& id);

    //===== GameObjectData =====

    struct GameObjectData {
        std::string name;
        glm::vec2 size;

        AnimatorDataRef animData = nullptr;
    };
    using GameObjectDataRef = std::shared_ptr<GameObjectData>;

    //===== UnitData =====

    struct UnitData : public GameObjectData {
        int navigationType = 1;
    };
    using UnitDataRef = std::shared_ptr<UnitData>;

    //===== BuildingData =====

    struct BuildingData {
        
    };
    using BuildingDataRef = std::shared_ptr<BuildingData>;

}//namespace eng
