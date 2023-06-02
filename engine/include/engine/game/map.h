#pragma once

#include <memory>
#include <array>

#include "engine/utils/mathdefs.h"
#include "engine/core/sprite.h"

namespace eng {

    namespace TileType { 
        enum { 
            GROUND = 0, MUD, WATER, 
            ROCK, ROCK_BROKEN, 
            WALL_HU, WALL_HU_DAMAGED, 
            WALL_OC, WALL_OC_DAMAGED, WALL_BROKEN, 
            TREES, TREES_FELLED,
            COUNT
        }; 
    }//namespace TileType

    struct TileData {
        int type;
        int variation;
        glm::ivec2 idx;
    };

    //===== Tileset =====

    

    class Tileset {
    public:
        struct TileDescription {
            // int type;
            // int cornerType;
            // std::vector<> nb_top;
            // std::vector<> nb_bot;
            // std::vector<> nb_left;
            // std::vector<> nb_right;

            glm::ivec2 idx;
        };
        struct Data {
            Sprite tilemap;
            std::array<TileDescription, TileType::COUNT> tileDesc;
        };
    public:
        Tileset() = default;
        Tileset(const std::string& config_filepath, int flags = 0);

        const Sprite& Tilemap() const { return data.tilemap; }

        //Updates tile.idx fields to use proper image for each tile (based on tile type & variations).
        void UpdateTileIndices(TileData* tiles, int count);
    private:
        Tileset::Data data;
    };
    using TilesetRef = std::shared_ptr<Tileset>;

    //===== Map =====

    class Map {
    public:
        Map(const glm::ivec2& size = glm::ivec2(10), const TilesetRef& tileset = nullptr);
        ~Map();

        Map(const Map&) = delete;
        Map& operator=(const Map&) = delete;

        Map(Map&&) noexcept;
        Map& operator=(Map&&) noexcept;

        glm::ivec2 Size() const { return size; }
        int Width() const { return size.x; }
        int Height() const { return size.y; }
        int Area() const { return size.x * size.y; }

        int& operator[](int i);
        const int& operator[](int i) const;

        int& operator()(int y, int x);
        const int& operator()(int y, int x) const;

        void Render();
        void RenderRange(int x, int y, int w, int h);

        void ChangeTileset(const TilesetRef& tilesetNew);

    private:
        int coord2idx(int y, int x) const { return y * size.x + x; }

        void Move(Map&&) noexcept;
        void Release() noexcept;
    private:
        glm::ivec2 size;
        TileData* tiles;
        TilesetRef tileset;
    };

}//namespace eng
