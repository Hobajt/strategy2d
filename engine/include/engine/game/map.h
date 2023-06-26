#pragma once

#include <memory>
#include <array>

#include "engine/utils/mathdefs.h"
#include "engine/core/sprite.h"

namespace eng {

    //TODO: maybe setup tiletypes so that specific bits signal specific functionality (traversable, buildable, etc.)
    //can still use an index when loading descriptions from a file

    namespace TileType { 
        /*NOTE - THINGS THAT DEPEND ON ORDER/VALUES HERE:
            - Map::CornersValidationCheck::resolutionTypes
            - Map::CornersValidationCheck::transition_types
            - Map::GetTransitionTileType::transition_corners
            - Map::ResolveCornerType()
            - IsWallTile() (local fn, map.cpp)
        */

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

    //Live representation of a tile.
    struct TileData {
        int tileType = 0;           //tile type identifier (enum value)
        int variation = 0;          //for visual modifications (if given tile type has any)
        int cornerType = 0;         //type identification for top-left corner of the tile

        glm::ivec2 idx = glm::ivec2(0);     //index, pointing to location of specific tile visuals in tilemap sprite
    };

    //To track map changes. Separate struct, in case TileData gets modified.
    struct TileRecord {
        int tileType = 0;
        int variation = 0;
        int cornerType = 0;
        glm::ivec2 pos = glm::ivec2(0);
    public:
        TileRecord() = default;
        TileRecord(const TileData& td, int cornerType_, int y, int x) : tileType(td.tileType), variation(td.variation), cornerType(cornerType_), pos(glm::ivec2(x, y)) {}
    };

    //Helper struct, used during the map modifications.
    struct TileMod {
        int y;
        int x;
        bool writeCorner;       //when this corner is painted from different tile (tile = 4 corners, top-left is the primary one tho)

        bool painted = false;

        int depth = 0;
        int prevCornerType = -1;
        int dominantCornerType = -1;
    public:
        TileMod() = default;
        static TileMod FromFloodFill(int y, int x, bool writeCorner) { return TileMod(y, x, writeCorner); }
        static TileMod FromResolution(int y, int x, int prevCornerType, int resType, int depth) { return TileMod(y, x, prevCornerType, resType, depth); }
    private:
        TileMod(int y_, int x_, bool writeCorner_) : y(y_), x(x_), writeCorner(writeCorner_) {}
        TileMod(int y_, int x_, int prevCornerType_, int resType, int depth_)
            : y(y_), x(x_), depth(depth_), prevCornerType(prevCornerType_), dominantCornerType(resType), writeCorner(false) {}
    };

    //Groups together all tiles of given TileType, sorted by their border type.
    //Tiles are described as tileset indices.
    class TileGraphics {
    public:
        glm::ivec2 GetTileIdx(int borderType, int variation) const;
        void Extend(int x, int y, int borderType);
    public:
        int type = -1;
        bool cornerless = false;
        glm::ivec2 defaultIdx = glm::ivec2(0);
    private:
        //maps borderType to tile indices
        std::unordered_map<int, std::vector<glm::ivec2>> mapping;
    };

    //===== Tileset =====

    class Tileset {
    public:
        struct Data {
            std::string name;
            Sprite tilemap;
            std::array<TileGraphics, TileType::COUNT> tiles;
        };
    public:
        Tileset() = default;
        Tileset(const std::string& config_filepath, int flags = 0);

        const Sprite& Tilemap() const { return data.tilemap; }
        std::string Name() const { return data.name; }

        //Updates tile.idx fields to use proper image for each tile (based on tile type, corner type & variations).
        //Assumes that provided array is of size ((size.x+1) * (size.y+1)).
        void UpdateTileIndices(TileData* tiles, const glm::ivec2& size) const;

        glm::ivec2 GetTileIdx(int tileType, int borderType, int variation);
    private:
        int c2i(int y, int x, const glm::ivec2& size) const { return y*(size.x+1)+x; }
    private:
        Tileset::Data data;
    };
    using TilesetRef = std::shared_ptr<Tileset>;

    //===== PaintBitmap =====

    namespace PaintFlags { enum { MARKED_FOR_PAINT = 1, VISITED = 2, FINALIZED = 4, DEBUG = 8 }; }

    class PaintBitmap {
    public:
        PaintBitmap() = default;
        PaintBitmap(const glm::ivec2& size);
        ~PaintBitmap();

        PaintBitmap(const PaintBitmap&) = delete;

        PaintBitmap(PaintBitmap&&) noexcept;
        PaintBitmap& operator=(PaintBitmap&&) noexcept;

        int& operator[](int i);
        const int& operator[](int i) const;

        int& operator()(int y, int x);
        const int& operator()(int y, int x) const;

        glm::ivec2 Size() const { return size; }

        void Fill(int value);
        void Clear() { Fill(0); }
    private:
        int* paint = nullptr;
        glm::ivec2 size = glm::ivec2(0,0);
    };

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

        int& operator()(int y, int x);
        const int& operator()(int y, int x) const;

        void Render();
        void RenderRange(int x, int y, int w, int h);

        void ChangeTileset(const TilesetRef& tilesetNew);

        TilesetRef GetTileset() const { return tileset; }

        //==== Methods for editor ====

        std::string GetTilesetName() const { return (tileset != nullptr) ? tileset->Name() : "none"; }

        //Revert changes provided in the vector. Only works if undone in the reverse order of applying modifications.
        //  rewrite_history = modifies provided vector, so that it can be used to re-do modifications using this method.
        void UndoChanges(std::vector<TileRecord>& history, bool rewrite_history = true);

        void ModifyTiles(PaintBitmap& paint, int tileType, bool randomVariation, int variationValue, std::vector<TileRecord>* history = nullptr);
    private:
        //Flood-fill; Adds all tiles that are marked for painting or are neighboring with a marked tile (starting at the specified location (y,x)).
        //Corner type of marked tiles is overriden with the new type (old type is stored in it's TileMod entry in the modified vec).
        //After the call, modified vector should contain all directly marked tiles as well as their direct neighbors. These tiles should also have VISITED flag set in the paint bitmap.
        void DetectAffectedTiles(PaintBitmap& paint, int y, int x, std::vector<TileMod>& modified, int cornerType);
        //Checks if given tile has valid combination of corner types.
        //If it doesn't, corner types are updated and neighboring tiles are added for processing too.
        //Returns new tile type, that should be assigned to this tile.
        int ResolveCornerConflict(TileMod m, std::vector<TileMod>& modified, int dominantCornerType, int depth);
        //Checks if given corner type combination makes a valid transition tile.
        //Returns transition tile index if it does or -1 if it doesn't.
        int GetTransitionTileType(int cornerType1, int cornerType2) const;
        //Checks provided corner combination, returns true if the combination is legal.
        //  dominantCornerType       - corner type, that should be preserved in case of a conflict.
        //  out_resolutionCornerType - corner type, that should replace the invalid ones in case of a conflict (forms valid transition with the dominant type).
        //  out_newTileType          - what tile type to assign to this tile.
        bool CornersValidationCheck(const glm::ivec4& corners, int dominantCornerType, int& out_resolutionCornerType, int& out_newTileType) const;
        //Defines what corner type to write when painting given tileType.
        int ResolveCornerType(int paintedTileType) const;

        TileData& at(int y, int x);
        const TileData& at(int y, int x) const;

        int c2i(int y, int x) const { return y * (size.x+1) + x; }

        void Move(Map&&) noexcept;
        void Release() noexcept;

        void DBG_Print() const;
    private:
        TilesetRef tileset;
        glm::ivec2 size;

        TileData* tiles;
    };

}//namespace eng
