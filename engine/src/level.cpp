#include "engine/game/level.h"

#include "engine/game/resources.h"
#include "engine/game/camera.h"
#include "engine/game/player_controller.h"

#include "engine/core/renderer.h"

#include "engine/utils/json.h"
#include "engine/utils/utils.h"

namespace eng {

    bool Parse_Mapfile(Mapfile& map, const nlohmann::json& config);
    nlohmann::json Export_Mapfile(const Mapfile& map);

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

        ENG_LOG_TRACE("[R] Savefile '{}' successfully loaded.", filepath.c_str());
    }

    void Savefile::Save(const std::string& filepath) {
        using json = nlohmann::json;

        json data = {};
        data["map"] = Export_Mapfile(map);

        WriteFile(filepath.c_str(), data.dump());
        
        ENG_LOG_TRACE("[R] Savefile::Save - successfully stored as '{}'.", filepath.c_str());
    }

    //===== Level =====

    Level::Level() : map(Map(glm::ivec2(0,0), nullptr)), objects({}) {}

    Level::Level(const glm::vec2& mapSize, const TilesetRef& tileset) : map(Map(mapSize, tileset)), objects({}), factions(Factions()) {}

    Level::Level(Savefile& savefile) : map(std::move(savefile.map)), objects({}), factions(std::move(savefile.factions), map.Size()) {
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

    Savefile Level::Export() {
        Savefile savefile;
        savefile.map = map.Export();
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

}//namespace eng
