#pragma once

#include <memory>
#include <vector>
#include <queue>
#include <array>

#include "engine/utils/mathdefs.h"
#include "engine/core/sprite.h"

#include "engine/game/object_data.h"

namespace eng {

    class Unit;

    using buildingMapCoords = std::tuple<glm::ivec2, glm::ivec2, ObjectID, int>;

    //TODO: maybe setup tiletypes so that specific bits signal specific functionality (traversable, buildable, etc.)
    //can still use an index when loading descriptions from a file

    namespace TileType { 
        /*NOTE - THINGS THAT DEPEND ON ORDER/VALUES HERE:
            - Map::CornersValidationCheck::resolutionTypes
            - Map::CornersValidationCheck::transition_types
            - Map::GetTransitionTileType::transition_corners
            - Map::DamageTile::tileMapping
            - Map::ResolveCornerType()
            - TileData::TileTraversability::tile_traversability
            - IsWallTile()
            - IsMapObject()
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

    namespace NavigationBit { enum { OBSTACLE = 0, GROUND = 1, WATER = 2, AIR = 4 }; }

    bool IsWallTile(int tileType);
    bool IsMapObject(int tileType);

    //Navigation data container.
    struct NavData {
        int taken = 0;                  //tracks if the tile is taken (bitmap - NavigationBit)
        int permanent = 0;              //defines if the object occupying it intends to stay (bitmap, false -> just moving through)
        bool building = false;          //additional check, to ensure that units can't pass trough buildings

        bool pathtile = false;      //purely for debugging
        float d = HUGE_VALF;
        bool visited = false;
        bool part_of_forrest = false;
    public:
        //Clears the temporary variables for next pathfinding.
        void Cleanup();

        void Claim(int navType, bool permanently, bool is_building);
        void Unclaim(int navType, bool is_building);
    };

    //Used to store intermediate info during the pathfinding computations.
    struct NavEntry {
        float h;
        float d;
        glm::ivec2 pos;
    public:
        NavEntry() = default;
        NavEntry(float h_, float d_, const glm::ivec2& pos_) : h(h_), d(d_), pos(pos_) {}

        friend inline bool operator<(const NavEntry& lhs, const NavEntry& rhs) { return lhs.h < rhs.h; }
        friend inline bool operator>(const NavEntry& lhs, const NavEntry& rhs) { return lhs.h > rhs.h; }
    };

    using pathfindingContainer = std::priority_queue<NavEntry, std::vector<NavEntry>, std::greater<NavEntry>>;

    //Live representation of a tile.
    struct TileData {
        int tileType = 0;           //tile type identifier (enum value)
        int variation = 0;          //for visual modifications (if given tile type has any)
        int cornerType = 0;         //type identification for top-left corner of the tile

        glm::ivec2 idx = glm::ivec2(0);     //index, pointing to location of specific tile visuals in tilemap sprite

        int health = 100;           //for trees/walls health tracking; useless in other tile types
        NavData nav;                //navigation variables

        ObjectID id = {};           //id of an object located on this tile (invalid means empty)
    public:
        TileData() = default;
        TileData(int tileType, int variation, int cornerType, int health);

        //Returns true if a unit with given navigation type can traverse this tile (& the tile isn't taken).
        bool Traversable(int unitNavType) const;
        bool TraversableOrForrest(int unitNavType) const;

        //Returns true if the tile is marked as permanently taken (object occupying it indends to stay there).
        //Also returns true if the tile is not taken, but is untraversable by definition (bcs of a tile type).
        bool PermanentlyTaken(int unitNavType) const;

        void UpdateID();

        bool IsTreeTile() const { return tileType == TileType::TREES; }
    private:
        //Defines how can this tile be traversed. Only considers tile type (no navigation data).
        int TileTraversability() const;
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

    //===== MapTiles =====

    class MapTiles {
    public:
        MapTiles() = default;
        MapTiles(const glm::ivec2& size);
        ~MapTiles();

        MapTiles(const MapTiles&) = delete;
        MapTiles& operator=(const MapTiles&) = delete;

        MapTiles Clone() const;

        MapTiles(MapTiles&&) noexcept;
        MapTiles& operator=(MapTiles&&) noexcept;

        glm::ivec2 Size() const { return size; }
        int Area() const { return size.x * size.y; }
        int Count() const { return size.x * size.y + size.x + size.y; }
        bool Valid() const { return (data != nullptr); }

        TileData& operator[](int i);
        const TileData& operator[](int i) const;

        TileData& operator()(int y, int x);
        const TileData& operator()(int y, int x) const;

        TileData& operator()(const glm::ivec2& idx);
        const TileData& operator()(const glm::ivec2& idx) const;

        void NavDataCleanup();
        int NavData_Forrest_FloodFill(const glm::ivec2& pos);
    private:
        void Move(MapTiles&& m) noexcept;
        void Release() noexcept;
    private:
        glm::ivec2 size = glm::ivec2(0);
        TileData* data = nullptr;
    };

    //===== Tileset =====

    namespace TilesetType { enum { SUMMER = 0, WINTER, WASTELAND }; }

    class Tileset {
    public:
        struct Data {
            std::string name;
            Sprite tilemap;
            std::array<TileGraphics, TileType::COUNT> tiles;
            int type;
        };
    public:
        Tileset() = default;
        Tileset(const std::string& config_filepath, int flags = 0);

        const Sprite& Tilemap() const { return data.tilemap; }
        std::string Name() const { return data.name; }

        //Updates tile.idx fields to use proper image for each tile (based on tile type, corner type & variations).
        //Assumes that provided array is of size ((size.x+1) * (size.y+1)).
        void UpdateTileIndices(MapTiles& tiles) const;
        void UpdateTileIndices(MapTiles& tiles, const glm::ivec2& coords) const;

        glm::ivec2 GetTileIdx(int tileType, int borderType, int variation);

        int GetType() const { return data.type; }
    private:
        void UpdateTileIndex(MapTiles& tiles, int y, int x, TileData* a, TileData* b, TileData* c, TileData* d) const;
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

    //===== Mapfile =====

    struct Mapfile {
        std::string tileset;
        MapTiles tiles;
    };

    //===== Map =====

    class Map {
    public:
        Map(const glm::ivec2& size = glm::ivec2(10), const TilesetRef& tileset = nullptr);
        Map(Mapfile&& mapfile);
        
        Map(const Map&) = delete;
        Map& operator=(const Map&) = delete;

        Map(Map&&) noexcept;
        Map& operator=(Map&&) noexcept;

        Mapfile Export();

        glm::ivec2 Size() const { return tiles.Size(); }
        int Width() const { return tiles.Size().x; }
        int Height() const { return tiles.Size().y; }
        int Area() const { return tiles.Area(); }

        const TileData& operator()(int y, int x) const;
        const TileData& operator()(const glm::ivec2& idx) const;

        //Apply damage to given tile's health (don't use for harvest). If health drops below zero, the tile type changes.
        void DamageTile(const glm::ivec2& idx, int damage, const ObjectID& src = ObjectID());
        //Harvest tick on given tile. Should only be used on wood tiles. Returns true when the health hits zero.
        bool HarvestTile(const glm::ivec2& idx);

        void Render();
        void RenderRange(int x, int y, int w, int h);

        bool IsWithinBounds(const glm::ivec2& pos) const;

        void ChangeTileset(const TilesetRef& tilesetNew);

        TilesetRef GetTileset() const { return tileset; }

        //Uses pathfinding algs to find the next position for given unit to travel to (in order to reach provided destination).
        glm::ivec2 Pathfinding_NextPosition(const Unit& unit, const glm::ivec2& target_pos);
        //Searches for path to a block of tiles.
        glm::ivec2 Pathfinding_NextPosition_Range(const Unit& unit, const glm::ivec2& target_min, const glm::ivec2& target_max);
        //Searches for path to nearest trees tile (any tree tile that's part of the same forrest).
        glm::ivec2 Pathfinding_NextPosition_Forrest(const Unit& unit, const glm::ivec2& target_pos);

        //Searches for path to a nearest building (buildings defined in argument). Returns false if no path is found. On success, building's ObjectID and next position for movement is returned via arguments.
        bool Pathfinding_NextPosition_NearestBuilding(const Unit& unit, const std::vector<buildingMapCoords>& buildings, ObjectID& out_id, glm::ivec2& out_nextPos);

        //Scan neighborhood for tree tiles & pick one. Prioritize tiles in the direction of preferred_pos.
        bool FindTrees(const glm::ivec2& worker_pos, const glm::ivec2& preferred_pos, glm::ivec2& out_pos, int radius);

        void AddObject(int navType, const glm::ivec2& pos, const glm::ivec2& size, const ObjectID& id, bool is_building);
        void RemoveObject(int navType, const glm::ivec2& pos, const glm::ivec2& size, bool is_building);
        void MoveUnit(int unitNavType, const glm::ivec2& pos_prev, const glm::ivec2& pos_next, bool permanently);

        ObjectID ObjectIDAt(const glm::ivec2& coords) const;

        //==== Methods for editor ====

        std::string GetTilesetName() const { return (tileset != nullptr) ? tileset->Name() : "none"; }

        //Revert changes provided in the vector. Only works if undone in the reverse order of applying modifications.
        //  rewrite_history = modifies provided vector, so that it can be used to re-do modifications using this method.
        void UndoChanges(std::vector<TileRecord>& history, bool rewrite_history = true);

        void ModifyTiles(PaintBitmap& paint, int tileType, bool randomVariation, int variationValue, std::vector<TileRecord>* history = nullptr);

        void DBG_GUI();
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

        glm::ivec2 MinDistanceNeighbor(const glm::ivec2& center);
        //Retrieves next position for movement. Call after pathfinding is done (uses filled out distance values in the map data).
        glm::ivec2 Pathfinding_RetrieveNextPos(const glm::ivec2& pos_src, const glm::ivec2& pos_dst, int navType);

        TileData& at(int y, int x);
        const TileData& at(int y, int x) const;

        int c2i(int y, int x) const { return y * (tiles.Size().x+1) + x; }

        void DBG_PrintTiles() const;
        void DBG_PrintDistances() const;
    private:
        TilesetRef tileset;
        MapTiles tiles;

        pathfindingContainer nav_list;
    };

}//namespace eng
