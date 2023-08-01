#pragma once

#include "engine/game/gameobject.h"
#include "engine/game/object_data.h"

#include "engine/utils/pool.hpp"

namespace eng {

    //===== ObjectPool =====

    class ObjectPool {
        using UnitsPool = pool<Unit, ObjectID::dtype, ObjectID::dtype>;
        using BuildingsPool = pool<Building, ObjectID::dtype, ObjectID::dtype>;
        using UtilityObjsPool = pool<UtilityObject, ObjectID::dtype, ObjectID::dtype>;
    public:
        void Update();
        void Render();

        //===== getters =====

        ObjectID GetObjectAt(const glm::ivec2& map_coords);

        Unit& GetUnit(const ObjectID& id);
        Building& GetBuilding(const ObjectID& id);
        FactionObject& GetObject(const ObjectID& id);

        bool GetUnit(const ObjectID& id, Unit*& ref_unit);
        bool GetBuilding(const ObjectID& id, Building*& ref_building);
        bool GetObject(const ObjectID& id, FactionObject*& ref_object);

        //===== insert =====

        ObjectID Add(Unit&& unit);
        ObjectID Add(Building&& building);
        ObjectID Add(UtilityObject&& utilityObj);
        
        template <typename... Args>
        ObjectID EmplaceUnit(Args&&... args) {
            UnitsPool::key key = units.emplace(GameObject::PeekNextID(), std::forward<Args>(args)...);
            ObjectID oid = ObjectID(ObjectType::UNIT, key.idx, key.id);
            units[key].IntegrateIntoLevel(oid);
            return oid;
        }

        template <typename... Args>
        ObjectID EmplaceBuilding(Args&&... args) {
            BuildingsPool::key key = buildings.emplace(GameObject::PeekNextID(), std::forward<Args>(args)...);
            ObjectID oid = ObjectID(ObjectType::BUILDING, key.idx, key.id);
            buildings[key].IntegrateIntoLevel(oid);
            return oid;
        }

        template <typename... Args>
        ObjectID EmplaceUtilityObj(Args&&... args) {
            UtilityObjsPool::key key = utilityObjs.emplace(GameObject::PeekNextID(), std::forward<Args>(args)...);
            ObjectID oid = ObjectID(ObjectType::UTILITY, key.idx, key.id);
            utilityObjs[key].IntegrateIntoLevel(oid);
            return oid;
        }

        void DBG_GUI();
    private:
        UnitsPool units;
        BuildingsPool buildings;
        UtilityObjsPool utilityObjs;
    };

}//namespace eng
