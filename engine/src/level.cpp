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
            throw std::exception("Savefile - invalid map data.");
        }

        //parse factions data
        if(config.count("factions") && !Parse_FactionsFile(factions, config.at("factions"))) {
            ENG_LOG_WARN("Savefile - invalid factions data.");
            throw std::exception("Savefile - invalid factions data.");
        }
        
        
        if(!config.count("info") || !Parse_Info(info, config.at("info"))) {
            ENG_LOG_WARN("Savefile - invalid info data.");
            throw std::exception("Savefile - invalid info data.");
        }

        ENG_LOG_TRACE("[R] Savefile '{}' successfully loaded.", filepath.c_str());
    }

    void Savefile::Save(const std::string& filepath) {
        using json = nlohmann::json;

        json data = {};
        data["map"] = Export_Mapfile(map);
        data["factions"] = Export_FactionsFile(factions);

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
        if(!std::filesystem::exists(filepath))
            return 1;
        
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

        int factionCount = std::min(info.maxPlayers, opponents);
        char name_buf[256];
        for(int i = 1; i < factionCount; i++) {
            snprintf(name_buf, sizeof(name_buf), "Faction %d", i);
            int race = Random::UniformInt(1);
            ff.factions.push_back(FactionsFile::FactionEntry(FactionControllerID::RandomAIMindset(), race, std::string(name_buf), i));

            for(int j = 1; j < i; j++) {
                ff.diplomacy.push_back({i, j, 1});
            }
        }
        factions = Factions(std::move(ff), map.Size());
        map.UploadOcclusionMask(factions.Player()->Occlusion(), factions.Player()->ID());
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
        info.maxPlayers = config.count("player_count") ? config.at("player_count") : 2;
        return true;
    }

    bool Parse_FactionsFile(FactionsFile& factions, const nlohmann::json& config) {
        //TODO:
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
        return {};
    }

    nlohmann::json Export_FactionsFile(const FactionsFile& factions) {
        //TODO:
        return {};
    }

}//namespace eng
