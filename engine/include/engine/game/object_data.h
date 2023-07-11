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
        int navigationType;

        AnimatorDataRef animData = nullptr;
    public:
        virtual bool IsBuilding() const { return false; }
    };
    using GameObjectDataRef = std::shared_ptr<GameObjectData>;

    //===== UnitData =====

    struct UnitData : public GameObjectData {
        
    };
    using UnitDataRef = std::shared_ptr<UnitData>;

    //===== BuildingData =====

    struct BuildingData : public GameObjectData {
        int attack_damage = 0;

        int health = 1200;

        int upgrade_target = 1000;
    public:
        virtual bool IsBuilding() const { return true; }
    };
    using BuildingDataRef = std::shared_ptr<BuildingData>;

}//namespace eng
