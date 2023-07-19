#pragma once

#include "engine/utils/mathdefs.h"
#include "engine/core/animator.h"

#include <string>
#include <memory>
#include <ostream>

namespace eng {

    //===== ObjectID =====

    namespace ObjectType { enum { INVALID, GAMEOBJECT, UNIT, BUILDING, UTILITY }; }

    //Unique identifier for any object within the ObjectPool.
    struct ObjectID {
        using dtype = size_t;
    public:
        dtype type = ObjectType::INVALID;
        dtype idx;
        dtype id;
    public:
        ObjectID() = default;
        ObjectID(dtype type_, dtype idx_, dtype id_) : type(type_), idx(idx_), id(id_) {}

        static bool IsValid(const ObjectID& id) { return id.type != ObjectType::INVALID; }
    };

    //for logging purposes
    std::ostream& operator<<(std::ostream& os, const ObjectID& id);

    //===== GameObjectData =====

    struct GameObjectData {
        std::string name;
        int objectType;

        glm::vec2 size;
        int navigationType;

        AnimatorDataRef animData = nullptr;
    public:
        virtual ~GameObjectData() = default;
        virtual int MaxHealth() const { return 0; }
    };
    using GameObjectDataRef = std::shared_ptr<GameObjectData>;

    //===== FactionObjectData =====

    struct FactionObjectData : public GameObjectData {
        int health;
    public:
        virtual int MaxHealth() const override { return health; }
    };
    using FactionObjectDataRef = std::shared_ptr<FactionObjectData>;

    //===== UnitData =====

    struct UnitData : public FactionObjectData {
        
    };
    using UnitDataRef = std::shared_ptr<UnitData>;

    //===== BuildingData =====

    struct BuildingData : public FactionObjectData {
        int attack_damage = 0;
        
        int upgrade_target = 1000;
    };
    using BuildingDataRef = std::shared_ptr<BuildingData>;

}//namespace eng
