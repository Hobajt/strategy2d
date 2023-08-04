#include "engine/game/object_pool.h"

#include "engine/utils/dbg_gui.h"

namespace eng {

    //===== ObjectPool =====

    void ObjectPool::Update() {
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

    bool ObjectPool::GetUnit(const ObjectID& id, Unit*& ref_unit) {
        ASSERT_MSG(id.type != ObjectType::INVALID, "ObjectPool::GetUnit - Using an invalid ID to access a unit.");
        if(id.type != ObjectType::UNIT) {
            ENG_LOG_ERROR("ObjectPool::GetUnit - provided ID doesn't belong to a unit (type={})", id.type);
            throw std::invalid_argument("Attempting to fetch unit with ID that doesn't belong to unit.");
        }

        UnitsPool::key key = UnitsPool::key(id.idx, id.id);
        if(units.exists(key)) {
            ref_unit = &units[key];
            return true;
        }
        return false;
    }

    bool ObjectPool::GetBuilding(const ObjectID& id, Building*& ref_building) {
        ASSERT_MSG(id.type != ObjectType::INVALID, "ObjectPool::GetBuilding - Using an invalid ID to access a building.");
        if(id.type != ObjectType::BUILDING) {
            ENG_LOG_ERROR("ObjectPool::GetBuilding - provided ID doesn't belong to a building (type={})", id.type);
            throw std::invalid_argument("Attempting to fetch building with ID that doesn't belong to building.");
        }

        BuildingsPool::key key = BuildingsPool::key(id.idx, id.id);
        if(buildings.exists(key)) {
            ref_building = &buildings[key];
            return true;
        }
        return false;
    }

    bool ObjectPool::GetObject(const ObjectID& id, FactionObject*& ref_object) {
        switch(id.type) {
            case ObjectType::BUILDING:
                return GetBuilding(id, (Building*&)ref_object);
            case ObjectType::UNIT:
                return GetUnit(id, (Unit*&)ref_object);
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

}//namespace eng
