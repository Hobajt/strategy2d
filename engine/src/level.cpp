#include "engine/game/level.h"

#include "engine/game/resources.h"
#include "engine/game/camera.h"
#include "engine/game/player_controller.h"

#include "engine/core/renderer.h"
#include "engine/core/input.h"

#include "engine/utils/json.h"
#include "engine/utils/utils.h"
#include "engine/utils/randomness.h"
#include "engine/utils/dbg_gui.h"

#define CONDITIONS_UPDATE_FREQUENCY 5.f

namespace eng {

    bool Parse_Mapfile(Mapfile& map, const nlohmann::json& config);
    bool Parse_Info(LevelInfo& info, const nlohmann::json& config);
    EndCondition Parse_EndCondition(const nlohmann::json& config);
    bool Parse_FactionsFile(const LevelInfo& info, FactionsFile& factions, const nlohmann::json& config);
    bool Parse_Objects(ObjectsFile& objects, const nlohmann::json& config);
    bool Parse_Camera(const nlohmann::json& config);
    Techtree Parse_Techtree(const nlohmann::json& config);
    EndgameStats Parse_EndgameStats(const nlohmann::json& config);

    nlohmann::json Export_Mapfile(const Mapfile& map);
    nlohmann::json Export_Info(const LevelInfo& info);
    nlohmann::json Export_EndCondition(const EndCondition& condition);
    nlohmann::json Export_FactionsFile(const LevelInfo& info, const FactionsFile& factions);
    nlohmann::json Export_Objects(const ObjectsFile& objects);
    nlohmann::json Export_Camera();
    nlohmann::json Export_Techtree(const Techtree& techtree);

    void parse_GameObject(const nlohmann::json& d, GameObject::Entry& e);
    void parse_FactionObject(const nlohmann::json& d, FactionObject::Entry& e);
    void parse_Unit(const nlohmann::json& d, Unit::Entry& e);
    void parse_UnitCommand(const nlohmann::json& d, Unit::Entry& e);
    void parse_UnitAction(const nlohmann::json& d, Unit::Entry& e);
    void parse_Building(const nlohmann::json& d, Building::Entry& e);
    void parse_BuildingAction(const nlohmann::json& d, Building::Entry& e);
    void parse_Utilities(const nlohmann::json& d, UtilityObject::Entry& e);
    void parse_Utilities_LiveData(const nlohmann::json& d, UtilityObject::Entry& e);

    //===== Savefile =====

    Savefile::Savefile(const std::string& filepath) {
        using json = nlohmann::json;

        //load the config and parse it as json file
        json config = json::parse(ReadFile(filepath.c_str()));

        //parse level info
        if(!config.count("info") || !Parse_Info(info, config.at("info"))) {
            ENG_LOG_WARN("Savefile - invalid info data.");
            throw std::runtime_error("Savefile - invalid info data.");
        }

        //parse map data
        if(!config.count("map") || !Parse_Mapfile(map, config.at("map"))) {
            ENG_LOG_WARN("Savefile - invalid map data.");
            throw std::runtime_error("Savefile - invalid map data.");
        }

        //parse factions data (optional entry, but malformed structure throws)
        if(config.count("factions") && !Parse_FactionsFile(info, factions, config.at("factions"))) {
            ENG_LOG_WARN("Savefile - invalid factions data.");
            throw std::runtime_error("Savefile - invalid factions data.");
        }

        //parse object data (optional entry, but malformed structure throws)
        if(config.count("objects") && !Parse_Objects(objects, config.at("objects"))) {
            ENG_LOG_WARN("Savefile - invalid object data.");
            throw std::runtime_error("Savefile - invalid object data.");
        }

        //parse camera params (optional entry, but malformed structure throws)
        if(config.count("camera") && !Parse_Camera(config.at("camera"))) {
            ENG_LOG_WARN("Savefile - invalid camera data.");
            throw std::runtime_error("Savefile - invalid camera data.");
        }

        ENG_LOG_TRACE("[R] Savefile '{}' successfully loaded.", filepath.c_str());
    }

    void LevelInfo::DBG_GUI() {
        ImGui::Begin("End Conditions");
        ImGui::Separator();
        ImGui::Text("Lose condition");
        end_conditions[EndConditionType::LOSE].DBG_GUI();
        ImGui::Separator();
        ImGui::Text("Win condition");
        end_conditions[EndConditionType::LOSE].DBG_GUI();
        ImGui::Separator();
        ImGui::Text("Force condition");
        if(ImGui::Button("LOSE")) {
            end_conditions[EndConditionType::LOSE].Set();
        }
        ImGui::SameLine();
        if(ImGui::Button("WIN")) {
            end_conditions[EndConditionType::WIN].Set();
        }
        ImGui::End();
    }

    void Savefile::Save(const std::string& filepath) {
        using json = nlohmann::json;

        json data = {};
        data["map"] = Export_Mapfile(map);
        data["factions"] = Export_FactionsFile(info, factions);
        data["objects"] = Export_Objects(objects);
        data["info"] = Export_Info(info);
        data["camera"] = Export_Camera();

        WriteFile(filepath.c_str(), data.dump());
        
        ENG_LOG_TRACE("[R] Savefile::Save - successfully stored as '{}'.", filepath.c_str());
    }

    //===== EndCondition =====

    EndCondition::EndCondition(const std::vector<ObjectID>& objects_, bool any_) : objects(objects_), objects_any(any_), disabled(false), state(false) {
        for(size_t i = objects.size(); i >= 0; --i) {
            if(objects[i] == ObjectID()) {
                ENG_LOG_WARN("EndCondition - removing invalid object {}", objects[i]);
                objects.erase(objects.begin() + i);
            }
        }

        ENG_LOG_TRACE("EndCondition - setup to track existence of {} objects.", objects.size());

        if(objects.size() < 1) {
            ENG_LOG_WARN("EndCondition - no valid object present during initialization (condition automatically met)!");
        }
    }

    EndCondition::EndCondition(const std::vector<int>& factions_, bool any_) : factions(factions_), factions_any(any_), disabled(false), state(false) {
        ENG_LOG_TRACE("EndCondition - setup to track existence of {} factions.", factions.size());
        if(factions.size() < 1) {
            ENG_LOG_WARN("EndCondition - no valid faction present during initialization (condition automatically met)!");
        }
    }

    EndCondition::EndCondition(int factionID_) : factions(std::vector<int>{ factionID_ }), disabled(false), state(false) {
        ENG_LOG_TRACE("EndCondition - setup to track existence of faction {}.", factionID_);
    }

    void EndCondition::Update(Level& level) {
        if(state)
            return;
        
        CheckFactions(level);
        CheckObjects(level);

        if(state)
            ENG_LOG_INFO("EndCondition met.");
    }

    void EndCondition::UpdateLinkage(const idMappingType& id_mapping) {
        for(int i = objects.size()-1; i >= 0; i--) {
            if(id_mapping.count(objects[i]))
                objects[i] = id_mapping.at(objects[i]);
        }
    }

    void EndCondition::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        ImGui::Text("Objects: %d", (int)objects.size());
        ImGui::SameLine();
        ImGui::Text("Any: %d", objects_any);

        ImGui::Text("Factions: %d", (int)factions.size());
        ImGui::SameLine();
        ImGui::Text("Any: %d", factions_any);

        ImGui::Text("Met: %d", state);
        ImGui::SameLine();
        ImGui::Text("Disabled: %d", disabled);
#endif
    }

    void EndCondition::CheckObjects(Level& level) {
        int dead = 0;
        FactionObject* obj = nullptr;

        for(ObjectID& id : objects) {
            dead += !level.objects.GetObject(id, obj, false);
        }

        state |= (dead != 0) && (objects_any || dead == objects.size());
    }

    void EndCondition::CheckFactions(Level& level) {
        int dead = 0;
        for(int id : factions) {
            dead += int(id < 0 || id >= level.factions.size() || level.factions[id]->IsEliminated());
        }
        
        state |= (dead != 0) && (factions_any || dead == factions.size());
    }

    //===== Level =====

    Level::Level() : map(Map(glm::ivec2(0,0), nullptr)), objects(ObjectPool{}) {}

    Level::Level(const glm::vec2& mapSize, const TilesetRef& tileset) : map(Map(mapSize, tileset)), objects(ObjectPool{}), factions(Factions()), initialized(true) {
        ENG_LOG_INFO("Level initialization complete.");
    }

    Level::Level(Savefile& savefile) : map(std::move(savefile.map)), info(savefile.info), factions(std::move(savefile.factions), map.Size(), map), objects(*this, std::move(savefile.objects)), initialized(true) {
        //restore stats after objects were created (otherwise each unit/building will be counted again)
        factions.RestoreEndgameStats(savefile.factions);
        
        ENG_LOG_INFO("Level initialization complete.");
    }

    void Level::Update() {
        map.UntouchabilityUpdate(factions.Diplomacy().Bitmap());
        factions.Update(*this);
        objects.Update();
        objects.RunesDispatch(*this, map.RunesDispatch());

        ConditionsUpdate();
    }

    void Level::Render() {
        map.Render();
        objects.Render();
    }

    bool Level::Save(const std::string& filepath) {
        try {
            Export().Save(filepath);
        } catch(std::exception&) {
            ENG_LOG_WARN("Level::Save - Failed to save level at '{}'", filepath);
            return false;
        }
        return true;
    }

    int Level::Load(const std::string& filepath, Level& out_level) {
        if(!std::filesystem::exists(filepath)) {
            ENG_LOG_WARN("Level::Load - Invalid filepath '{}'", filepath);
            return 1;
        }
        
        try {
            Savefile sf = Savefile(filepath);
            out_level = Level(sf);
            out_level.LevelPtrUpdate();
        } catch(std::exception&) {
            ENG_LOG_WARN("Level::Load - Failed to load level from '{}'", filepath);
            return 2;
        }
        return 0;
    }

    void Level::Release() {
        objects = {};
        factions = {};
        info = {};
        map = {};
        initialized = false;
        ENG_LOG_INFO("Level data released.");
    }

    void Level::CustomGame_InitFactions(int playerRace, int opponents) {
        FactionsFile ff = {};
        ff.factions.push_back(FactionsFile::FactionEntry(FactionControllerID::NATURE,        0, "Neutral", 5, 0));
        ff.factions.push_back(FactionsFile::FactionEntry(FactionControllerID::LOCAL_PLAYER,  playerRace, "Player", 0, 1));

        if(opponents == 0) {
            opponents = info.preferred_opponents;
        }

        int factionCount = std::min((int)info.startingLocations.size(), opponents+1);
        char name_buf[256];
        for(int i = 1; i < factionCount; i++) {
            snprintf(name_buf, sizeof(name_buf), "Faction %d", i);
            int race = Random::UniformInt(1);
            ff.factions.push_back(FactionsFile::FactionEntry(FactionControllerID::RandomAIMindset(), race, std::string(name_buf), i, i+1));

            for(int j = 1; j < i+2; j++) {
                ff.diplomacy.push_back({i, j, 1});
            }
        }
        factions = Factions(std::move(ff), map.Size(), map);

        //spawn starting unit for each faction
        std::vector<glm::ivec2> starting_locations = info.startingLocations;
        std::vector<UnitDataRef> worker_data = { Resources::LoadUnit("human/peasant"), Resources::LoadUnit("orc/peon") };
        for(FactionControllerRef& faction : factions) {
            if(faction->ControllerID() != FactionControllerID::NATURE) {
                int loc_idx = Random::UniformInt(starting_locations.size()-1);
                objects.EmplaceUnit(*this, worker_data[faction->Race()], faction, starting_locations[loc_idx], false);
                starting_locations.erase(starting_locations.begin() + loc_idx);
            }
        }

        ENG_LOG_INFO("CustomGame - factions initialized.");
    }

    void Level::CustomGame_InitEndConditions() {
        int pID = factions.Player()->ID();

        std::vector<int> factions_to_eliminate = {};
        for(int i = 0; i < factions.size(); i++) {
            int fID = factions[i]->ID();
            if(fID != pID && factions.Diplomacy().AreHostile(pID, fID))
                factions_to_eliminate.push_back(i);
        }

        info.end_conditions[EndConditionType::WIN ] = EndCondition(factions_to_eliminate, false);
        info.end_conditions[EndConditionType::LOSE] = EndCondition(pID);
    }

    void Level::EndConditionsEnabled(bool enabled) {
        info.end_conditions[0].disabled = !enabled;
        info.end_conditions[1].disabled = !enabled;
    }

    Savefile Level::Export() {
        Savefile savefile;
        if(factions.IsInitialized())
            factions.Player()->UpdateOcclusions(*this);
        savefile.map = map.Export();
        savefile.factions = factions.Export();
        savefile.objects = objects.Export();
        savefile.info = info;
        return savefile;
    }

    void Level::LevelPtrUpdate() {
        objects.LevelPtrUpdate(*this);
    }

    void Level::ConditionsUpdate() {
        if(lastConditionsUpdate + CONDITIONS_UPDATE_FREQUENCY < Input::CurrentTime()) {
            lastConditionsUpdate = Input::CurrentTime();

            for(int i = 0; i < 2; i++) {
                info.end_conditions[i].Update(*this);
                if(info.end_conditions[i].IsMet()) {
                    factions.Player()->DisplayEndDialog(i);
                    //to make it tick only once
                    info.end_conditions[0].disabled = info.end_conditions[1].disabled = true;
                }
            }
        }
    }

    //==============================

    bool Parse_Mapfile(Mapfile& map, const nlohmann::json& config) {
        using json = nlohmann::json;

        map.tiles = MapTiles(eng::json::parse_ivec2(config.at("size")));
        int i = 0;
        bool warn = false;
        for(auto& entry : config.at("data")) {
            if(entry.size() == 4)
                map.tiles[i++] = TileData(int(entry.at(0)), int(entry.at(1)), int(entry.at(2)), int(entry.at(3)));
            else {
                map.tiles[i++] = TileData(int(entry.at(0)), int(entry.at(1)), int(entry.at(2)), 100);
                warn = true;
            }
        }
        map.tileset = config.at("tileset");

        if(warn) {
            ENG_LOG_WARN("Mapfile format is outdated.");
        }

        if(i != map.tiles.Count()) {
            ENG_LOG_WARN("Mapfile - size mismatch ({}/{}).", i, map.tiles.Count());
            return false;
        }

        return true;
    }

    bool Parse_Info(LevelInfo& info, const nlohmann::json& config) {
        if(config.count("starting_locations")) {
            for(auto& entry : config.at("starting_locations")) {
                info.startingLocations.push_back(json::parse_ivec2(entry));
            }
        }
        else {
            ENG_LOG_WARN("Loaded level has no starting locations defined, generating default ones.");
            info.startingLocations.push_back(glm::ivec2(0,0));
            info.startingLocations.push_back(glm::ivec2(1,0));
        }

        info.preferred_opponents = config.count("preferred_opponents")  ? int(config.at("preferred_opponents")) : 1;
        info.campaignIdx         = config.count("campaign_idx")         ? int(config.at("campaign_idx")) : -1;
        info.custom_game         = config.count("custom_game")          ? bool(config.at("custom_game")) : true;
        info.race                = config.count("race")                 ? int(config.at("race")) : 0;

        if(config.count("conditions"))
            info.end_conditions = EndConditions{ Parse_EndCondition(config.at("conditions")[0]), Parse_EndCondition(config.at("conditions")[1]) };
        else
            info.end_conditions = EndConditions{ EndCondition(), EndCondition() };
        
        return true;
    }

    EndCondition Parse_EndCondition(const nlohmann::json& config) {
        EndCondition c = {};

        int i = 0;
        c.disabled      = config.at(i++);
        c.state         = config.at(i++);
        c.disabled      = config.at(i++);
        c.factions_any  = config.at(i++);
        c.objects_any   = config.at(i++);

        c.factions = {};
        auto& fac = config.at(i);
        if(fac.size() > 0) {
            for(auto& id : fac) {
                c.factions.push_back(id);
            }
        }

        c.objects = {};
        auto& obj = config.at(i);
        if(obj.size() > 0) {
            for(auto& id : obj) {
                c.objects.push_back(ObjectID(json::parse_ivec3(id)));
            }
        }

        return c;
    }

    bool Parse_FactionsFile(const LevelInfo& info, FactionsFile& factions, const nlohmann::json& config) {
        for(auto& entry : config.at("factions")) {
            FactionsFile::FactionEntry e = {};

            int i = 0;
            auto& v = entry.at(0);
            e.id            = v.at(i++);
            e.controllerID  = v.at(i++);
            e.colorIdx      = v.at(i++);
            e.race          = v.at(i++);

            e.name = entry.at(1);

            for(auto& v : entry.at(2)) {
                e.occlusionData.push_back(v);
            }

            e.techtree = Parse_Techtree(entry.at(3));
            e.eliminated = (entry.size() > 4) ? bool(entry.at(4)) : false;

            e.stats = (entry.size() > 5) ? Parse_EndgameStats(entry.at(5)) : EndgameStats{};

            factions.factions.push_back(e);
        }

        if(info.custom_game) {
            //default diplomacy -> everyone hostile to everyone (excluding neutral factions)
            for(int i = 0; i < factions.factions.size(); i++) {
                if(factions.factions[i].controllerID == FactionControllerID::NATURE)
                    continue;
                for(int j = i+1; j < factions.factions.size(); j++) {
                    if(factions.factions[j].controllerID == FactionControllerID::NATURE)
                        continue;
                    
                    factions.diplomacy.push_back(glm::ivec3(i,j,1));
                }
            }
        }
        else {
            for(auto& relation : config.at("diplomacy")) {
                factions.diplomacy.push_back(json::parse_ivec3(relation));
            }
        }

        return true;
    }

    

    bool Parse_Objects(ObjectsFile& objects, const nlohmann::json& config) {
        try {
            for(auto& entry : config.at("units")) {
                Unit::Entry e = {};
                parse_GameObject(entry.at(0), e);
                parse_FactionObject(entry.at(1), e);
                parse_Unit(entry.at(2), e);
                parse_UnitCommand(entry.at(3), e);
                parse_UnitAction(entry.at(4), e);
                objects.units.push_back(e);
            }

            for(auto& entry : config.at("buildings")) {
                Building::Entry e = {};
                parse_GameObject(entry.at(0), e);
                parse_FactionObject(entry.at(1), e);
                parse_Building(entry.at(2), e);
                parse_BuildingAction(entry.at(3), e);
                objects.buildings.push_back(e);
            }

            for(auto& entry : config.at("utilities")) {
                UtilityObject::Entry e = {};
                parse_GameObject(entry.at(0), e);
                parse_Utilities(entry.at(1), e);
                parse_Utilities_LiveData(entry.at(2), e);
                objects.utilities.push_back(e);
            }

            for(auto& e : config.at("entrance").at(0)) {
                objects.entrance.entries.push_back(EntranceController::Entry{ ObjectID(e.at(0), e.at(1), e.at(2)), ObjectID(e.at(3), e.at(4), e.at(5)), e.at(6), e.at(7) });
            }

            for(auto& e : config.at("entrance").at(1)) {
                objects.entrance.workEntries.push_back(EntranceController::WorkEntry{ ObjectID(e.at(0), e.at(1), e.at(2)), ObjectID(e.at(3), e.at(4), e.at(5)), glm::ivec2(e.at(6), e.at(7)), e.at(8), e.at(9) });
            }

            for(auto& e : config.at("entrance").at(2)) {
                objects.entrance.gms.push_back({ ObjectID(e.at(0), e.at(1), e.at(2)), e.at(3) });
            }
        } catch(nlohmann::json::exception&) {
            ENG_LOG_WARN("Failed to parse savefile objects.");
            return false;
        }

        return true;
    }

    bool Parse_Camera(const nlohmann::json& config) {
        Camera& cam = Camera::Get();

        cam.Zoom(config.at("zoom"));
        cam.Position(json::parse_ivec2(config.at("position")));

        return true;
    }

    Techtree Parse_Techtree(const nlohmann::json& config) {
        Techtree t = {};
        for(int i = 0; i < 2; i++) {
            std::array<uint8_t, ResearchType::COUNT>& data = t.ResearchData(bool(i));
            auto& entry = config.at(i);
            for(int j = 0; j < ResearchType::COUNT; j++) {
                data[j] = entry.at(j);
            }
        }

        std::array<uint8_t, ResearchType::COUNT>& research_limits = t.ResearchLimits();
        if(config.at(2).size() != 0) {
            auto& entry = config.at(2);
            for(int j = 0; j < ResearchType::COUNT; j++) {
                research_limits[j] = entry.at(j);
            }
        }

        std::array<bool, BuildingType::COUNT>& building_limits = t.BuildingLimits();
        if(config.at(3).size() != 0) {
            auto& entry = config.at(3);
            for(int j = 0; j < BuildingType::COUNT; j++) {
                building_limits[j] = bool(int(entry.at(j)));
            }
        }

        std::array<bool, UnitType::COUNT>& unit_limits = t.UnitLimits();
        if(config.at(4).size() != 0) {
            auto& entry = config.at(4);
            for(int j = 0; j < UnitType::COUNT; j++) {
                unit_limits[j] = bool(int(entry.at(j)));
            }
        }
        
        t.RecalculateBoth();
        return t;
    }

    EndgameStats Parse_EndgameStats(const nlohmann::json& config) {
        EndgameStats stats = {};

        int i = 0;
        stats.total_units           = config.at(i++);
        stats.total_buildings       = config.at(i++);
        stats.units_killed          = config.at(i++);
        stats.buildings_razed       = config.at(i++);
        stats.total_resources[0]    = config.at(i++);
        stats.total_resources[1]    = config.at(i++);
        stats.total_resources[2]    = config.at(i++);

        return stats;
    }

    nlohmann::json Export_Mapfile(const Mapfile& map) {
        using json = nlohmann::json;

        json out = {};

        out["size"] = { map.tiles.Size().x, map.tiles.Size().y };
        out["tileset"] = map.tileset;
        json& data = out["data"] = {};

        int count = map.tiles.Count();
        for(int i = 0; i < count; i++) {
            const TileData& td = map.tiles[i];
            data.push_back({ td.tileType, td.variation, td.cornerType, td.health });
        }

        return out;
    }
    
    nlohmann::json Export_Info(const LevelInfo& info) {
        using json = nlohmann::json;
        json out = {};

        json& loc = out["starting_locations"] = {};
        for(const glm::ivec2& pos : info.startingLocations) {
            loc.push_back({ pos.x, pos.y });
        }

        out["custom_game"]          = info.custom_game;
        out["preferred_opponents"]  = info.preferred_opponents;
        if(info.campaignIdx >= 0)
            out["campaign_idx"] = info.campaignIdx;
        out["race"] = info.race;
        
        out["conditions"] = { Export_EndCondition(info.end_conditions[0]), Export_EndCondition(info.end_conditions[1]) };

        return out;
    }

    nlohmann::json Export_EndCondition(const EndCondition& condition) {
        using json = nlohmann::json;
        json out = {};

        out.push_back(condition.disabled);
        out.push_back(condition.state);
        out.push_back(condition.disabled);
        out.push_back(condition.factions_any);
        out.push_back(condition.objects_any);

        out.push_back(condition.factions);

        json obj = {};
        for(const ObjectID& id : condition.objects)
            obj.push_back({ id.type, id.idx, id.id });
        out.push_back(obj);

        return out;
    }

    nlohmann::json Export_FactionsFile(const LevelInfo& info, const FactionsFile& factions) {
        using json = nlohmann::json;
        json out = {};

        json& f = out["factions"] = {};
        for(const FactionsFile::FactionEntry& entry : factions.factions) {
            json e = {};

            e.push_back({ entry.id, entry.controllerID, entry.colorIdx, entry.race });
            e.push_back(entry.name);
            e.push_back(entry.occlusionData);
            e.push_back(Export_Techtree(entry.techtree));
            e.push_back(entry.eliminated);

            e.push_back({ entry.stats.total_units, entry.stats.total_buildings, entry.stats.units_killed, entry.stats.buildings_razed, entry.stats.total_resources[0], entry.stats.total_resources[1], entry.stats.total_resources[2] });

            f.push_back(e);
        }

        if(!info.custom_game) {
            json& diplomacy = out["diplomacy"] = {};
            for(const glm::ivec3& r : factions.diplomacy) {
                diplomacy.push_back({ r.x, r.y, r.z });
            }
        }
        return out;
    }

    nlohmann::json Export_Objects(const ObjectsFile& objects) {
        using json = nlohmann::json;
        json out = {};

        json& units = out["units"] = {};
        for(const auto& entry : objects.units) {
            json e = {};

            //GameObject
            e.push_back({ entry.id.x, entry.id.y, entry.id.z, entry.num_id.x, entry.num_id.y, entry.num_id.z, entry.position.x, entry.position.y, entry.anim_orientation, entry.anim_action, entry.anim_frame, entry.killed });
            //FactionObject
            e.push_back({ entry.health, entry.factionIdx, entry.colorIdx, entry.variationIdx, entry.active, entry.finalized, entry.faction_informed });
            //Unit
            e.push_back({ entry.move_offset.x, entry.move_offset.y, entry.carry_state, entry.mana, entry.anim_ended });
            //Unit::Command
            e.push_back({ entry.command.type, entry.command.target_pos.x, entry.command.target_pos.y, entry.command.target_id.type, entry.command.target_id.idx, entry.command.target_id.id, entry.command.flag, entry.command.v2.x, entry.command.v2.y });
            //Unit::Action
            e.push_back({ entry.action.type, entry.action.data.i, entry.action.data.j, entry.action.data.k, entry.action.data.b, entry.action.data.c, entry.action.data.t, entry.action.data.count });
            
            units.push_back(e);
        }

        json& buildings = out["buildings"] = {};
        for(const auto& entry : objects.buildings) {
            json e = {};

            //GameObject
            e.push_back({ entry.id.x, entry.id.y, entry.id.z, entry.num_id.x, entry.num_id.y, entry.num_id.z, entry.position.x, entry.position.y, entry.anim_orientation, entry.anim_action, entry.anim_frame, entry.killed });
            //FactionObject
            e.push_back({ entry.health, entry.factionIdx, entry.colorIdx, entry.variationIdx, entry.active, entry.finalized, entry.faction_informed });
            //Building
            e.push_back({ entry.constructed, entry.amount_left, entry.real_actionIdx });
            //Building::Action
            e.push_back({ entry.action.type, entry.action.data.t1, entry.action.data.t2, entry.action.data.t3, entry.action.data.i, entry.action.data.flag, entry.action.data.target_id.type, entry.action.data.target_id.idx, entry.action.data.target_id.id });

            buildings.push_back(e);
        }

        json& utilities = out["utilities"] = {};
        for(const auto& entry : objects.utilities) {
            json e = {};

            //GameObject
            e.push_back({ entry.id.x, entry.id.y, entry.id.z, entry.num_id.x, entry.num_id.y, entry.num_id.z, entry.position.x, entry.position.y, entry.anim_orientation, entry.anim_action, entry.anim_frame, entry.killed });
            //UtilityObject
            e.push_back({ entry.real_position.x, entry.real_position.y, entry.real_size.x, entry.real_size.y });
            //UtilityObject::LiveData
            const auto& ld = entry.live_data;
            e.push_back({ 
                ld.source_pos.x, ld.source_pos.y, ld.target_pos.x, ld.target_pos.y, 
                ld.targetID.type, ld.targetID.idx, ld.targetID.id, 
                ld.sourceID.type, ld.sourceID.idx, ld.sourceID.id, 
                ld.f1, ld.f2, ld.f3, ld.i1, ld.i2, ld.i3, ld.i4, ld.i5, ld.i6, ld.i7,
                ld.ids[0].type, ld.ids[0].idx, ld.ids[0].id,
                ld.ids[1].type, ld.ids[1].idx, ld.ids[1].id,
                ld.ids[2].type, ld.ids[2].idx, ld.ids[2].id,
                ld.ids[3].type, ld.ids[3].idx, ld.ids[3].id,
                ld.ids[4].type, ld.ids[4].idx, ld.ids[4].id,
            });

            utilities.push_back(e);
        }

        json& entrance = out["entrance"] = {};

        json entries = {};
        for(const auto& entry : objects.entrance.entries) {
            entries.push_back({ entry.entered.type, entry.entered.idx, entry.entered.id, entry.enteree.type, entry.enteree.idx, entry.enteree.id, entry.construction, entry.carry_state });
        }
        entrance.push_back(entries);

        entries = {};
        for(const auto& entry : objects.entrance.workEntries) {
            entries.push_back({ entry.entered.type, entry.entered.idx, entry.entered.id, entry.enteree.type, entry.enteree.idx, entry.enteree.id, entry.cmd_target.x, entry.cmd_target.y, entry.cmd_type, entry.start_time });
        }
        entrance.push_back(entries);

        entries = {};
        for(const auto& entry : objects.entrance.gms) {
            entries.push_back({ entry.first.type, entry.first.idx, entry.first.id, entry.second });
        }
        entrance.push_back(entries);

        return out;
    }

    nlohmann::json Export_Camera() {
        nlohmann::json out = {};

        Camera& cam = Camera::Get();
        out["zoom"] = cam.Zoom();
        out["position"] = { cam.Position().x, cam.Position().y };

        return out;
    }

    nlohmann::json Export_Techtree(const Techtree& techtree) {
        nlohmann::json out = {};

        out.push_back(techtree.ResearchData(false));
        out.push_back(techtree.ResearchData(true));

        if(techtree.HasResearchLimits())
            out.push_back(techtree.ResearchLimits());
        else
            out.push_back(std::vector<int>{});
        
        if(techtree.HasBuildingLimits()) {
            std::vector<int> vals = {};
            for (bool b : techtree.BuildingLimits()) {
                vals.push_back((int)b);
            }
            out.push_back(vals);
        } else
            out.push_back(std::vector<int>{});
        
        if(techtree.HasUnitLimits()) {
            std::vector<int> vals = {};
            for (bool b : techtree.UnitLimits()) {
                vals.push_back((int)b);
            }
            out.push_back(vals);
        } else
            out.push_back(std::vector<int>{});

        return out;
    }

    //============================================

    void parse_GameObject(const nlohmann::json& d, GameObject::Entry& e) {
        int i = 0;
        e.id[0]              = d.at(i++);
        e.id[1]              = d.at(i++);
        e.id[2]              = d.at(i++);
        e.num_id[0]          = d.at(i++);
        e.num_id[1]          = d.at(i++);
        e.num_id[2]          = d.at(i++);
        e.position[0]        = d.at(i++);
        e.position[1]        = d.at(i++);
        e.anim_orientation   = d.at(i++);
        e.anim_action        = d.at(i++);
        e.anim_frame         = d.at(i++);
        e.killed             = d.at(i++);
    }

    void parse_FactionObject(const nlohmann::json& d, FactionObject::Entry& e) {
        int i = 0;
        e.health             = d.at(i++);
        e.factionIdx         = d.at(i++);
        e.colorIdx           = d.at(i++);
        e.variationIdx       = d.at(i++);
        e.active             = d.at(i++);
        e.finalized          = d.at(i++);
        e.faction_informed   = d.at(i++);
    }

    void parse_Unit(const nlohmann::json& d, Unit::Entry& e) {
        int i = 0;
        e.move_offset[0] = d.at(i++);
        e.move_offset[1] = d.at(i++);
        e.carry_state    = d.at(i++);
        e.mana           = d.at(i++);
        e.anim_ended     = d.at(i++);
    }

    void parse_UnitCommand(const nlohmann::json& d, Unit::Entry& e) {
        int i = 0;
        e.command.type               = d.at(i++);
        e.command.target_pos[0]      = d.at(i++);
        e.command.target_pos[1]      = d.at(i++);
        e.command.target_id.type     = d.at(i++);
        e.command.target_id.idx      = d.at(i++);
        e.command.target_id.id       = d.at(i++);
        e.command.flag               = d.at(i++);
        e.command.v2.x               = d.at(i++);
        e.command.v2.y               = d.at(i++);
    }

    void parse_UnitAction(const nlohmann::json& d, Unit::Entry& e) {
        int i = 0;
        e.action.type            = d.at(i++);
        e.action.data.i          = d.at(i++);
        e.action.data.j          = d.at(i++);
        e.action.data.k          = d.at(i++);
        e.action.data.b          = d.at(i++);
        e.action.data.c          = d.at(i++);
        e.action.data.t          = d.at(i++);
        e.action.data.count      = d.at(i++);
    }

    void parse_Building(const nlohmann::json& d, Building::Entry& e) {
        int i = 0;
        e.constructed    = d.at(i++);
        e.amount_left    = d.at(i++);
        e.real_actionIdx = d.at(i++);
    }

    void parse_BuildingAction(const nlohmann::json& d, Building::Entry& e) {
        int i = 0;
        e.action.type            = d.at(i++);
        e.action.data.t1         = d.at(i++);
        e.action.data.t2         = d.at(i++);
        e.action.data.t3         = d.at(i++);
        e.action.data.i          = d.at(i++);
        e.action.data.flag       = d.at(i++);
    }

    void parse_Utilities(const nlohmann::json& d, UtilityObject::Entry& e) {
        int i = 0;
        e.real_position[0]  = d.at(i++);
        e.real_position[1]  = d.at(i++);
        e.real_size[0]      = d.at(i++);
        e.real_size[1]      = d.at(i++);
    }

    void parse_Utilities_LiveData(const nlohmann::json& d, UtilityObject::Entry& e) {
        auto& ld = e.live_data;

        int i = 0;
        ld.source_pos[0] = d.at(i++);
        ld.source_pos[1] = d.at(i++);
        ld.target_pos[0] = d.at(i++);
        ld.target_pos[1] = d.at(i++);
        ld.targetID.type = d.at(i++);
        ld.targetID.idx  = d.at(i++);
        ld.targetID.id   = d.at(i++);
        ld.sourceID.type = d.at(i++);
        ld.sourceID.idx  = d.at(i++);
        ld.sourceID.id   = d.at(i++);

        ld.f1 = d.at(i++);
        ld.f2 = d.at(i++);
        ld.f3 = d.at(i++);
        ld.i1 = d.at(i++);
        ld.i2 = d.at(i++);
        ld.i3 = d.at(i++);
        ld.i4 = d.at(i++);
        ld.i5 = d.at(i++);
        ld.i6 = d.at(i++);
        ld.i7 = d.at(i++);

        for(int j = 0; j < 5; j++) {
            ld.ids[j].type = d.at(i++);
            ld.ids[j].idx  = d.at(i++);
            ld.ids[j].id   = d.at(i++);
        }
    }

}//namespace eng
