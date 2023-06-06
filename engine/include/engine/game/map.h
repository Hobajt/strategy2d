#pragma once

#include <memory>
#include <array>

#include "engine/utils/mathdefs.h"
#include "engine/core/sprite.h"

namespace eng {

    namespace TileType { 
        enum {
            //traversable ground
            GROUND1 = 0,
            GROUND2,
            MUD1,
            MUD2,
            WALL_BROKEN,
            ROCK_BROKEN,
            TREES_FELLED,

            //water
            WATER,

            //obstacle tiles
            ROCK,
            WALL_HU,
            WALL_HU_DAMAGED, 
            WALL_OC,
            WALL_OC_DAMAGED, 
            TREES, 

            COUNT
        }; 
    }//namespace TileType

    struct TileData {
        int type = 0;                       //tile type identifier (enum value)
        int variation = 0;                  //for visual modifications (if given tile type has any)
        glm::ivec2 idx = glm::ivec2(0);     //index of visuals in tilemap sprite
    };

    //===== Tileset =====

    class Tileset {
    public:
        struct TileDescription {
            // int type;
            glm::ivec2 idx;
            // int cornerType;
            // std::vector<> nb_top;
            // std::vector<> nb_bot;
            // std::vector<> nb_left;
            // std::vector<> nb_right;

        };
        struct Data {
            Sprite tilemap;
            std::array<TileDescription, TileType::COUNT> tileDesc;
            std::string name;
        };
    public:
        Tileset() = default;
        Tileset(const std::string& config_filepath, int flags = 0);

        const Sprite& Tilemap() const { return data.tilemap; }
        std::string Name() const { return data.name; }

        //Updates tile.idx fields to use proper image for each tile (based on tile type & variations).
        void UpdateTileIndices(TileData* tiles, const glm::ivec2& size);
        void UpdateTileIndices(TileData* tiles, const glm::ivec2& size, int y, int x);

        glm::ivec2 GetIdxFor(int tileType);
    private:
        Tileset::Data data;
    };
    using TilesetRef = std::shared_ptr<Tileset>;

    //===== Map =====

    class Map {
    public:
        Map(const glm::ivec2& size = glm::ivec2(10), const TilesetRef& tileset = nullptr);
        ~Map();
        
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

        //==== Methods for editor ====

        std::string GetTilesetName() const { return (tileset != nullptr) ? tileset->Name() : "none"; }

        Map Clone() const;

        void ModifyTile(int y, int x, int type, int variation, bool update_indices = true);
        void ModifyTiles(bool* bitmap, int type, bool randomVariations, int variation);

        void UpdateTileIndices();
        void UpdateTileIndices(int y, int x);

        const TileData& GetTile(int y, int x) const { return tiles[y*size.x + x]; }
    private:
        int coord2idx(int y, int x) const { return y * size.x + x; }

        Map(const Map&) noexcept;

        void Move(Map&&) noexcept;
        void Release() noexcept;
    private:
        TilesetRef tileset;
        TileData* tiles;
        glm::ivec2 size;
    };

}//namespace eng
