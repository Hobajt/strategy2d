#pragma once

#include "engine/game/gameobject.h"
#include "engine/game/object_data.h"

#include "engine/utils/pool.hpp"

namespace eng {

    class ObjectPool;

    //===== EntranceController =====

    class EntranceController {
        struct Entry {
            ObjectID entered;
            ObjectID enteree;
            bool construction;
            int carry_state;
        };
        struct WorkEntry {
            ObjectID entered;
            ObjectID enteree;
            glm::ivec2 cmd_target;
            int cmd_type;
            float start_time;
        };
    public:
        void Update(ObjectPool& objects);

        bool IssueEntrance_Work(ObjectPool& objects, const ObjectID& buildingID, const ObjectID& workerID, const glm::ivec2& cmd_target, int cmd_type);

        bool IssueEntrance_Transport(ObjectPool& objects, const ObjectID& transportID, const ObjectID& unitID);
        //exit can fail if there's nowhere to spawn the unit
        bool IssueExit_Transport(ObjectPool& objects, const ObjectID& transportID, const ObjectID& unitID);

        bool IssueEntrance_Construction(ObjectPool& objects, const ObjectID& buildingID, const ObjectID& workerID);
        bool IssueExit_Construction(ObjectPool& objects, const ObjectID& buildingID);

        int KillObjectsInside(ObjectPool& objects, const ObjectID& id);
    private:
        bool IssueExit_Work(ObjectPool& objects, const WorkEntry& entry);

        void Goldmine_Increment(const ObjectID& gmID, Building* gm);
        void Goldmine_Decrement(const ObjectID& gmID, Building* gm);
    private:
        std::vector<Entry> entries;
        std::vector<WorkEntry> workEntries;
        std::vector<std::pair<ObjectID, int>> gms;
    };

    //===== ObjectPool =====

    class ObjectPool {
        using UnitsPool = pool<Unit, ObjectID::dtype, ObjectID::dtype>;
        using BuildingsPool = pool<Building, ObjectID::dtype, ObjectID::dtype>;
        using UtilityObjsPool = pool<UtilityObject, ObjectID::dtype, ObjectID::dtype>;
    public:
        void Update();
        void Render();

        //Update units of certain type to new type.
        void UnitUpgrade(int factionID, int old_type, int new_type, bool isOrcUnit);

        //===== entrance controller API =====

        bool IssueEntrance_Work(const ObjectID& buildingID, const ObjectID& workerID, const glm::ivec2& cmd_target, int cmdType) { return entranceController.IssueEntrance_Work(*this, buildingID, workerID, cmd_target, cmdType); }

        bool IssueEntrance_Transport(const ObjectID& transportID, const ObjectID& unitID) { return entranceController.IssueEntrance_Transport(*this, transportID, unitID); }
        bool IssueExit_Transport(const ObjectID& transportID, const ObjectID& unitID) { return entranceController.IssueEntrance_Transport(*this, transportID, unitID); }

        bool IssueEntrance_Construction(const ObjectID& buildingID, const ObjectID& workerID) { return entranceController.IssueEntrance_Construction(*this, buildingID, workerID); }
        bool IssueExit_Construction(const ObjectID& buildingID) { return entranceController.IssueExit_Construction(*this, buildingID); }

        int KillObjectsInside(const ObjectID& id) { return entranceController.KillObjectsInside(*this, id); }

        //===== getters =====

        ObjectID GetObjectAt(const glm::ivec2& map_coords);

        Unit& GetUnit(const ObjectID& id);
        Building& GetBuilding(const ObjectID& id);
        FactionObject& GetObject(const ObjectID& id);

        bool GetUnit(const ObjectID& id, Unit*& ref_unit, bool exclude_inactive = true);
        bool GetBuilding(const ObjectID& id, Building*& ref_building, bool exclude_inactive = true);
        bool GetObject(const ObjectID& id, FactionObject*& ref_object, bool exclude_inactive = true);

        //Updates building's identifier within the pool. Provided object_id will no longer be valid after this call.
        void UpdateBuildingID(const ObjectID& object_id);

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

        template <typename... Args>
        void QueueUtilityObject(Args&&... args) {
            to_spawn.emplace_back(std::forward<Args>(args)...);
        }

        void DBG_GUI();
    private:
        UnitsPool units;
        BuildingsPool buildings;
        UtilityObjsPool utilityObjs;

        std::vector<ObjectID::dtype> markedForRemoval;
        std::vector<UtilityObject> to_spawn;    //objects added from other objects Update() method

        EntranceController entranceController;
    };

}//namespace eng
