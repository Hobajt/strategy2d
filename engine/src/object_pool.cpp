#include "engine/game/object_pool.h"

#include "engine/utils/dbg_gui.h"

#include "engine/core/input.h"
#include "engine/game/config.h"

#define WORKER_ENTRY_DURATION 1.f

namespace eng {

    int GetPreferredDirection(const glm::ivec2& target, const glm::ivec2& building);

    //===== EntranceController =====

    void EntranceController::Update(ObjectPool& objects) {
        //TODO: could maybe use some more fitting data structure, other than vector

        //timing updates & respawning for worker entrances
        float currentTime = (float)Input::CurrentTime();
        float duration = WORKER_ENTRY_DURATION / Config::GameSpeed();
        for(int i = (int)workEntries.size()-1; i >= 0; i--) {   
            if(workEntries[i].start_time + duration <= currentTime) {
                IssueExit_Work(objects, workEntries[i]);
                workEntries.erase(workEntries.begin() + i);
            }
        }


        /* EntranceController in general:
            - transport & construction should be easy to implement
                - no timing involved
                - only need to store pair of ObjectIDs per entrance request
                - exit is handled by issuing another request

            - work entrance:
                - need to store command info along with entrance data
                - need to use it to re-issue the command on object exiting (exiting will be issued from this update method)
                - how to manage the timing:
                    - there will probably be some shared constant that defines how long does a worker stay inside
                    - there's also the issue of game speed changing
                        - exit time will get invalidated whenever speed changes

            - how to handle container death:
                - can maybe do requests from within Kill() method
                - sufficient to only invoke from enterable objects
        */
    }

    bool EntranceController::IssueEntrance_Work(ObjectPool& objects, const ObjectID& buildingID, const ObjectID& workerID, const glm::ivec2& cmd_target, int cmd_type) {
        Unit* worker = nullptr;
        Building* building = nullptr;
        if(!objects.GetUnit(workerID, worker) || !objects.GetBuilding(buildingID, building)) {
            ENG_LOG_WARN("EntranceController::WorkEntrance - object not found.");
            return false;
        }

        if(!worker->IsWorker()) {
            ENG_LOG_WARN("EntranceController::WorkEntrance - provided unit is not a worker.");
            return false;
        }

        if(!worker->IsActive() || !building->IsActive()) {
            ENG_LOG_WARN("EntranceController::WorkEntrance - object is not active.");
            return false;
        }

        if(!building->CanDropoffResource(worker->CarryStatus()) && !building->IsGatherable(worker->NavigationType())) {
            ENG_LOG_WARN("EntranceController::WorkEntrance - resource type mismatch.");
            return false;
        }

        worker->WithdrawObject();
        workEntries.push_back({ buildingID, workerID, cmd_target, cmd_type, (float)Input::CurrentTime() });
        ENG_LOG_TRACE("EntranceController::WorkEntrance - Worker '{}' entered '{}'.", *worker, *building);
        return true;
    }

    bool EntranceController::IssueEntrance_Transport(ObjectPool& objects, const ObjectID& transportID, const ObjectID& unitID) {
        //TODO:
        throw std::runtime_error("not implemented yet");
    }

    bool EntranceController::IssueExit_Transport(ObjectPool& objects, const ObjectID& transportID, const ObjectID& unitID) {
        //TODO:
        throw std::runtime_error("not implemented yet");
    }

    bool EntranceController::IssueEntrance_Construction(ObjectPool& objects, const ObjectID& buildingID, const ObjectID& workerID) {
        //TODO:
        throw std::runtime_error("not implemented yet");
    }

    bool EntranceController::IssueExit_Construction(ObjectPool& objects, const ObjectID& buildingID, const ObjectID& workerID) {
        //TODO:
        throw std::runtime_error("not implemented yet");
    }

    bool EntranceController::IssueExit_Work(ObjectPool& objects, const WorkEntry& entry) {
        Unit* worker;
        Building* building;
        if(!objects.GetUnit(entry.enteree, worker, false) || !objects.GetBuilding(entry.entered, building)) {
            ENG_LOG_WARN("EntranceController::WorkExit - object not found.");
            return false;
        }

        Command cmd;
        glm::ivec2 respawn_position;
        int preferred_direction = 3;
        switch(entry.cmd_type) {
            case WorkerCarryState::GOLD:        //worker entered with gold -> return to Gather command
            {
                throw std::runtime_error("not implemented yet");
                // ObjectID goldmineID = convert entry.cmd_target to ObjectID;
                // Building* goldmine = nullptr;
                // cmd = Command::Gather(goldmineID);
                // if(objects.GetBuilding(goldmineID, goldmine)) {
                //     preferred_direction = GetPreferredDirection(goldmine->Position(), building->Position());
                // }
                break;
            }
            case WorkerCarryState::WOOD:        //worker entered with wood -> return to Harvest command
                cmd = Command::Harvest(entry.cmd_target);
                preferred_direction = GetPreferredDirection(entry.cmd_target, building->Position());
                break;
            default:                            //worker entered empty handed -> is in resource deposit -> ReturnGoods command
                cmd = Command::ReturnGoods();
                break;
        }

        building->NearbySpawnCoords(worker->NavigationType(), preferred_direction, respawn_position);
        worker->ReinsertObject(respawn_position);
        worker->IssueCommand(cmd);

        ENG_LOG_TRACE("EntranceController::WorkExit - Worker '{}' left '{}'.", *worker, *building);
        return true;
    }

    //===== ObjectPool =====

    void ObjectPool::Update() {
        entranceController.Update(*this);

        for(Unit& u : units) {
            if(u.Update()) {
                u.Kill();
                markedForRemoval.push_back(u.OID().idx);
            }
        }
        markedForRemoval.push_back((ObjectID::dtype)-1);
        
        for(Building& b : buildings) {
            if(b.Update()) {
                b.Kill();
                markedForRemoval.push_back(b.OID().idx);
            }
        }
        markedForRemoval.push_back((ObjectID::dtype)-1);
        
        for(UtilityObject& u : utilityObjs) {
            if(u.Update()) {
                u.Kill();
                markedForRemoval.push_back(u.OID().idx);
            }
        }

        int poolIdx = 0;
        for(ObjectID::dtype idx : markedForRemoval) {
            if(idx == (ObjectID::dtype)-1) {
                poolIdx++;
                continue;
            }

            switch(poolIdx) {
                case 0: units.remove(idx);          break;
                case 1: buildings.remove(idx);      break;
                case 2: utilityObjs.remove(idx);    break;
            }
        }
        markedForRemoval.clear();
    }

    void ObjectPool::Render() {
        for(Unit& u : units)
            u.Render();
        
        for(Building& b : buildings)
            b.Render();
        
        for(UtilityObject& u : utilityObjs)
            u.Render();
    }

    //===== getters =====

    ObjectID ObjectPool::GetObjectAt(const glm::ivec2& map_coords) {
        for(Unit& u : units) {
            if(u.CheckPosition(map_coords))
                return u.OID();
        }
        
        for(Building& b : buildings) {
            if(b.CheckPosition(map_coords))
                return b.OID();
        }
        
        return ObjectID();
    }

    Unit& ObjectPool::GetUnit(const ObjectID& id) {
        ASSERT_MSG(id.type != ObjectType::INVALID, "ObjectPool::GetUnit - Using an invalid ID to access a unit.");
        if(id.type != ObjectType::UNIT) {
            ENG_LOG_ERROR("ObjectPool::GetUnit - provided ID doesn't belong to a unit (type={})", id.type);
            throw std::invalid_argument("Attempting to fetch unit with ID that doesn't belong to unit.");
        }
        return units[UnitsPool::key(id.idx, id.id)];
    }

    Building& ObjectPool::GetBuilding(const ObjectID& id) {
        ASSERT_MSG(id.type != ObjectType::INVALID, "ObjectPool::GetBuilding - Using an invalid ID to access a building.");
        if(id.type != ObjectType::BUILDING) {
            ENG_LOG_ERROR("ObjectPool::GetBuilding - provided ID doesn't belong to a building (type={})", id.type);
            throw std::invalid_argument("Attempting to fetch building with ID that doesn't belong to building.");
        }
        return buildings[BuildingsPool::key(id.idx, id.id)];
    }

    FactionObject& ObjectPool::GetObject(const ObjectID& id) {
        switch(id.type) {
            case ObjectType::BUILDING:
                return GetBuilding(id);
            case ObjectType::UNIT:
                return GetUnit(id);
            default:
                ENG_LOG_ERROR("ObjectPool::GetObject - invalid ID provided ({})", id);
                throw std::invalid_argument("Invalid ID used when accessing an object.");
        }
    }

    bool ObjectPool::GetUnit(const ObjectID& id, Unit*& ref_unit, bool exclude_inactive) {
        ASSERT_MSG(id.type != ObjectType::INVALID, "ObjectPool::GetUnit - Using an invalid ID to access a unit.");
        if(id.type != ObjectType::UNIT) {
            ENG_LOG_ERROR("ObjectPool::GetUnit - provided ID doesn't belong to a unit (type={})", id.type);
            throw std::invalid_argument("Attempting to fetch unit with ID that doesn't belong to unit.");
        }

        UnitsPool::key key = UnitsPool::key(id.idx, id.id);
        if(units.exists(key)) {
            ref_unit = &units[key];
            if(!ref_unit->IsActive() && exclude_inactive) {
                ref_unit = nullptr;
                return false;
            }
            else
                return true;
        }
        return false;
    }

    bool ObjectPool::GetBuilding(const ObjectID& id, Building*& ref_building, bool exclude_inactive) {
        ASSERT_MSG(id.type != ObjectType::INVALID, "ObjectPool::GetBuilding - Using an invalid ID to access a building.");
        if(id.type != ObjectType::BUILDING) {
            ENG_LOG_ERROR("ObjectPool::GetBuilding - provided ID doesn't belong to a building (type={})", id.type);
            throw std::invalid_argument("Attempting to fetch building with ID that doesn't belong to building.");
        }

        BuildingsPool::key key = BuildingsPool::key(id.idx, id.id);
        if(buildings.exists(key)) {
            ref_building = &buildings[key];
            if(!ref_building->IsActive() && exclude_inactive) {
                ref_building = nullptr;
                return false;
            }
            else
                return true;
        }
        return false;
    }

    bool ObjectPool::GetObject(const ObjectID& id, FactionObject*& ref_object, bool exclude_inactive) {
        switch(id.type) {
            case ObjectType::BUILDING:
                return GetBuilding(id, (Building*&)ref_object, exclude_inactive);
            case ObjectType::UNIT:
                return GetUnit(id, (Unit*&)ref_object, exclude_inactive);
            default:
                ENG_LOG_ERROR("ObjectPool::GetObject - invalid ID provided ({})", id);
                throw std::invalid_argument("Invalid ID used when accessing an object.");
        }
    }

    //===== insert =====

    ObjectID ObjectPool::Add(Unit&& unit) {
        UnitsPool::key key = units.add(unit.ID(), std::move(unit));
        ObjectID oid = ObjectID(ObjectType::UNIT, key.idx, key.id);
        units[key].IntegrateIntoLevel(oid);
        return oid;
    }

    ObjectID ObjectPool::Add(Building&& building) {
        BuildingsPool::key key = buildings.add(building.ID(), std::move(building));
        ObjectID oid = ObjectID(ObjectType::BUILDING, key.idx, key.id);
        buildings[key].IntegrateIntoLevel(oid);
        return oid;
    }
    
    ObjectID ObjectPool::Add(UtilityObject&& utilityObj) {
        UtilityObjsPool::key key = utilityObjs.add(utilityObj.ID(), std::move(utilityObj));
        ObjectID oid = ObjectID(ObjectType::UTILITY, key.idx, key.id);
        utilityObjs[key].IntegrateIntoLevel(oid);
        return oid;
    }

    void ObjectPool::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        ImGui::Begin("GameObjects");

        ImGui::Text("Units: %d, Buildings: %d, Utilities: %d", units.size(), buildings.size(), utilityObjs.size());
        ImGui::Separator();

        // float indent_val = ImGui::GetWindowSize().x * 0.05f;

        if(ImGui::CollapsingHeader("Units")) {
            ImGui::Indent();
            for(Unit& u : units)
                u.DBG_GUI();
            ImGui::Unindent();
        }
        ImGui::Separator();

        if(ImGui::CollapsingHeader("Buildings")) {
            ImGui::Indent();
            for(Building& b : buildings)
                b.DBG_GUI();
            ImGui::Unindent();
        }
        ImGui::Separator();

        if(ImGui::CollapsingHeader("Utilities")) {
            ImGui::Indent();
            for(UtilityObject& u : utilityObjs)
                u.DBG_GUI();
            ImGui::Unindent();
        }

        ImGui::End();
#endif
    }

    //=============================================================

    // glm::ivec2 GetPreferredDirection(const glm::ivec2& target, const glm::ivec2& building) {
    //     glm::ivec2 res = target - building;
    //     glm::ivec2 v = glm::abs(res);
    //     res = glm::step(0, res) * 2 - 1;
    //     res[int(v.x > v.y)] = 0;
    //     return res;
    // }

    int GetPreferredDirection(const glm::ivec2& target, const glm::ivec2& building) {
        glm::ivec2 vec = target - building;
        glm::ivec2 s = glm::sign(vec);
        glm::ivec2 v = glm::abs(vec);
        int t = int(v.x > v.y);
        int res = ((t * s.x) + ((1-t) * -s.y) + 1) + t;
        return res;
    }

}//namespace eng
