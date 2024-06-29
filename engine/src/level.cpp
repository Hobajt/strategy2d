#include "engine/game/level.h"

#include "engine/game/resources.h"
#include "engine/game/camera.h"
#include "engine/game/player_controller.h"

#include "engine/core/renderer.h"

#include "engine/utils/json.h"
#include "engine/utils/utils.h"

#include "engine/utils/randomness.h"

namespace eng {

    bool Parse_Mapfile(Mapfile& map, const nlohmann::json& config);
    bool Parse_Info(LevelInfo& info, const nlohmann::json& config);
    bool Parse_FactionsFile(const LevelInfo& info, FactionsFile& factions, const nlohmann::json& config);
    bool Parse_Objects(ObjectsFile& objects, const nlohmann::json& config);
    Techtree Parse_Techtree(const nlohmann::json& config);

    nlohmann::json Export_Mapfile(const Mapfile& map);
    nlohmann::json Export_Info(const LevelInfo& info);
    nlohmann::json Export_FactionsFile(const LevelInfo& info, const FactionsFile& factions);
    nlohmann::json Export_Objects(const ObjectsFile& objects);
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

        if(config.count("objects") && !Parse_Objects(objects, config.at("objects"))) {
            ENG_LOG_WARN("Savefile - invalid object data.");
            throw std::runtime_error("Savefile - invalid object data.");
        }

        ENG_LOG_TRACE("[R] Savefile '{}' successfully loaded.", filepath.c_str());
    }

    void Savefile::Save(const std::string& filepath) {
        using json = nlohmann::json;

        json data = {};
        data["map"] = Export_Mapfile(map);
        data["factions"] = Export_FactionsFile(info, factions);
        data["objects"] = Export_Objects(objects);
        data["info"] = Export_Info(info);

        WriteFile(filepath.c_str(), data.dump());
        
        ENG_LOG_TRACE("[R] Savefile::Save - successfully stored as '{}'.", filepath.c_str());
    }

    //===== Level =====

    Level::Level() : map(Map(glm::ivec2(0,0), nullptr)), objects(ObjectPool{}) {}

    Level::Level(const glm::vec2& mapSize, const TilesetRef& tileset) : map(Map(mapSize, tileset)), objects(ObjectPool{}), factions(Factions()), initialized(true) {}

    Level::Level(Savefile& savefile) : map(std::move(savefile.map)), info(savefile.info), factions(std::move(savefile.factions), map.Size(), map), objects(*this, std::move(savefile.objects)), initialized(true) {}

    void Level::Update() {
        map.UntouchabilityUpdate(factions.Diplomacy().Bitmap());
        factions.Update(*this);
        objects.Update();
        objects.RunesDispatch(*this, map.RunesDispatch());
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

    Savefile Level::Export() {
        Savefile savefile;
        savefile.map = map.Export();
        savefile.factions = factions.Export();
        savefile.objects = objects.Export();
        savefile.info = info;
        return savefile;
    }

    void Level::LevelPtrUpdate() {
        objects.LevelPtrUpdate(*this);
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
        
        return true;
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
