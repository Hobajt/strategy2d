#pragma once

#include <vector>

#include "engine/utils/mathdefs.h"
#include "engine/core/sprite.h"
#include "engine/game/gameobject.h"
#include "engine/game/faction.h"

namespace eng {

    //===== MapTiles =====

    class MapTiles {
        struct Tile {
            int id;
            int frameIdx;
            Sprite* sprite;
        };
    public:
        MapTiles(const glm::ivec2& size = glm::ivec2(10), const SpritesheetRef& tileset);
        ~MapTiles();

        glm::ivec2 Size() const { return size; }
        int Width() const { return size.x; }
        int Height() const { return size.y; }
        int Area() const { return size.x * size.y; }

        int& operator[](int i);
        const int& operator[](int i) const;

        int& operator()(int y, int x);
        const int& operator()(int y, int x) const;

        void Render();
        void RenderRange();

        void ChangeTileset(const SpritesheetRef& tileset_);
    private:
        Tile* tiles;
        glm::ivec2 size;
        SpritesheetRef tileset;
        std::vector<Sprite> tileSprites;
    };

    //===== Map =====

    class Map {
    public:
        Map(const glm::vec2& mapSize, const SpritesheetRef& tileset);

        void ChangeTileset(const SpritesheetRef& tileset);

        void Render();
        void RenderRange();
    private:
        MapTiles tiles;
        std::vector<GameObject> doodads;
    };

    struct Savefile {};

    //===== Level =====

    class Level {
    public:
        Level();
        Level(const glm::vec2& mapSize, const SpritesheetRef& tileset);
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
