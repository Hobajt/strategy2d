#include "engine/game/object_pool.h"

#include "engine/utils/dbg_gui.h"

#include "engine/core/input.h"
#include "engine/game/config.h"
#include "engine/game/map.h"

#include "engine/game/resources.h"
#include "engine/game/level.h"

#include <sstream>

#define WORKER_ENTRY_DURATION 1.f

#define MAX_UNLOAD_RANGE 3

namespace eng {

    int GetPreferredDirection(const glm::ivec2& target, const glm::ivec2& building);

    //===== EntranceController =====

    EntranceController::EntranceController(EntranceController::ExportData&& data, const idMappingType& id_mapping)
        : entries(std::move(data.entries)), workEntries(std::move(data.workEntries)), gms(std::move(data.gms)) {

        for(Entry& entry : entries) {
            entry.entered = id_mapping.at(entry.entered);
            entry.enteree = id_mapping.at(entry.enteree);
        }

        for(WorkEntry& entry : workEntries) {
            entry.entered = id_mapping.at(entry.entered);
            entry.enteree = id_mapping.at(entry.enteree);
        }

        for(auto& entry : gms) {
            entry.first = id_mapping.at(entry.first);
        }
    }

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

    EntranceController::ExportData EntranceController::Export() const {
        EntranceController::ExportData data = {};
        data.workEntries = workEntries;
        data.entries = entries;
        data.gms = gms;
        return data;
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

        //goldmine visuals update
        if(building->IsGatherable(NavigationBit::GROUND)) {
            Goldmine_Increment(buildingID, building);
        }

        worker->WithdrawObject();
        workEntries.push_back({ buildingID, workerID, cmd_target, cmd_type, (float)Input::CurrentTime() });
        ENG_LOG_TRACE("EntranceController::WorkEntrance - Worker '{}' entered '{}'.", *worker, *building);
        return true;
    }

    bool EntranceController::IssueEntrance_Transport(ObjectPool& objects, const ObjectID& transportID, const ObjectID& unitID) {
        Unit* unit = nullptr;
        Unit* transport = nullptr;
        if(!objects.GetUnit(transportID, transport) || !objects.GetUnit(unitID, unit)) {
            ENG_LOG_WARN("EntranceController::TransportEntrance - object not found.");
            return false;
        }

        if(!transport->IsTransport()) {
            ENG_LOG_WARN("EntranceController::TransportEntrance - provided object doesn't have transport capabilities.");
            return false;
        }

        if(!transport->IsActive() || !unit->IsActive()) {
            ENG_LOG_WARN("EntranceController::TransportEntrance - object is not active.");
            return false;
        }

        if(transport->Transport_IsFull()) {
            return false;
        }

        unit->WithdrawObject();
        transport->Transport_UnitAdded();
        entries.push_back({ transportID, unitID, false, unit->CarryStatus() });
        ENG_LOG_TRACE("EntranceController::TransportEntrance - Unit '{}' entered '{}'.", *unit, *transport);
        return true;
    }

    bool EntranceController::IssueExit_Transport(ObjectPool& objects, const ObjectID& transportID, const ObjectID& unitID) {
        Unit* transport;
        if(transportID.type != ObjectType::UNIT || !objects.GetUnit(transportID, transport)) {
            ENG_LOG_WARN("EntranceController::TransportExit - transport not found.");
            return false;
        }

        Unit* unit;
        glm::ivec2 respawn_position;
        for(int i = (int)entries.size()-1; i >= 0; i--) {
            if(entries[i].entered == transportID && entries[i].enteree == unitID) {
                if(objects.GetUnit(entries[i].enteree, unit, false)) {
                    int carry_state = entries[i].carry_state;
                    entries.erase(entries.begin() + i);
                    if(transport->NearbySpawnCoords(unit->NavigationType(), 3, respawn_position, MAX_UNLOAD_RANGE) > 0) {
                        ASSERT_MSG(!unit->IsActive(), "EntranceController::TransportExit - unit cannot be active whilst inside a transport.");
                        unit->ReinsertObject(respawn_position);
                        if(unit->IsWorker())
                            unit->ChangeCarryStatus(carry_state);
                        transport->Transport_UnitRemoved();
                        return true;
                    }
                    else {
                        ENG_LOG_WARN("EntranceController::TransportExit - no place to spawn the unit.");
                        return false;
                    }
                }
                else {
                    ENG_LOG_WARN("EntranceController::TransportExit - entry located, but unit object not found ({}).", *transport);
                    entries.erase(entries.begin() + i);
                    return false;
                }
            }
        }

        ENG_LOG_WARN("EntranceController::TransportExit - entry {} not found ({}).", unitID, *transport);
        return false;
    }

    std::pair<int,int> EntranceController::IssueExit_Transport(ObjectPool& objects, const ObjectID& transportID) {
        Unit* transport;
        if(transportID.type != ObjectType::UNIT || !objects.GetUnit(transportID, transport)) {
            ENG_LOG_WARN("EntranceController::TransportExit - transport not found.");
            return { -1, -1 };
        }

        int unloaded = 0;
        int total = 0;
        bool cant_unload = false;

        Unit* unit;
        glm::ivec2 respawn_position;
        for(int i = (int)entries.size()-1; i >= 0; i--) {
            if(entries[i].entered == transportID) {
                total++;
                if(objects.GetUnit(entries[i].enteree, unit, false)) {
                    int carry_state = entries[i].carry_state;
                    if(!cant_unload && transport->NearbySpawnCoords(unit->NavigationType(), 3, respawn_position, MAX_UNLOAD_RANGE) > 0) {
                        ASSERT_MSG(!unit->IsActive(), "EntranceController::TransportExit - unit cannot be active whilst inside a transport.");
                        unit->ReinsertObject(respawn_position);
                        if(unit->IsWorker())
                            unit->ChangeCarryStatus(carry_state);
                        transport->Transport_UnitRemoved();
                        unloaded++;
                        entries.erase(entries.begin() + i);
                    }
                    else {
                        ENG_LOG_WARN("EntranceController::TransportExit - no place to spawn the unit.");
                        cant_unload = true;
                    }
                }
                else {
                    ENG_LOG_WARN("EntranceController::TransportExit - entry located, but unit object not found ({}).", *transport);
                    entries.erase(entries.begin() + i);
                }
            }
        }
        return { unloaded, total };
    }

    bool EntranceController::IssueEntrance_Construction(ObjectPool& objects, const ObjectID& buildingID, const ObjectID& workerID) {
        Unit* worker = nullptr;
        Building* building = nullptr;
        if(!objects.GetUnit(workerID, worker, false) || !objects.GetBuilding(buildingID, building)) {
            ENG_LOG_WARN("EntranceController::ConstructionEntrance - object not found.");
            return false;
        }

        if(!worker->IsWorker()) {
            ENG_LOG_WARN("EntranceController::ConstructionEntrance - provided unit is not a worker.");
            return false;
        }

        //worker has to be inactive prior to entering (otherwise, it would obstruct the spawning of the construction site)
        if(worker->IsActive() || !building->IsActive()) {
            ENG_LOG_WARN("EntranceController::ConstructionEntrance - object activity error.");
            return false;
        }
        
        entries.push_back({ buildingID, workerID, true, worker->CarryStatus() });
        ENG_LOG_TRACE("EntranceController::ConstructionEntrance - Worker '{}' entered '{}'.", *worker, *building);
        return true;
    }

    bool EntranceController::IssueExit_Construction(ObjectPool& objects, const ObjectID& buildingID, bool auto_commands) {
        Building* building;
        if(buildingID.type != ObjectType::BUILDING || !objects.GetBuilding(buildingID, building)) {
            ENG_LOG_WARN("EntranceController::ConstructionExit - building not found.");
            return false;
        }
        int building_type = building->NumID()[1];

        Unit* worker;
        int num_workers = 0;
        glm::ivec2 respawn_position;
        for(int i = (int)entries.size()-1; i >= 0; i--) {
            if(entries[i].construction && entries[i].entered == buildingID) {
                if(objects.GetUnit(entries[i].enteree, worker, false)) {
                    num_workers++;
                    building->NearbySpawnCoords(worker->NavigationType(), 3, respawn_position);
                    ASSERT_MSG(!worker->IsActive(), "EntranceController::ConstructionExit - construction worker cannot be active.");
                    worker->ReinsertObject(respawn_position);
                    worker->ChangeCarryStatus(entries[i].carry_state);

                    //auto issue new command when workers finish construction of specific buildings
                    if(auto_commands) {
                        switch(building_type) {
                            case BuildingType::LUMBER_MILL:
                                worker->IssueCommand(Command::Harvest(respawn_position));
                                break;
                            case BuildingType::OIL_PLATFORM:
                                IssueEntrance_Work(objects, buildingID, entries[i].enteree, glm::ivec2(buildingID.idx, buildingID.id), worker->CarryStatus());
                                break;
                        }
                    }
                }
                else {
                    ENG_LOG_WARN("EntranceController::ConstructionExit - entry located, but worker object not found ({}).", *building);
                }
                entries.erase(entries.begin() + i);
            }
        }

        if(num_workers != 1) {
            ENG_LOG_WARN("EntranceController::ConstructionExit - suspicious number of workers ({}) left the building '{}'.", num_workers, *building);
        }
        return (num_workers > 0);
    }

    void EntranceController::GetUnitsInside(const ObjectID& container_unit, std::vector<ObjectID>& out_ids) {
        out_ids.clear();
        for(auto& entry : entries) {
            if(entry.entered == container_unit)
                out_ids.push_back(entry.enteree);
        }
    }

    int EntranceController::KillObjectsInside(ObjectPool& objects, const ObjectID& id) {
        int count = 0;
        Unit* unit;

        for(int i = (int)entries.size()-1; i >= 0; i--) {
            if(entries[i].entered == id) {
                if(objects.GetUnit(entries[i].enteree, unit, false)) {
                    unit->Kill(true);
                }
                else {
                    ENG_LOG_WARN("EntranceController::KillObjectsInside - invalid unit ID '{}'", entries[i].enteree);
                }
                entries.erase(entries.begin() + i);
                count++;
            }
        }

        for(int i = (int)workEntries.size()-1; i >= 0; i--) {
            if(workEntries[i].entered == id) {
                if(objects.GetUnit(workEntries[i].enteree, unit, false)) {
                    unit->Kill(true);
                }
                else {
                    ENG_LOG_WARN("EntranceController::KillObjectsInside - invalid unit ID '{}'", workEntries[i].enteree);
                }
                workEntries.erase(workEntries.begin() + i);
                count++;
            }
        }

        return count;
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
        bool set_carry_state = false;
        bool no_prev_command = (entry.cmd_target.x == -1) && (entry.cmd_target.y == -1);
        switch(entry.cmd_type) {
            case WorkerCarryState::GOLD:        //worker entered with gold -> return to Gather command
            case WorkerCarryState::OIL:
            {
                if(!no_prev_command) {
                    ObjectID goldmineID = ObjectID(ObjectType::BUILDING, entry.cmd_target.x, entry.cmd_target.y);
                    Building* goldmine = nullptr;
                    cmd = Command::Gather(goldmineID);
                    if(objects.GetBuilding(goldmineID, goldmine)) {
                        preferred_direction = GetPreferredDirection(goldmine->Position(), building->Position());
                    }
                }
                break;
            }
            case WorkerCarryState::WOOD:        //worker entered with wood -> return to Harvest command
                if(!no_prev_command) {
                    cmd = Command::Harvest(entry.cmd_target);
                    preferred_direction = GetPreferredDirection(entry.cmd_target, building->Position());
                }
                break;
            default:                            //worker entered empty handed -> is in resource deposit -> ReturnGoods command
                cmd = Command::ReturnGoods(entry.cmd_target);
                set_carry_state = true;
                if(building->IsGatherable(NavigationBit::GROUND)) {
                    Goldmine_Decrement(entry.entered, building);
                }
                break;
        }

        building->NearbySpawnCoords(worker->NavigationType(), preferred_direction, respawn_position);
        worker->ReinsertObject(respawn_position);
        if(set_carry_state) {
            worker->ChangeCarryStatus((building->NavigationType() != NavigationBit::WATER) ? WorkerCarryState::GOLD : WorkerCarryState::OIL);
            building->DecrementResource();
        }
        if(!no_prev_command) {
            worker->IssueCommand(cmd);
        }

        ENG_LOG_TRACE("EntranceController::WorkExit - Worker '{}' left '{}'.", *worker, *building);
        return true;
    }

    void EntranceController::Goldmine_Increment(const ObjectID& gmID, Building* gm) {
        bool found = false;
        size_t i = 0;
        for(i = 0; i < gms.size(); i++) {
            if(gms[i].first == gmID) {
                gms[i].second++;
                found = true;
                break;
            }
        }

        if(!found) {
            gms.push_back({ gmID, 1 });
            i = gms.size()-1;
        }
        
        gm->SetVariationIdx(1);
    }
    
    void EntranceController::Goldmine_Decrement(const ObjectID& gmID, Building* gm) {
        bool found = false;
        size_t i = 0;
        for(i = 0; i < gms.size(); i++) {
            if(gms[i].first == gmID) {
                gms[i].second = std::max(gms[i].second-1, 0);
                found = true;
                break;
            }
        }

        if(!found) {
            gms.push_back({ gmID, 0 });
            i = gms.size()-1;
        }
        
        if(gms[i].second <= 0) {
            gm->SetVariationIdx(0);
        }
    }

    //===== ObjectPool =====

    ObjectPool::ObjectPool(Level& level, ObjectsFile&& file) {
        auto id_mapping = PopulatePools(level, file);
        id_mapping.insert({ ObjectID(), ObjectID() });
        entranceController = EntranceController(std::move(file.entrance), id_mapping);
        UpdateLinkage(level, id_mapping);
        InitObjectCounter(level);
    }

    void ObjectPool::Release() {
        //have to cleanup EntranceController first (buildings KillObjectsInside() from their destructor)
        entranceController = {};

        units.clear();
        buildings.clear();
        utilityObjs.clear();
    }

    idMappingType ObjectPool::PopulatePools(Level& level, const ObjectsFile& file) {
        std::vector<std::pair<ObjectID, ObjectID>> id_mapping = {};
        int failure_counter = 0;
        int max_id = 0;

        for(const Unit::Entry& entry : file.units) {
            try {
                UnitsPool::key key = units.add(entry.id.z, Unit(level, entry));
                ObjectID new_id = ObjectID(ObjectType::UNIT, key.idx, key.id);
                if(max_id < entry.id.z) max_id = entry.id.z;
                units[key].IntegrateIntoLevel(new_id);
                id_mapping.push_back({ entry.id, new_id });
            } catch(std::exception&) {
                ENG_LOG_WARN("Failed to load unit (ID={}, num_id={}, faction={})", entry.id, entry.num_id, entry.factionIdx);
                failure_counter++;
            }
        }

        for(const Building::Entry& entry : file.buildings) {
            try {
                BuildingsPool::key key = buildings.add(entry.id.z, Building(level, entry));
                ObjectID new_id = ObjectID(ObjectType::BUILDING, key.idx, key.id);
                if(max_id < entry.id.z) max_id = entry.id.z;
                buildings[key].IntegrateIntoLevel(new_id);
                id_mapping.push_back({ entry.id, new_id });
            } catch(std::exception&) {
                ENG_LOG_WARN("Failed to load building (ID={}, num_id={}, faction={})", entry.id, entry.num_id, entry.factionIdx);
                failure_counter++;
            }
        }

        for(const UtilityObject::Entry& entry : file.utilities) {
            try {
                UtilityObjsPool::key key = utilityObjs.add(entry.id.z, UtilityObject(level, entry));
                ObjectID new_id = ObjectID(ObjectType::UTILITY, key.idx, key.id);
                if(max_id < entry.id.z) max_id = entry.id.z;
                utilityObjs[key].IntegrateIntoLevel(new_id);
                id_mapping.push_back({ entry.id, new_id });
            } catch(std::exception&) {
                ENG_LOG_WARN("Failed to load utility object (ID={}, num_id={})", entry.id, entry.num_id);
                failure_counter++;
            }
        }

        if(failure_counter != 0) {
            ENG_LOG_WARN("ObjectPool::PopulatePools - total failed loads = {}", failure_counter);
        }

        GameObject::SetID(max_id+1);

        ENG_LOG_TRACE("ObjectPool::PopulatePools: U={}, B={}, UT={}", units.size(), buildings.size(), utilityObjs.size());
        return idMappingType(id_mapping.begin(), id_mapping.end());
    }

    void ObjectPool::UpdateLinkage(Level& level, const idMappingType& id_mapping) {
        for(Unit& u : units) {
            u.RepairIDs(id_mapping);
        }
        
        for(Building& b : buildings) {
            b.RepairIDs(id_mapping);
        }
        
        for(UtilityObject& u : utilityObjs) {
            u.RepairIDs(id_mapping);
        }

        level.info.end_conditions[0].UpdateLinkage(id_mapping);
        level.info.end_conditions[1].UpdateLinkage(id_mapping);
    }

    void ObjectPool::Update() {
        entranceController.Update(*this);

        for(int i = 0; i < factionObjectCount.size(); i++)
            factionObjectCount[i] = 0;
        for(int i = 0; i < factionKillCount.size(); i++)
            factionKillCount[i] = glm::ivec2(0);

        for(Unit& u : units) {
            factionObjectCount[u.FactionIdx()]++;
            if(u.Update()) {
                if(u.Kill()) {
                    markedForRemoval.push_back(u.OID().idx);
                }
            }
        }
        markedForRemoval.push_back((ObjectID::dtype)-1);
        
        for(Building& b : buildings) {
            factionObjectCount[b.FactionIdx()]++;
            if(b.Update()) {
                if(b.Kill()) {
                    markedForRemoval.push_back(b.OID().idx);
                }
            }
        }
        markedForRemoval.push_back((ObjectID::dtype)-1);
        
        for(UtilityObject& u : utilityObjs) {
            if(u.Update()) {
                if(u.Kill()) {
                    markedForRemoval.push_back(u.OID().idx);
                }
            }
        }

        int poolIdx = 0;
        for(ObjectID::dtype idx : markedForRemoval) {
            if(idx == (ObjectID::dtype)-1) {
                poolIdx++;
                continue;
            }

            switch(poolIdx) {
                case 0:
                    IncrementKillCounter(idx, false);
                    units.remove(idx);
                    break;
                case 1:
                    IncrementKillCounter(idx, true);
                    buildings.remove(idx);
                    break;
                case 2:
                    utilityObjs.remove(idx);
                    break;
            }
        }
        markedForRemoval.clear();

        for(UtilityObject& obj : to_spawn)
            Add(std::move(obj));
        to_spawn.clear();
    }

    void ObjectPool::IncrementKillCounter(ObjectID::dtype idx, bool isBuilding) {
        int fIdx = -1;
        if(isBuilding && buildings.taken(idx))
            fIdx = buildings[idx].LastHitFaction();
        else if(!isBuilding && units.taken(idx))
            fIdx = units[idx].LastHitFaction();
        
        if(fIdx < 0)
            return;

        for(int i = factionKillCount.size() - 1; i <= fIdx; i++) {
            factionKillCount.push_back(glm::ivec2(0));
        }

        factionKillCount[fIdx][int(isBuilding)]++;
    }

    void ObjectPool::Render() {
        for(Unit& u : units)
            u.Render();
        
        for(Building& b : buildings)
            b.Render();
        
        for(UtilityObject& u : utilityObjs)
            u.Render();
    }

    ObjectsFile ObjectPool::Export() const {
        ObjectsFile file = {};

        for(const Unit& u : units)
            file.units.push_back(u.Export());
        
        for(const Building& b : buildings)
            file.buildings.push_back(b.Export());
        
        for(const UtilityObject& u : utilityObjs)
            file.utilities.push_back(u.Export());
        
        file.entrance = entranceController.Export();

        return file;
    }

    void ObjectPool::RefreshColors() {
        for(Unit& u : units) {
            u.ResetColor();
        }
        for(Building& b : buildings) {
            b.ResetColor();
        }
    }

    void ObjectPool::RemoveFactionlessObjects(Level& level) {
        int units_removed = 0;
        for(Unit& u : units) {
            if(!level.factions.IsValidFaction(u.Faction())) {
                u.Kill(true);
                units_removed++;
            }
        }

        int buildings_removed = 0;
        for(Building& b : buildings) {
            if(!level.factions.IsValidFaction(b.Faction())) {
                b.Kill(true);
                buildings_removed++;
            }
        }

        ENG_LOG_TRACE("ObjectPool::RemoveFactionlessObjects - removed {} factionless objects ({} units, {} buildings).", units_removed+buildings_removed, units_removed, buildings_removed);
    }

    void ObjectPool::RemoveInvalidlyPlacedObjects(Level& level) {
        int units_removed = 0;
        for(Unit& u : units) {
            if(!level.map.HasValidPlacement_Unit(u.Position(), u.NavigationType())) {
                u.Kill(true);
                units_removed++;
            }
        }

        int buildings_removed = 0;
        for(Building& b : buildings) {
            if(!level.map.HasValidPlacement_Building(b.Position(), b.Data()->size, b.NavigationType(), b.IsCoastal(), b.Data()->IsOil())) {
                b.Kill(true);
                buildings_removed++;
            }
        }

        ENG_LOG_TRACE("ObjectPool::RemoveInvalidlyPlacedObjects - removed {} factionless objects ({} units, {} buildings).", units_removed+buildings_removed, units_removed, buildings_removed);
    }

    std::vector<ClickSelectionEntry> ObjectPool::ClickSelectionDataFromIDs(const std::vector<ObjectID>& ids) {
        std::vector<ClickSelectionEntry> entries;
        entries.reserve(ids.size());

        for(const ObjectID& id : ids) {
            FactionObject& obj = GetObject(id);

            ClickSelectionEntry entry = {};
            entry.id = id;
            entry.obj = &obj;
            entry.factionID = obj.FactionIdx();
            entry.aabb = obj.AABB();
            entry.num_id = obj.NumID();

            entries.push_back(entry);
        }

        return entries;
    }

    void ObjectPool::LevelPtrUpdate(Level& lvl) {
        for(Unit& u : units)
            u.LevelPtrUpdate(lvl);
        for(Building& u : buildings)
            u.LevelPtrUpdate(lvl);
        for(UtilityObject& u : utilityObjs)
            u.LevelPtrUpdate(lvl);
    }

    void ObjectPool::UnitUpgrade(int factionID, int old_type, int new_type, bool isOrcUnit) {
        int count = 0;

        for(Unit& u : units) {
            if(u.UnitUpgrade(factionID, old_type, new_type, isOrcUnit)) {
                count++;
            }
        }

        ENG_LOG_TRACE("ObjectPool::UnitUpgade - upgraded {} -> {} (faction{}, unit count={})", old_type, new_type, factionID, count);
    }

    void ObjectPool::RandomizeIdleRotations() {
        for(Unit& u : units) {
            u.RandomizeIdleRotation();
        }
    }

    void ObjectPool::InitObjectCounter(Level& level) {
        if(factionObjectCount.size() != 0) {
            ENG_LOG_WARN("ObjectPool::InitObjectCounter - counter already initialized.");
            return;
        }

        for(int i = 0; i < level.factions.size(); i++)
            factionObjectCount.push_back(0);

        
        for(Unit& u : units) {
            factionObjectCount[u.FactionIdx()]++;
        }
        
        for(Building& b : buildings) {
            factionObjectCount[b.FactionIdx()]++;
        }

        for(int i = 0; i < level.factions.size(); i++)
            factionKillCount.push_back(glm::ivec2(0));
    }

    void ObjectPool::InitObjectCounter_Editor() {
        factionObjectCount.clear();
        for(int i = 0; i < 12; i++)
            factionObjectCount.push_back(0);

        factionKillCount.clear();
        for(int i = 0; i < 12; i++)
            factionKillCount.push_back(glm::ivec2(0));
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

    int ObjectPool::GetCorpsesAround(const glm::ivec2& position, int radius, std::vector<UtilityObject*>& out_corpses) {
        for(UtilityObject& u : utilityObjs) {
            if(u.UData()->utility_id == UtilityObjectType::CORPSE && u.LD().i7 != 0 && chessboard_distance(position, u.Position()) <= radius) {
                out_corpses.push_back(&u);
            }
        }
        return out_corpses.size();
    }

    void ObjectPool::RunesDispatch(Level& level, std::vector<std::pair<glm::ivec2, ObjectID>>& exploded_runes) {
        UtilityObjectDataRef runes = Resources::LoadSpell(SpellID::RUNES);
        int basicDamage = runes->i3;
        int pierceDamage = runes->i4;

        UtilityObjectDataRef followup = nullptr;
        if(runes->spawn_followup) {
            followup =  Resources::LoadUtilityObj(runes->followup_name);
        }

        Unit* unit = nullptr;
        for(auto& [pos, id] : exploded_runes) {
            if(!GetUnit(id, unit)) {
                ENG_LOG_WARN("ObjectPool::RunesDispatch - something went wrong, unit not found.");
                continue;
            }

            ENG_LOG_FINE("Rune exploded at {} - victim={}", pos, *unit);
            unit->ApplyDamageFlat(basicDamage + pierceDamage);

            if(followup != nullptr) {
                QueueUtilityObject(level, followup, glm::vec2(pos) + 0.5f, ObjectID());
            }
        }
        exploded_runes.clear();
    }

    void ObjectPool::UpdateBuildingID(const ObjectID& object_id) {
        if(object_id.type != ObjectType::BUILDING) {
            ENG_LOG_ERROR("ObjectPool::UpdateBuildingID - provided ID doesn't belong to a building (type={})", object_id.type);
            throw std::invalid_argument("Attempting to fetch building with ID that doesn't belong to building.");
        }
        int new_id = GameObject::GetNextID();
        Building& building = buildings.update_key(BuildingsPool::key(object_id.idx, object_id.id), new_id);
        building.oid.id = building.id = new_id;
        ENG_LOG_TRACE("ObjectPool::UpdateBuildingID - {} is now {}", object_id, building.oid);
    }

    void ObjectPool::UpdateUnitID(const ObjectID& object_id) {
        if(object_id.type != ObjectType::UNIT) {
            ENG_LOG_ERROR("ObjectPool::UpdateUnitID - provided ID doesn't belong to a unit (type={})", object_id.type);
            throw std::invalid_argument("Attempting to fetch unit with ID that doesn't belong to unit.");
        }
        int new_id = GameObject::GetNextID();
        Unit& unit = units.update_key(UnitsPool::key(object_id.idx, object_id.id), new_id);
        unit.oid.id = unit.id = new_id;
        ENG_LOG_TRACE("ObjectPool::UpdateUnitID - {} is now {}", object_id, unit.oid);
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

        std::stringstream factionObjectCount_str;
        std::copy(factionObjectCount.begin(), factionObjectCount.end(), std::ostream_iterator<int>(factionObjectCount_str, " "));

        ImGui::Text("Units: %d, Buildings: %d, Utilities: %d", (int)units.size(), (int)buildings.size(), (int)utilityObjs.size());
        ImGui::Text("Faction Object Counter: %s", factionObjectCount_str.str().c_str());
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
