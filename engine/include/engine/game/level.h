#pragma once

#include <vector>

#include "engine/utils/mathdefs.h"

#include "engine/game/gameobject.h"
#include "engine/game/faction.h"
#include "engine/game/map.h"

namespace eng {

    struct Savefile {};

    //===== Level =====

    class Level {
    public:
        Level();
        Level(const glm::vec2& mapSize, const TilesetRef& tileset);
        Level(const Savefile& savefile);

        void Update();
        void Render();
    private:
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
