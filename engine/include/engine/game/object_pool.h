#pragma once

#include "engine/game/gameobject.h"
#include "engine/game/object_data.h"

#include "engine/utils/pool.hpp"

namespace eng {

    //===== ObjectPool =====

    class ObjectPool {
        using UnitsPool = pool<Unit, ObjectID::dtype, ObjectID::dtype>;
        using BuildingsPool = pool<Building, ObjectID::dtype, ObjectID::dtype>;
    public:
        void Update();
        void Render();

        //===== getters =====

        ObjectID GetObjectAt(const glm::ivec2& map_coords);

        Unit& GetUnit(const ObjectID& id);
        Building& GetBuilding(const ObjectID& id);
        GameObject& GetObject(const ObjectID& id);

        bool GetUnit(const ObjectID& id, Unit*& ref_unit);
        bool GetBuilding(const ObjectID& id, Building*& ref_building);
        bool GetObject(const ObjectID& id, GameObject*& ref_object);

        //===== insert =====

        ObjectID Add(Unit&& unit);
        ObjectID Add(Building&& building);
        
        template <typename... Args>
        ObjectID EmplaceUnit(Args&&... args) {
            UnitsPool::key key = units.emplace(GameObject::PeekNextID(), std::forward<Args>(args)...);
            ObjectID oid = ObjectID(ObjectType::UNIT, key.idx, key.id);
            units[key].SetObjectID(oid);
            return oid;
        }

        template <typename... Args>
        ObjectID EmplaceBuilding(Args&&... args) {
            BuildingsPool::key key = units.emplace(GameObject::PeekNextID(), std::forward<Args>(args)...);
            ObjectID oid = ObjectID(ObjectType::BUILDING, key.idx, key.id);
            buildings[key].SetObjectID(oid);
            return oid;
        }

        void DBG_GUI();
    private:
        UnitsPool units;
        BuildingsPool buildings;
    };

}//namespace eng
