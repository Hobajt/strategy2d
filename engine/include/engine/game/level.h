#pragma once

#include <vector>

#include "engine/utils/mathdefs.h"

#include "engine/game/gameobject.h"
#include "engine/game/faction.h"
#include "engine/game/map.h"
#include "engine/game/object_pool.h"

namespace eng {

    //===== Savefile =====

    struct Savefile {
        Mapfile map;
    public:
        Savefile() = default;
        Savefile(const std::string& filepath);

        void Save(const std::string& filepath);
    };

    //===== Level =====

    class Level {
    public:
        Level();
        Level(const glm::vec2& mapSize, const TilesetRef& tileset);
        Level(Savefile& savefile);

        void Update();
        void Render();

        bool Save(const std::string& filepath);
        static int Load(const std::string& filepath, Level& out_level);

        glm::ivec2 MapSize() const { return map.Size(); }
    private:
        Savefile Export();
    public:
        Map map;
        ObjectPool objects;
        std::vector<FactionControllerRef> factions;
        Diplomacy diplomacy;
    };

    // class Savefile {
    //     struct GameObjectSave {
    //         int id;
    //         int type;
    //         Action action;
    //         float progress;
    //     };

    //     std::string levelName;
    //     int campaignIdx;

    //     std::vector<> resources;
    //     std::vector<GameObjectSave> objects;
        
    // };

}//namespace eng
