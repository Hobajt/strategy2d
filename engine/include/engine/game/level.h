#pragma once

#include <vector>

#include "engine/utils/mathdefs.h"

#include "engine/game/gameobject.h"
#include "engine/game/faction.h"
#include "engine/game/map.h"
#include "engine/game/object_pool.h"

namespace eng {

    //===== LevelInfo =====

    //Container for various level metadata.
    struct LevelInfo {
        int campaignIdx = -1;
        bool custom_game = true;
        int preferred_opponents = 1;
        std::vector<glm::ivec2> startingLocations;
    };

    //===== Savefile =====

    struct Savefile {
        Mapfile map;
        LevelInfo info;
        FactionsFile factions;
        ObjectsFile objects;
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

        void CustomGame_InitFactions(int playerRace, int opponentCount);
    private:
        Savefile Export();
        void LevelPtrUpdate();
    public:
        //dont change the order !!!
        Map map;
        LevelInfo info;
        Factions factions;
        ObjectPool objects;

        bool initialized = false;
    };

}//namespace eng
