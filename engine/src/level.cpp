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
    bool Parse_FactionsFile(FactionsFile& factions, const nlohmann::json& config);

    nlohmann::json Export_Mapfile(const Mapfile& map);
    nlohmann::json Export_Info(const LevelInfo& info);
    nlohmann::json Export_FactionsFile(const FactionsFile& factions);

    //===== Savefile =====

    Savefile::Savefile(const std::string& filepath) {
        using json = nlohmann::json;

        //load the config and parse it as json file
        json config = json::parse(ReadFile(filepath.c_str()));

        //parse map data
        if(!config.count("map") || !Parse_Mapfile(map, config.at("map"))) {
            ENG_LOG_WARN("Savefile - invalid map data.");
            throw std::runtime_error("Savefile - invalid map data.");
        }

        //parse factions data
        if(config.count("factions") && !Parse_FactionsFile(factions, config.at("factions"))) {
            ENG_LOG_WARN("Savefile - invalid factions data.");
            throw std::runtime_error("Savefile - invalid factions data.");
        }
        
        
        if(!config.count("info") || !Parse_Info(info, config.at("info"))) {
            ENG_LOG_WARN("Savefile - invalid info data.");
            throw std::runtime_error("Savefile - invalid info data.");
        }

        ENG_LOG_TRACE("[R] Savefile '{}' successfully loaded.", filepath.c_str());
    }

    void Savefile::Save(const std::string& filepath) {
        using json = nlohmann::json;

        json data = {};
        data["map"] = Export_Mapfile(map);
        data["factions"] = Export_FactionsFile(factions);
        data["info"] = Export_Info(info);

        WriteFile(filepath.c_str(), data.dump());
        
        ENG_LOG_TRACE("[R] Savefile::Save - successfully stored as '{}'.", filepath.c_str());
    }

    //===== Level =====

    Level::Level() : map(Map(glm::ivec2(0,0), nullptr)), objects({}) {}

    Level::Level(const glm::vec2& mapSize, const TilesetRef& tileset) : map(Map(mapSize, tileset)), objects({}), factions(Factions()), initialized(true) {}

    Level::Level(Savefile& savefile) : map(std::move(savefile.map)), objects({}), factions(std::move(savefile.factions), map.Size()), info(savefile.info), initialized(true) {
        if(factions.IsInitialized())
            map.UploadOcclusionMask(factions.Player()->Occlusion(), factions.Player()->ID());
    }

    void Level::Update() {
        factions.Update(*this);
        objects.Update();
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
        } catch(std::exception&) {
            ENG_LOG_WARN("Level::Load - Failed to load level from '{}'", filepath);
            return 2;
        }
        return 0;
    }

    void Level::CustomGame_InitFactions(int playerRace, int opponents) {
        FactionsFile ff = {};
        ff.factions.push_back(FactionsFile::FactionEntry(FactionControllerID::NATURE,        0, "Neutral", 5));
        ff.factions.push_back(FactionsFile::FactionEntry(FactionControllerID::LOCAL_PLAYER,  playerRace, "Player", 0));

        if(opponents == 0) {
            opponents = info.preferred_opponents;
        }

        int factionCount = std::min((int)info.startingLocations.size(), opponents+1);
        char name_buf[256];
        for(int i = 1; i < factionCount; i++) {
            snprintf(name_buf, sizeof(name_buf), "Faction %d", i);
            int race = Random::UniformInt(1);
            ff.factions.push_back(FactionsFile::FactionEntry(FactionControllerID::RandomAIMindset(), race, std::string(name_buf), i));

            for(int j = 1; j < i+2; j++) {
                ff.diplomacy.push_back({i, j, 1});
            }
        }
        factions = Factions(std::move(ff), map.Size());
        map.UploadOcclusionMask(factions.Player()->Occlusion(), factions.Player()->ID());

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
        savefile.info = info;
        return savefile;
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
        //TODO:
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
        
        return true;
    }

    bool Parse_FactionsFile(FactionsFile& factions, const nlohmann::json& config) {
        for(auto& entry : config.at("factions")) {
            FactionsFile::FactionEntry e = {};

            glm::ivec3 v = json::parse_ivec3(entry.at(0));
            e.controllerID  = v[0];
            e.colorIdx      = v[1];
            e.race          = v[2];

            e.name = entry.at(1);

            for(auto& v : entry.at(2)) {
                e.occlusionData.push_back(v);
            }

            factions.factions.push_back(e);

            //TODO: add techtree parsing
        }
        
        for(auto& relation : config.at("diplomacy")) {
            factions.diplomacy.push_back(json::parse_ivec3(relation));
        }

        return true;
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
        //TODO:
        using json = nlohmann::json;

        json out = {};

        json& loc = out["starting_locations"] = {};
        for(const glm::ivec2& pos : info.startingLocations) {
            loc.push_back({ pos.x, pos.y });
        }

        out["preferred_opponents"] = info.preferred_opponents;
        if(info.campaignIdx >= 0) out["campaign_idx"] = info.campaignIdx;

        return out;
    }

    nlohmann::json Export_FactionsFile(const FactionsFile& factions) {
        using json = nlohmann::json;

        json out = {};

        json& f = out["factions"] = {};
        for(const FactionsFile::FactionEntry& entry : factions.factions) {
            json e = {};

            e.push_back({ entry.controllerID, entry.colorIdx, entry.race });
            e.push_back(entry.name);
            e.push_back(entry.occlusionData);
            
            //TODO: add techtree data export

            f.push_back(e);
        }

        json& diplomacy = out["diplomacy"] = {};
        for(const glm::ivec3& r : factions.diplomacy) {
            diplomacy.push_back({ r.x, r.y, r.z });
        }

        return out;
    }

}//namespace eng
