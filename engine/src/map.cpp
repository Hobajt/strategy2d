#include "engine/game/map.h"

#include "engine/game/resources.h"
#include "engine/game/camera.h"

#include "engine/utils/utils.h"
#include "engine/utils/json.h"
#include "engine/utils/dbg_gui.h"
#include "engine/utils/randomness.h"
#include "engine/game/config.h"

#include "engine/game/gameobject.h"
#include "engine/game/player_controller.h"
#include "engine/utils/generator.h"

#include "engine/game/level.h"

#include <random>
#include <limits>
#include <sstream>

static constexpr int TRANSITION_TILE_OPTIONS = 6;
static constexpr int HARVEST_WOOD_HEALTH_TICK = 10;

static constexpr int MIN_OIL_PATCH_DISTANCE = 3;

static constexpr int INVIS_DETECTION_RADIUS = 4;

namespace eng {

    Tileset::Data ParseConfig_Tileset(const std::string& config_filepath, int flags);

    //Generates tile's border type from corner tile types.
    int GetBorderType(int t_a, int a, int b, int c, int d);

    typedef float(*heuristic_fn)(const glm::ivec2& src, const glm::ivec2& dst);

    float heuristic_euclidean(const glm::ivec2& src, const glm::ivec2& dst);
    float heuristic_diagonal(const glm::ivec2& src, const glm::ivec2& dst);
    float range_heuristic(const glm::ivec2& src, const glm::ivec2& m, const glm::ivec2& M, heuristic_fn H);

    bool Pathfinding_AStar(MapTiles& tiles, pathfindingContainer& open, const glm::ivec2& pos_src, const glm::ivec2& pos_dst, int navType, heuristic_fn h);
    bool Pathfinding_AStar_Range(MapTiles& tiles, pathfindingContainer& open, const glm::ivec2& pos_src_, int range, const glm::ivec2& m, const glm::ivec2& M, glm::ivec2* result_pos, int navType, heuristic_fn h);
    bool Pathfinding_AStar_Forrest(MapTiles& tiles, pathfindingContainer& open, const glm::ivec2& pos_src, const glm::ivec2& pos_dst, int navType, heuristic_fn h);
    bool Pathfinding_Dijkstra_NearestBuilding(MapTiles& tiles, pathfindingContainer& open, const glm::ivec2& pos_src_, const std::vector<buildingMapCoords>& targets, int resourceType, int navType, glm::ivec2& out_dst_pos);

    //an element of tmp array, used during tileset parsing
    struct TileDescription {
        int tileType = -1;
        int borderType = 0;
    };

    void NavData::Cleanup() {
        d = std::numeric_limits<float>::infinity();
        visited = false;
        pathtile = false;
        part_of_forrest = false;
    }

    void NavData::Claim(int navType, bool permanently, bool is_building) {
        taken |= navType;
        permanent |= int(permanently)*navType;
        building |= is_building;
    }

    void NavData::Unclaim(int navType, bool is_building) {
        taken &= ~navType;
        permanent &= ~navType;
        building = (is_building) ? false : building;
    }

    TileData::TileData(int tileType_, int variation_, int cornerType_, int health_)
        : tileType(tileType_), cornerType(cornerType_), variation(variation_), health(health_) {
        UpdateID();
    }

    bool TileData::Traversable(int unitNavType) const {
        return bool(unitNavType & (TileTraversability() + NavigationBit::AIR) & (~nav.taken)) && (!nav.building || unitNavType == NavigationBit::AIR);
    }

    bool TileData::TraversableOrForrest(int unitNavType) const {
        return Traversable(unitNavType) || nav.part_of_forrest;
    }

    bool TileData::Traversable_Terrain(int unitNavType) const {
        return bool(unitNavType & (TileTraversability() + NavigationBit::AIR));
    }

    bool TileData::IsEmptyWaterTile() const {
        return (TileTraversability() == NavigationBit::WATER) && !(ObjectID::IsObject(info[0].id) && ObjectID::IsValid(info[0].id));
    }

    bool TileData::PermanentlyTaken(int unitNavType) const {
        return bool(unitNavType & nav.taken & nav.permanent) || bool(unitNavType & ~(TileTraversability() + NavigationBit::AIR)) || nav.building;
    }

    void TileData::UpdateID() {
        info[0].id = IsMapObject(tileType) ? ObjectID(ObjectType::MAP_OBJECT, 0, tileType) : ObjectID();
    }

    void TileData::VisionIncrement() {
        //increment vision counter & disable both unexplored and fog of war bits
        vis.visionCounter++;
        vis.occlusion |= 3 & (3*int(vis.visionCounter > 0));
        ASSERT_MSG(vis.visionCounter >= 0, "TileData::VisionIncrement - vision counter is at a negative value.")
    }

    void TileData::VisionDecrement() {
        //decrement vision counter, enable fog of war if there's no vision
        if((--vis.visionCounter) <= 0)
            vis.occlusion &= ~2;
        ASSERT_MSG(vis.visionCounter >= 0, "TileData::VisionIncrement - vision counter is at a negative value.")
    }

    void TileData::UpdateOcclusionIndices(int a, int b, int c, int d) {
        //inner parentheses = get bit corresponding to fog/occlusion; outer parentheses = move it to proper position
        vis.occ_idx = ((a & 1) << 0) | ((b & 1) << 1) | ((c & 1) << 2) | ((d & 1) << 3);
        vis.fog_idx = ((a & 2) >> 1) | ((b & 2) << 0) | ((c & 2) << 1) | ((d & 2) << 2);
    }

    bool TileData::IsVisible(bool occlusion_enabled) const {
        return !occlusion_enabled || (vis.occ_idx != 0 && (!Config::FogOfWar() || vis.fog_idx != 0));
    }

    bool TileData::IsCoastTile() const {
        return coastal;
    }

    int TileData::DespawnRune(ObjectID::dtype ID) {
        if(rune_id == ID) {
            rune_id = 0;
            return 1;
        }
        return 0;
    }

    int TileData::TileTraversability() const {
        constexpr static int tile_traversability[TileType::COUNT] = {
            NavigationBit::GROUND,
            NavigationBit::GROUND,
            NavigationBit::GROUND,
            NavigationBit::GROUND,
            NavigationBit::GROUND,
            NavigationBit::GROUND,
            NavigationBit::GROUND,
            NavigationBit::WATER,
            NavigationBit::OBSTACLE,
            NavigationBit::OBSTACLE,
            NavigationBit::OBSTACLE,
            NavigationBit::OBSTACLE,
            NavigationBit::OBSTACLE,
            NavigationBit::OBSTACLE,
        };
        return tile_traversability[tileType];
    }

    //===== TileGraphics =====

    glm::ivec2 TileGraphics::GetTileIdx(int borderType, int variation) const {
        if(cornerless) {
            return defaultIdx;
        }

        if(mapping.count(borderType)) {
            const std::vector<glm::ivec2>& indices = mapping.at(borderType);
            ASSERT_MSG(indices.size() > 0, "TileGraphics - there should be at least 1 tile index in the mapping");

            int idx = 0;
            if(indices.size() != 1) {
                //variation -> 0-50 = 1st option
                variation = std::min(variation, 99);
                idx = (variation / 50) * ((variation-50) % (indices.size()-1));
            }
            return indices.at(idx);
        }

        //TODO: what to do if given border type isn't specified?
        //  - don't throw exception
        //  - debug   - draw special tile (add magenta tile at every tileset's (0,0))
        //  - release - find the closest matching visuals (hamming distance between the corner encoding)


        ENG_LOG_ERROR("Missing tileset index for borderType={} (tile type={})", borderType, type);
        // throw std::runtime_error("");
        return defaultIdx;
    }

    void TileGraphics::Extend(int x, int y, int borderType) {
        glm::ivec2 idx = glm::ivec2(x, y);
        mapping[borderType].push_back(idx);
    }

    //===== MapTiles =====

    MapTiles::MapTiles(const glm::ivec2& size_) : size(size_) {
        data = new TileData[(size.x+1) * (size.y+1)];
    }

    MapTiles::~MapTiles() {
        Release();
    }

    MapTiles MapTiles::Clone() const {
        MapTiles m = MapTiles(size);
        memcpy(m.data, data, sizeof(TileData) * (size.x+1) * (size.y+1));
        return m;
    }

    MapTiles::MapTiles(MapTiles&& m) noexcept {
        Move(std::move(m));
    }
    
    MapTiles& MapTiles::operator=(MapTiles&& m) noexcept {
        Release();
        Move(std::move(m));
        return *this;
    }

    TileData& MapTiles::operator[](int i) {
        ASSERT_MSG(data != nullptr, "Map isn't properly initialized!");
        ASSERT_MSG(((unsigned(i) <= unsigned(size.y*size.x+size.y+size.x))), "Array index ([]) is out of bounds.", i);
        return data[i];
    }

    const TileData& MapTiles::operator[](int i) const {
        ASSERT_MSG(data != nullptr, "Map isn't properly initialized!");
        ASSERT_MSG(((unsigned(i) <= unsigned(size.y*size.x+size.y+size.x))), "Array index ([]) is out of bounds.", i);
        return data[i];
    }

    TileData& MapTiles::operator()(int y, int x) {
        ASSERT_MSG(data != nullptr, "Map isn't properly initialized!");
        ASSERT_MSG(((unsigned(y) <= unsigned(size.y)) && (unsigned(x) <= unsigned(size.x))), "Array index ({}, {}) is out of bounds.", y, x);
        return data[y*(size.x+1)+x];
    }

    const TileData& MapTiles::operator()(int y, int x) const {
        ASSERT_MSG(data != nullptr, "Map isn't properly initialized!");
        ASSERT_MSG(((unsigned(y) <= unsigned(size.y)) && (unsigned(x) <= unsigned(size.x))), "Array index ({}, {}) is out of bounds.", y, x);
        return data[y*(size.x+1)+x];
    }

    TileData& MapTiles::operator()(const glm::ivec2& idx) {
        return operator()(idx.y, idx.x);
    }

    const TileData& MapTiles::operator()(const glm::ivec2& idx) const {
        return operator()(idx.y, idx.x);
    }

    bool MapTiles::IsWithinBounds(const glm::ivec2& coords) const {
        return IsWithinBounds(coords.y, coords.x);
    }

    bool MapTiles::IsWithinBounds(const glm::ivec2& coords, int size) const {
        for(int y = 0; y < size; y++) {
            for(int x = 0; x < size; x++) {
                if(!IsWithinBounds(coords.y + y, coords.x + x))
                    return false;
            }
        }
        return true;
    }

    bool MapTiles::IsWithinBounds(int y, int x) const {
        return ((unsigned(y) < unsigned(size.y)) && (unsigned(x) < unsigned(size.x)));
    }

    void MapTiles::VisibilityIncrement(const glm::ivec2& pos, const glm::ivec2& obj_size, int range) {
        glm::ivec2 m = glm::ivec2(std::max(pos.x-range+1, 0), std::max(pos.y-range+1, 0));
        glm::ivec2 M = glm::ivec2(std::min(pos.x+obj_size.x+range, size.x+1), std::min(pos.y+obj_size.y+range, size.y+1));
        if(m.x >= M.x || m.y >= M.y) {
            ENG_LOG_WARN("MapTiles::Visibility - attempting to create visibility outside of the map bounds (m={}, M={})", m, M);
            return;
        }

        RoundCorners_Decrement(m, M, range);

        //iterate the revealed map patch, increment vision counter & mark all as unoccluded
        for(int y = m.y; y < M.y; y++) {
            for(int x = m.x; x < M.x; x++) {
                operator()(y, x).VisionIncrement();
            }
        }
    }

    void MapTiles::VisibilityDecrement(const glm::ivec2& pos, const glm::ivec2& obj_size, int range) {
        glm::ivec2 m = glm::ivec2(std::max(pos.x-range+1, 0), std::max(pos.y-range+1, 0));
        glm::ivec2 M = glm::ivec2(std::min(pos.x+obj_size.x+range, size.x+1), std::min(pos.y+obj_size.y+range, size.y+1));
        if(m.x >= M.x || m.y >= M.y) {
            ENG_LOG_WARN("MapTiles::Visibility - attempting to create visibility outside of the map bounds (m={}, M={})", m, M);
            return;
        }

        RoundCorners_Increment(m, M, range);
        
        for(int y = m.y; y < M.y; y++) {
            for(int x = m.x; x < M.x; x++) {
                operator()(y, x).VisionDecrement();
            }
        }
    }
    
    void MapTiles::VisibilityUpdate(const glm::ivec2& pos_prev, const glm::ivec2& pos_next, int range) {
        //TODO: could detect motion to neighboring tile & only update the borders instead of the entire vision range

        glm::ivec2 m = glm::ivec2(std::max(pos_prev.x-range+1, 0), std::max(pos_prev.y-range+1, 0));
        glm::ivec2 M = glm::ivec2(std::min(pos_prev.x+1+range, size.x+1), std::min(pos_prev.y+1+range, size.y+1));
        RoundCorners_Increment(m, M, range);
        for(int y = m.y; y < M.y; y++) {
            for(int x = m.x; x < M.x; x++) {
                operator()(y, x).VisionDecrement();
            }
        }

        m = glm::ivec2(std::max(pos_next.x-range+1, 0), std::max(pos_next.y-range+1, 0));
        M = glm::ivec2(std::min(pos_next.x+1+range, size.x+1), std::min(pos_next.y+1+range, size.y+1));
        RoundCorners_Decrement(m, M, range);
        for(int y = m.y; y < M.y; y++) {
            for(int x = m.x; x < M.x; x++) {
                operator()(y, x).VisionIncrement();
            }
        }
    }

    void MapTiles::UpdateOcclusionIndices() {
        for(int y = 0; y < size.y; y++) {
            TileData *a, *b, *c, *d;
            a = &operator()(y+0, 0);
            c = &operator()(y+1, 0);

            for(int x = 0; x < size.x; x++) {
                b = &operator()(y+0, x+1);
                d = &operator()(y+1, x+1);

                a->UpdateOcclusionIndices(a->vis.occlusion, b->vis.occlusion, c->vis.occlusion, d->vis.occlusion);
                
                a = b;
                c = d;
            }
        }
    }

    void MapTiles::MapReveal() {
        for(int y = 0; y <= size.y; y++) {
            for(int x = 0; x <= size.x; x++) {
                operator()(y, x).VisionIncrement();
            }
        }
    }

    void MapTiles::RoundCorners_Increment(const glm::ivec2& m, const glm::ivec2& M, int range) {
        if(range > 1) {
            operator()(m.y  , m.x  ).vis.visionCounter++;
            operator()(m.y  , M.x-1).vis.visionCounter++;
            operator()(M.y-1, m.x  ).vis.visionCounter++;
            operator()(M.y-1, M.x-1).vis.visionCounter++;
            if(range > 4) {
                operator()(m.y  , m.x+1).vis.visionCounter++;
                operator()(m.y  , M.x-2).vis.visionCounter++;
                operator()(m.y+1, m.x  ).vis.visionCounter++;
                operator()(m.y+1, M.x-1).vis.visionCounter++;
                operator()(M.y-2, m.x  ).vis.visionCounter++;
                operator()(M.y-2, M.x-1).vis.visionCounter++;
                operator()(M.y-1, m.x+1).vis.visionCounter++;
                operator()(M.y-1, M.x-2).vis.visionCounter++;
            }
        }
    }

    void MapTiles::RoundCorners_Decrement(const glm::ivec2& m, const glm::ivec2& M, int range) {
        if(range > 1) {
            operator()(m.y  , m.x  ).vis.visionCounter--;
            operator()(m.y  , M.x-1).vis.visionCounter--;
            operator()(M.y-1, m.x  ).vis.visionCounter--;
            operator()(M.y-1, M.x-1).vis.visionCounter--;
            if(range > 4) {
                operator()(m.y  , m.x+1).vis.visionCounter--;
                operator()(m.y  , M.x-2).vis.visionCounter--;
                operator()(m.y+1, m.x  ).vis.visionCounter--;
                operator()(m.y+1, M.x-1).vis.visionCounter--;
                operator()(M.y-2, m.x  ).vis.visionCounter--;
                operator()(M.y-2, M.x-1).vis.visionCounter--;
                operator()(M.y-1, m.x+1).vis.visionCounter--;
                operator()(M.y-1, M.x-2).vis.visionCounter--;
            }
        }
    }

    void MapTiles::NavDataCleanup() {
        int area = (size.x+1)*(size.y+1);
        for(int i = 0; i < area; i++)
            data[i].nav.Cleanup();
    }

    int MapTiles::NavData_Forrest_FloodFill(const glm::ivec2& pos) {
        std::vector<glm::ivec2> to_visit;
        to_visit.push_back(pos);

        int forrest_size = 0;
        for(size_t idx = 0; idx < to_visit.size(); idx++) {
            glm::ivec2 coords = to_visit.at(idx);
            TileData& td = operator()(coords);

            if(td.nav.part_of_forrest || !td.IsTreeTile())
                continue;
            
            td.nav.part_of_forrest = true;
            forrest_size++;

            if(coords.y > 0) {
                if(coords.x > 0)        to_visit.push_back(glm::ivec2(coords.x-1, coords.y-1));
                                        to_visit.push_back(glm::ivec2(coords.x+0, coords.y-1));
                if(coords.x < size.x)   to_visit.push_back(glm::ivec2(coords.x+1, coords.y-1));
            }

            if(coords.x > 0)            to_visit.push_back(glm::ivec2(coords.x-1, coords.y+0));
            if(coords.x < size.x)       to_visit.push_back(glm::ivec2(coords.x+1, coords.y+0));

            if(coords.y < size.y) {
                if(coords.x > 0)        to_visit.push_back(glm::ivec2(coords.x-1, coords.y+1));
                                        to_visit.push_back(glm::ivec2(coords.x+0, coords.y+1));
                if(coords.x < size.x)   to_visit.push_back(glm::ivec2(coords.x+1, coords.y+1));
            }
        }

        return forrest_size;
    }
    
    void MapTiles::Move(MapTiles&& m) noexcept {
        data = m.data;
        size = m.size;

        m.data = nullptr;
    }

    void MapTiles::Release() noexcept {
        delete[] data;
    }

    //===== Tileset =====

    Tileset::Tileset(const std::string& config_filepath, int flags) {
        data = ParseConfig_Tileset(config_filepath, flags);
    }

    void Tileset::UpdateTileIndices(MapTiles& tiles) const {
        glm::ivec2 size = tiles.Size();

        for(int y = 0; y < size.y; y++) {
            //adressing 4 tiles at a time to retrieve all cornerTypes for tile 'a'
            TileData *a, *b, *c, *d;
            a = &tiles(y+0, 0);
            c = &tiles(y+1, 0);

            for(int x = 0; x < size.x; x++) {
                b = &tiles(y+0, x+1);
                d = &tiles(y+1, x+1);
                
                UpdateTileIndex(tiles, y, x, a, b, c, d);
                
                a = b;
                c = d;
            }
        }
    }

    void Tileset::UpdateTileIndices(MapTiles& tiles, const glm::ivec2& coords) const {
        int x = coords.x;
        int y = coords.y;
        UpdateTileIndex(tiles, y, x, &tiles(y, x), &tiles(y, x+1), &tiles(y+1, x), &tiles(y+1, x+1));
    }

    void Tileset::UpdateTileIndex(MapTiles& tiles, int y, int x, TileData* a, TileData* b, TileData* c, TileData* d) const {
        if(!IsWallTile(a->tileType)) {
            //not a wall tile -> pick index based on corners
            int borderType = GetBorderType(a->tileType, a->cornerType, b->cornerType, c->cornerType, d->cornerType);
            a->idx = data.tiles[a->tileType].GetTileIdx(borderType, a->variation);
            a->coastal = (a->tileType == TileType::WATER) && (borderType != 0);
        }
        else {
            //wall tile -> pick index based on neighboring tile types
            int t = (y > 0) ? (int)IsWallTile(tiles[c2i(y-1, x, tiles.Size())].tileType) : 0;
            int l = (int)IsWallTile(b->tileType);
            int b = (int)IsWallTile(c->tileType);
            int r = (x > 0) ? (int)IsWallTile(tiles[c2i(y, x-1, tiles.Size())].tileType) : 0;
            int borderType = GetBorderType(1, t, r, b, l);
            a->idx = data.tiles[a->tileType].GetTileIdx(borderType, a->variation);
        }
    }

    glm::ivec2 Tileset::GetTileIdx(int tileType, int borderType, int variation) {
        ASSERT_MSG(((unsigned int)(tileType) < TileType::COUNT), "Attempting to use invalid tile type.");
        return data.tiles.at(tileType).GetTileIdx(borderType, variation);
    }

    //===== PaintBitmap =====

    PaintBitmap::PaintBitmap(const glm::ivec2& size_) : size(size_) {
        int area = (size.x+1) * (size.y+1);
        paint = new int[area];
        for(int i = 0; i < area; i++)
            paint[i] = 0;
    }

    PaintBitmap::~PaintBitmap() {
        delete[] paint;
    }

    PaintBitmap::PaintBitmap(PaintBitmap&& p) noexcept {
        paint = p.paint;
        size = p.size;

        p.paint = nullptr;
        p.size = glm::ivec2(0);
    }

    PaintBitmap& PaintBitmap::operator=(PaintBitmap&& p) noexcept {
        delete[] paint;

        paint = p.paint;
        size = p.size;

        p.paint = nullptr;
        p.size = glm::ivec2(0);

        return *this;
    }

    int& PaintBitmap::operator[](int i) {
        ASSERT_MSG(paint != nullptr, "PaintBitmap isn't properly initialized!");
        ASSERT_MSG((unsigned)(i) < (unsigned)((size.x+1) * (size.y+1)), "PaintBitmap - Index {} is out of bounds.", i);
        return paint[i];
    }

    const int& PaintBitmap::operator[](int i) const {
        ASSERT_MSG(paint != nullptr, "PaintBitmap isn't properly initialized!");
        ASSERT_MSG((unsigned)(i) < (unsigned)((size.x+1) * (size.y+1)), "PaintBitmap - Index {} is out of bounds.", i);
        return paint[i];
    }

    int& PaintBitmap::operator()(int y, int x) {
        ASSERT_MSG(paint != nullptr, "PaintBitmap isn't properly initialized!");
        ASSERT_MSG(((unsigned(y) < unsigned(size.y+1)) && (unsigned(x) < unsigned(size.x+1))), "PaintBitmap - Index ({}, {}) is out of bounds.", y, x);
        return operator[](y*(size.x+1) + x);
    }

    const int& PaintBitmap::operator()(int y, int x) const {
        ASSERT_MSG(paint != nullptr, "PaintBitmap isn't properly initialized!");
        ASSERT_MSG(((unsigned(y) < unsigned(size.y+1)) && (unsigned(x) < unsigned(size.x+1))), "PaintBitmap - Index ({}, {}) is out of bounds.", y, x);
        return operator[](y*(size.x+1) + x);
    }

    void PaintBitmap::Fill(int value) {
        int area = (size.x+1)*(size.y+1);
        for(int i = 0; i < area; i++)
            paint[i] = value;
    }

    //===== Map =====

    Map::Map(const glm::ivec2& size_, const TilesetRef& tileset_) : tiles(MapTiles(size_)), occlusion(GenOcclusionSprite()) {
        ChangeTileset(tileset_);
        Camera::Get().SetBounds(Size());
    }

    Map::Map(Mapfile&& mapfile) : tiles(std::move(mapfile.tiles)), occlusion(GenOcclusionSprite()) {
        ASSERT_MSG(tiles.Valid(), "Mapfile doesn't contain any tile descriptions!");
        ChangeTileset(Resources::LoadTileset(mapfile.tileset));
        Camera::Get().SetBounds(Size());
    }

    Map::Map(Map&& m) noexcept {
        Move(std::move(m));
    }

    Map& Map::operator=(Map&& m) noexcept {
        Move(std::move(m));
        return *this;
    }

    const TileData& Map::operator()(int y, int x) const {
        ASSERT_MSG((unsigned(y) < unsigned(tiles.Size().y)) && (unsigned(x) < unsigned(tiles.Size().x)), "Array index out of bounds.");
        return at(y,x);
    }

    const TileData& Map::operator()(const glm::ivec2& idx) const {
        return operator()(idx.y, idx.x);
    }

    int Map::DamageTile(const glm::ivec2& idx, int damage, const ObjectID& src) {
        constexpr static std::array<int, 6> tileMapping = {
            TileType::ROCK_BROKEN,
            TileType::WALL_HU_DAMAGED, 
            TileType::WALL_BROKEN,
            TileType::WALL_OC_DAMAGED, 
            TileType::WALL_BROKEN,
            TileType::TREES_FELLED,
        };

        TileData& td = at(idx.y, idx.x);
        if(!IsMapObject(td.tileType)) {
            ENG_LOG_WARN("Map::DamageTile - Attempting to damage a tile which isn't a map object (type={}).", td.tileType);
            return 0;
        }
        
        damage = int(damage * (Random::Uniform() * 0.5f + 0.5f));
        if(damage < 0) damage = 0;
        
        td.health -= damage;
        ENG_LOG_FINE("[DMG] {} dealt {} damage to map_object at ({},{}) (melee, remaining: {}/100).", src.to_string(), damage, idx.x, idx.y, td.health);

        if(td.health <= 0) {
            int newType = tileMapping[td.tileType - TileType::ROCK];
            ENG_LOG_FINE("Tile transformation [({},{})] - {} -> {}", idx.x, idx.y, td.tileType, newType);
            td.tileType = newType;
            td.health = 100;
            tileset->UpdateTileIndices(tiles, idx);
            td.UpdateID();
        }

        return damage;
    }

    bool Map::HarvestTile(const glm::ivec2& idx) {
        TileData& td = at(idx.y, idx.x);
        if(!td.IsTreeTile()) {
            return false;
        }

        td.health -= HARVEST_WOOD_HEALTH_TICK;
        if(td.health <= 0) {
            ENG_LOG_FINE("Tile transformation [({},{})] - {} -> {} (trees)", idx.x, idx.y, td.tileType, TileType::TREES_FELLED);
            td.tileType = TileType::TREES_FELLED;
            td.health = 100;
            tileset->UpdateTileIndices(tiles, idx);
            return true;
        }
        else
            return false;
    }

    Mapfile Map::Export() {
        Mapfile mapfile;
        mapfile.tiles = tiles.Clone();
        mapfile.tileset = tileset->Name();
        return mapfile;
    }

    void Map::Render() {
        Camera& cam = Camera::Get();

        if(enable_occlusion) {
            tiles.UpdateOcclusionIndices();
        }

        float zIdx_map = 0.f;
        float zIdx_occ = -0.7f;

        for(int y = 0; y < tiles.Size().y; y++) {
            for(int x = 0; x < tiles.Size().x; x++) {
                int i = c2i(y, x);
                glm::vec2 pos = cam.map2screen(glm::vec2(x, y));
                tileset->Tilemap().Render(glm::uvec4(0), glm::vec3(pos, zIdx_map), cam.Mult(), tiles[i].idx.y, tiles[i].idx.x);
                
                //use textures based on occlusion indices
                if(enable_occlusion) {
                    occlusion.Render(glm::vec3(pos, zIdx_occ), cam.Mult(), 0, tiles[i].vis.occ_idx, glm::vec4(0.f, 0.f, 0.f, 1.f));

                    if(Config::FogOfWar()) {
                        occlusion.Render(glm::vec3(pos, zIdx_occ+1e-3f), cam.Mult(), 1, tiles[i].vis.fog_idx, glm::vec4(0.f, 0.f, 0.f, 1.f));
                    }
                }
            }
        }
    }

    void Map::RenderRange(int xs, int ys, int w, int h) {
        Camera& cam = Camera::Get();

        for(int y = ys; y < std::min(tiles.Size().y, h); y++) {
            for(int x = xs; x < std::min(tiles.Size().x, w); x++) {
                int i = c2i(y, x);
                glm::vec3 pos = glm::vec3((glm::vec2(x, y) - 0.5f - cam.Position()) * cam.Mult(), 0.f);
                tileset->Tilemap().Render(glm::uvec4(0), pos, cam.Mult(), tiles[i].idx.y, tiles[i].idx.x);
            }
        }
    }

    bool Map::IsWithinBounds(const glm::ivec2& pos) const {
        return ((unsigned int)pos.y) < ((unsigned int)tiles.Size().y) && ((unsigned int)pos.x) < ((unsigned int)tiles.Size().x);
    }

    void Map::ChangeTileset(const TilesetRef& tilesetNew) {
        TilesetRef ts = (tilesetNew != nullptr) ? tilesetNew : Resources::DefaultTileset();
        if(tileset != tilesetNew) {
            tileset = tilesetNew;
            tileset->UpdateTileIndices(tiles);
        }
    }

    void Map::UntouchabilityUpdate(const std::vector<int>& faction_bits) {
        for(int y = 0; y < tiles.Size().y; y++) {
            for(int x = 0; x < tiles.Size().x; x++) {
                TileData& td = tiles(y,x);
                for(int i = 0; i < 2; i++) {
                    ObjectInfo& info = td.info[i];
                    info.untouchable = 0;

                    if(info.factionId != -1) {
                        if(info.invisible)
                            info.untouchable = faction_bits[info.factionId];
                        else if(info.num_id[0] == ObjectType::UNIT && info.num_id[1] == UnitType::SUBMARINE) {
                            info.untouchable = InvisibilityDetection(y, x, faction_bits[info.factionId]);
                        }
                    }
                }
            }
        }

        if(Config::Hack_MapReveal()) {
            tiles.MapReveal();
        }
    }

    int Map::InvisibilityDetection(int ys, int xs, int untouchable) {
        int ym = std::max(0, ys - INVIS_DETECTION_RADIUS);
        int xm = std::max(0, xs - INVIS_DETECTION_RADIUS);
        int yM = std::min(tiles.Size().y, ys + INVIS_DETECTION_RADIUS);
        int xM = std::min(tiles.Size().x, xs + INVIS_DETECTION_RADIUS);

        for(int y = ym; y < yM; y++) {
            for(int x = xm; x < xM; x++) {
                if(tiles(y,x).info[1].factionId != -1) {
                    untouchable &= ~(1 << tiles(y,x).info[1].factionId);
                }
            }
        }

        return untouchable;
    }

    void Map::SetObjectInvisibility(const glm::ivec2& position, bool state, bool airborne) {
        ASSERT_MSG(IsWithinBounds(position), "Map::SetObjectInvisibility - Invalid position provided ({}, {})", position.x, position.y);
        ObjectInfo& info = tiles(position).info[int(airborne)];
        if(info.factionId >= 0) {
            info.invisible = state;
        }
        else {
            ENG_LOG_ERROR("Map::SetObjectInvisibility - location (({},{}), airborne={}) doesn't contain a valid object.", position.x, position.y, airborne);
            throw std::runtime_error("");
        }
    }

    bool Map::IsUntouchable(const glm::ivec2& position, bool airborne) const {
        return IsUntouchable(position, airborne, playerFactionId);
    }

    bool Map::IsUntouchable(const glm::ivec2& position, bool airborne, int factionIdx) const {
        ASSERT_MSG(IsWithinBounds(position), "Map::IsUntouchable - Invalid position provided ({}, {})", position.x, position.y);
        const ObjectInfo& info = tiles(position).info[int(airborne)];
        ASSERT_MSG(ObjectID::IsValid(info.id), "Map::IsUntouchable - location (({},{}), airborne={}) doesn't contain a valid object.", position.x, position.y, airborne);
        return info.IsUntouchable(factionIdx);
    }

    glm::ivec2 Map::Pathfinding_NextPosition(const Unit& unit, const glm::ivec2& target_pos) {
        ENG_LOG_FINEST("    Map::Pathfinding::Lookup | from ({},{}) to ({},{}) (unit {})", unit.Position().x, unit.Position().y, target_pos.x, target_pos.y, unit.ID());

        //identify unit's movement type (gnd/water/air)
        int navType = unit.NavigationType();

        glm::ivec2 unit_pos = unit.Position();
        glm::ivec2 dst_pos = target_pos;

        //coordinates fix for airborne units
        if(navType != NavigationBit::GROUND) {
            unit_pos = make_even(unit_pos);
            dst_pos = make_even(dst_pos);
        }

        if(unit_pos == target_pos)
            return target_pos;

        //fills distance values in the tile data
        Pathfinding_AStar(tiles, nav_list, unit_pos, dst_pos, navType, &heuristic_euclidean);
        // DBG_PrintDistances();
        
        //assembles the path, returns the next tile coord, that can be reached via a straight line travel
        return Pathfinding_RetrieveNextPos(unit_pos, dst_pos, navType);
    }

    glm::ivec2 Map::Pathfinding_NextPosition_Range(const Unit& unit, const glm::ivec2& m, const glm::ivec2& M, int distance) {
        ENG_LOG_FINEST("    Map::Pathfinding::Lookup | from ({},{}) to block [({},{})-({},{})] (unit {}, range {})", unit.Position().x, unit.Position().y, m.x, m.y, M.x, M.y, unit.ID(), unit.AttackRange());

        if(distance < 0) distance = unit.AttackRange();

        //already within the range
        if(get_range(unit.Position(), m, M) <= distance)
            return unit.Position();

        //identify unit's movement type (gnd/water/air)
        int navType = unit.NavigationType();

        //target fix for airborne units
        glm::ivec2 dm = m;
        glm::ivec2 dM = M;

        //fills distance values in the tile data
        glm::ivec2 dst_pos = glm::ivec2(-1);
        if(!Pathfinding_AStar_Range(tiles, nav_list, unit.Position(), distance, dm, dM, &dst_pos, navType, &heuristic_euclidean)) {
            //destination unreachable
            return unit.Position();
        }
        // DBG_PrintDistances();

        if(navType != NavigationBit::GROUND)
            dst_pos = make_even(dst_pos);
        
        //assembles the path, returns the next tile coord, that can be reached via a straight line travel
        return Pathfinding_RetrieveNextPos(unit.Position(), dst_pos, navType);
    }

    bool Map::Pathfinding_CanGetInRange(const FactionObject& src, const glm::ivec2& m, const glm::ivec2& M, int distance) {
        if(get_range(src.Position(), m, M) <= src.AttackRange())
            return true;
        else if(src.IsUnit()) {
            if(src.NavigationType() != NavigationBit::AIR)
                return Pathfinding_NextPosition_Range((Unit&)src, m, M, distance) != src.Position();
            else
                return true;
        }
        else
            return false;
    }

    glm::ivec2 Map::Pathfinding_NextPosition_Forrest(const Unit& unit, const glm::ivec2& target_pos) {
        ENG_LOG_FINEST("    Map::Pathfinding::Lookup | from ({},{}) to ({},{}) (unit {}; forrest search)", unit.Position().x, unit.Position().y, target_pos.x, target_pos.y, unit.ID());

        if(unit.Position() == target_pos)
            return target_pos;

        //identify unit's movement type (gnd/water/air)
        int navType = unit.NavigationType();

        //target fix for airborne units
        glm::ivec2 dst_pos = (navType == NavigationBit::GROUND) ? target_pos : make_even(target_pos);

        //fills distance values in the tile data
        Pathfinding_AStar_Forrest(tiles, nav_list, unit.Position(), dst_pos, navType, &heuristic_euclidean);
        // DBG_PrintDistances();
        
        //assembles the path, returns the next tile coord, that can be reached via a straight line travel
        return Pathfinding_RetrieveNextPos(unit.Position(), dst_pos, navType);
    }

    bool Map::Pathfinding_NextPosition_NearestBuilding(const Unit& unit, const std::vector<buildingMapCoords>& buildings, ObjectID& out_id, glm::ivec2& out_nextPos) {
        ENG_LOG_FINEST("    Map::Pathfinding::Lookup | from ({},{}) (unit {}; nearest building, {} buildings)", unit.Position().x, unit.Position().y, unit.ID(), (int)buildings.size());

        //identify unit's movement type (gnd/water/air)
        int navType = unit.NavigationType();
        int res_type = unit.CarryStatus();

        glm::ivec2 dst_pos;
        if(!Pathfinding_Dijkstra_NearestBuilding(tiles, nav_list, unit.Position(), buildings, res_type, navType, dst_pos)) {
            return false;
        }

        //resolve which building was selected
        out_id = ObjectID();
        for(auto& [m,M,ID,resType] : buildings) {
            if(!HAS_FLAG(resType, res_type))
                continue;

            if(dst_pos.x >= m.x && dst_pos.x <= M.x && dst_pos.y >= m.y && dst_pos.y <= M.y) {
                out_id = ID;
                break;
            }

            ENG_LOG_TRACE("Building {}: ({}, {}) - ({}, {})", ID, m.x, m.y, M.x, M.y);

        }
        ENG_LOG_FINER("    Map::Pathfinding::Result | Building ID: {}, destination = ({}, {})", out_id, dst_pos.x, dst_pos.y);
        ASSERT_MSG(ObjectID::IsValid(out_id), "Pathfinding_NextPosition_NearestBuilding - there should always be valid building at this point.");

        //round up, so that destination still lies within the building
        if(navType != NavigationBit::GROUND)
            dst_pos = make_even(dst_pos + 1);
 
        out_nextPos = Pathfinding_RetrieveNextPos(unit.Position(), dst_pos, navType);
        return true;
    }

    glm::ivec2 Map::Pathfinding_DockingLocation(const glm::ivec2& transport_pos, const glm::ivec2& unit_pos) {
        ENG_LOG_FINEST("    Map::Pathfinding::Lookup | from ({},{}) to ({},{}) (docking)", transport_pos.x, transport_pos.y, unit_pos.x, unit_pos.y);
        
        int navType = NavigationBit::WATER;
        //TODO: might want to take into consideration coast tile accessibility (from the unit's POV)

        //coordinates fix for non-ground units
        glm::ivec2 src_pos = make_even(transport_pos);
        glm::ivec2 dst_pos = make_even(unit_pos);

        if(src_pos == dst_pos)
            return src_pos;

        //fills distance values in the tile data
        Pathfinding_AStar(tiles, nav_list, src_pos, dst_pos, navType, &heuristic_euclidean);
        // DBG_PrintDistances();
        
        //assembles the path, returns the next tile coord, that can be reached via a straight line travel
        return Pathfinding_RetrieveFinalPos(src_pos, dst_pos, navType);
    }

    int Map::NearbySpawnCoords(glm::ivec2 building_pos, glm::ivec2 building_size, int preferred_dir, int nav_type, glm::ivec2& out_coords, int max_range) {
        ASSERT_MSG(preferred_dir >= 0 && preferred_dir <= 3, "preferred_dir has to be in range <0,3> (identifies preferred side of the building)");

        /*     2
            -------
          3 |     | 1
            |     |
            -------
               0
        */

        int step = 1;
        if(nav_type != NavigationBit::GROUND) {
            building_pos = (building_pos / 2) * 2;
            building_size = ((building_size + 1) / 2) * 2;
            step = 2;
        }

        //cap search at map size in case of unlimited range
        if(max_range < 1) {
            max_range = std::max(tiles.Size().x, tiles.Size().y);
        }

        glm::ivec2 pref_dir = glm::ivec2(
            ((preferred_dir+1) % 2) * (-2*(preferred_dir / 2)+1), 
            ((preferred_dir+0) % 2) * (-2*(preferred_dir / 2)+1)
        );

        // ENG_LOG_INFO("pos: ({}, {}), size: ({}, {}), dir: {} -> ({}, {})", building_pos.x, building_pos.y, building_size.x, building_size.y, preferred_dir, pref_dir.x, pref_dir.y);

        glm::ivec2 start_offset_dir = glm::ivec2(((preferred_dir+1)%4)/2, preferred_dir / 2);
        // ENG_LOG_INFO("DBG - ({}, {})", start_offset_dir.x, start_offset_dir.y);

        //start searching at neighboring tiles & slowly increase the searched radius
        for(int r = step; r < max_range; r += step) {
            glm::ivec2 size = building_size-step + 2*r;
            glm::ivec2 pos = building_pos - r + start_offset_dir * size;
            // if(nav_type != NavigationBit::GROUND) pos = (pos / 2) * 2;
            glm::ivec2 dir = pref_dir * step;
            // ENG_LOG_INFO("R={}", r);

            bool stillValid = false;

            //fillout the coords to visit - square radius around the building
            for(int i = 0; i < 4; i++) {
                int sz = size[int((preferred_dir+i) % 2 != 0)];
                for(int j = 0; j < sz; j += step) {
                    bool withinBounds = tiles.IsWithinBounds(pos, step);
                    stillValid |= withinBounds;
                    
                    // ENG_LOG_INFO("   - ({}, {})", pos.x, pos.y);
                    if(withinBounds && tiles(pos).Traversable(nav_type)) {
                        out_coords = pos;
                        return r;
                    }

                    pos += dir;
                }

                //rotate 90 degrees
                dir = glm::ivec2(-dir.y, dir.x);
            }

            //terminate early, if not a single coordinate on current range was in map bounds
            if(!stillValid)
                break;
        }

        return -1;
    }

    bool Map::IsAreaBuildable(const glm::ivec2& position, const glm::ivec2& building_size, int nav_type, const glm::ivec2& worker_pos, bool coastal, bool requires_foundation) const {
        if(coastal && NearestOilPatchDistance(position, building_size) < MIN_OIL_PATCH_DISTANCE) {
            return false;
        }

        if(requires_foundation && !BuildingFoundationCheck(position, BuildingType::OIL_PATCH)) {
            return false;
        }

        bool has_coastal_tiles = false;
        for(int y = position.y; y < position.y + building_size.y; y++) {
            for(int x = position.x; x < position.x + building_size.x; x++) {
                if(glm::ivec2(x,y) != worker_pos && (!tiles.IsWithinBounds(y,x) || !tiles(y,x).Traversable(nav_type))) {
                    return false;
                }
                has_coastal_tiles |= tiles(y,x).IsCoastTile();
            }
        }
        return has_coastal_tiles || !coastal;
    }

    bool Map::IsBuildable(const glm::ivec2& position, int nav_type, const glm::ivec2& worker_pos) const {
        return (tiles.IsWithinBounds(position) && tiles(position).Traversable(nav_type)) || (position == worker_pos); 
    }

    bool Map::CoastCheck(const glm::ivec2& position, const glm::ivec2& building_size, bool coastal) const {
        if(!coastal)
            return true;

        if(NearestOilPatchDistance(position, building_size) < MIN_OIL_PATCH_DISTANCE)
            return false;

        return IsLocationCoastal(position, building_size);
    }

    bool Map::IsLocationCoastal(const glm::ivec2& position, const glm::ivec2& size) const {
        bool has_coast = false;

        for(int y = position.y; y < position.y + size.y; y++) {
            for(int x = position.x; x < position.x + size.x; x++) {
                if(!tiles.IsWithinBounds(y,x))
                    return false;
                has_coast |= tiles(y,x).IsCoastTile();
            }
        }

        return has_coast;
    }

    bool Map::CanSpawn(const glm::ivec2& position, int unitNavType) {
        return tiles.IsWithinBounds(position) && tiles(position).Traversable(unitNavType);
    }

    bool Map::HasValidPlacement_Unit(const glm::ivec2& position, int unitNavType) {
        if(unitNavType != NavigationBit::WATER)
            return tiles.IsWithinBounds(position) && tiles(position).Traversable_Terrain(unitNavType);
        else {
            bool valid = true;
            for(int y = 0; y < 2; y++) {
                for(int x = 0; x < 2; x++) {
                    valid &= tiles.IsWithinBounds(position) && tiles(position).Traversable_Terrain(unitNavType);
                }
            }
            return valid;
        }
    }

    bool Map::HasValidPlacement_Building(const glm::ivec2& position, const glm::ivec2& size, int navType, bool coastal, bool is_oil) {
        bool has_coast = false;

        for(int y = position.y; y < position.y + size.y; y++) {
            for(int x = position.x; x < position.x + size.x; x++) {
                glm::ivec2 pos = glm::ivec2(x,y);
                if(!tiles.IsWithinBounds(pos) || !tiles(pos).Traversable_Terrain(navType))
                    return false;
                has_coast |= tiles(y,x).IsCoastTile();
            }
        }

        return (navType != NavigationBit::WATER) ? (!has_coast) : (is_oil ^ has_coast);
    }

    int Map::NearestOilPatchDistance(const glm::ivec2& pos, const glm::ivec2& size) const {
        int min_d = std::numeric_limits<int>::max();
        for(auto& to : traversableObjects) {
            if(to.type != BuildingType::OIL_PATCH && to.type != BuildingType::OIL_PLATFORM)
                continue;
            
            int distance = get_range(to.pos, to.pos + 2, pos, pos+size);

            if(distance < min_d)
                min_d = distance;
        }
        return min_d;
    }

    bool Map::BuildingFoundationCheck(const glm::ivec2& position, int type) const {
        for(const auto& to : traversableObjects) {
            if(to.type == type && to.pos == position)
                return true;
        }
        return false;
    }

    ObjectID Map::GetFoundationObjectAt(const glm::ivec2& position) const {
        for(const auto& to : traversableObjects) {
            if(to.pos == position)
                return to.id;
        }
        return ObjectID();
    }
    
    bool Map::FindTrees(const glm::ivec2& worker_pos, const glm::ivec2& preferred_pos, glm::ivec2& out_pos, int radius) {
        glm::ivec2 lim = glm::ivec2(std::min(worker_pos.x + radius, Size().x-1), std::min(worker_pos.y + radius, Size().y-1));

        glm::ivec2 coords = glm::ivec2(-1, -1);
        unsigned int D = (unsigned int)(-1);

        for(int y = std::max(worker_pos.y - radius, 0); y <= lim.y; y++) {
            for(int x = std::max(worker_pos.x - radius, 0); x <= lim.x; x++) {
                if(tiles(y,x).IsTreeTile()) {
                    unsigned int d = (unsigned int)chessboard_distance(worker_pos, glm::ivec2(x,y));
                    if(d < D) {
                        D = d;
                        coords = glm::ivec2(x,y);
                    }
                }
            }
        }

        out_pos = coords;
        return D < (unsigned int)(-1);
    }

    bool Map::SearchForTarget(const FactionObject& src, const DiplomacyMatrix& diplomacy, int range, ObjectID& out_targetID, glm::ivec2* out_targetPos) {
        glm::ivec2 sz = glm::ivec2(src.Data()->size) - 1;
        glm::ivec2 src_pos = src.Position();
        int src_factionId = src.FactionIdx();

        //scan the neighboring tiles
        for(int y = -range; y <= range+sz.y; y++) {
            int d_y = std::abs(y) - std::clamp(y, 0, sz.y);

            for(int x = -range; x <= range+sz.x; x++) {
                int d_x = std::abs(x) - std::clamp(x, 0, sz.x);
                int distance = std::min(d_x, d_y);
                
                //ignore tiles with that are outside of attack range (chessboard distance, accounting for object size > 1)
                if(distance > range)
                    continue;

                glm::ivec2 pos = glm::ivec2(src_pos.x + x, src_pos.y + y);
                if(tiles.IsWithinBounds(pos)) {
                    if(diplomacy.AreHostile(src_factionId, tiles(pos).info[0].factionId) && !IsUntouchable(pos, false) && src.CanAttackTarget(tiles(pos).TileTraversability()) && Pathfinding_CanGetInRange(src, pos, pos)) {
                        out_targetID = tiles(pos).info[0].id;
                        if(out_targetPos != nullptr)
                            *out_targetPos = pos;
                        return true;
                    }
                    else if(diplomacy.AreHostile(src_factionId, tiles(pos).info[1].factionId) && !IsUntouchable(pos, true) && src.CanAttackTarget(NavigationBit::AIR) && Pathfinding_CanGetInRange(src, pos, pos)) {
                        out_targetID = tiles(pos).info[1].id;
                        if(out_targetPos != nullptr)
                            *out_targetPos = pos;
                        return true;
                    }
                }
            }
        }

        return false;
    }

    void Map::AddObject(int navType, const glm::ivec2& pos, const glm::ivec2& size, const ObjectID& id, int factionId, int colorIdx, const glm::ivec3& num_id, bool is_building, int sight) {
        ENG_LOG_FINE("Map::AddObject - adding at [{},{}], size [{},{}]", pos.x, pos.y, size.x, size.y);

        if(size.x == 0 || size.y == 0)
            ENG_LOG_WARN("Map::AddObject - object has no size");
        
        int i = int(navType == NavigationBit::AIR);

        for(int y = 0; y < size.y; y++) {
            for(int x = 0; x < size.x; x++) {
                glm::vec2 idx = glm::ivec2(pos.x+x, pos.y+y);
                if(!tiles(idx).Traversable(navType))
                    ENG_LOG_WARN("Map::AddObject - invalid placement location {} (navType={})", idx, navType);
                tiles(idx).nav.Claim(navType, true, is_building);
                tiles(idx).info[i].id = id;
                tiles(idx).info[i].factionId = factionId;
                tiles(idx).info[i].colorIdx = colorIdx;
                tiles(idx).info[i].num_id = num_id;
            }
        }

        VisibilityIncrement(pos, size, sight, factionId);
    }

    void Map::RemoveObject(int navType, const glm::ivec2& pos, const glm::ivec2& size, bool is_building, int factionId, int sight) {
        ENG_LOG_FINE("Map::RemoveObject - removing at [{},{}], size [{},{}]", pos.x, pos.y, size.x, size.y);

        if(size.x == 0 || size.y == 0)
            ENG_LOG_WARN("Map::RemoveObject - object has no size");

        int i = int(navType == NavigationBit::AIR);

        for(int y = 0; y < size.y; y++) {
            for(int x = 0; x < size.x; x++) {
                glm::vec2 idx = glm::ivec2(pos.x+x, pos.y+y);
                if(tiles(idx).Traversable(navType))
                    ENG_LOG_WARN("Map::RemoveObject - location {} already marked as free (navType={})", idx, navType);
                tiles(idx).nav.Unclaim(navType, is_building);
                tiles(idx).info[i] = {};
            }
        }

        VisibilityDecrement(pos, size, sight, factionId);
    }

    void Map::MoveUnit(int unit_navType, const glm::ivec2& pos_prev, const glm::ivec2& pos_next, bool permanently, int sight) {
        tiles(pos_prev).nav.Unclaim(unit_navType, false);
        tiles(pos_next).nav.Claim(unit_navType, permanently, false);
        
        int i = int(unit_navType == NavigationBit::AIR);
        
        tiles(pos_next).info[i] = tiles(pos_prev).info[i];
        tiles(pos_prev).info[i] = ObjectInfo{};

        int factionId = tiles(pos_next).info[i].factionId;

        if(factionId == playerFactionId) {
            VisibilityUpdate(pos_prev, pos_next, sight);
        }

        //runes dispatch (non-air units only)
        if(i == 0 && tiles(pos_next).rune_id != 0) {
            rune_dispatch.push_back({pos_next, tiles(pos_next).info[0].id});
            tiles(pos_next).rune_id = 0;
        }
    }

    void Map::SpawnRunes(const glm::ivec2& pos, ObjectID::dtype ID) {
        tiles(pos.y+0, pos.x+0).rune_id = ID;
        tiles(pos.y+1, pos.x+0).rune_id = ID;
        tiles(pos.y-1, pos.x+0).rune_id = ID;
        tiles(pos.y+0, pos.x-1).rune_id = ID;
        tiles(pos.y+0, pos.x+1).rune_id = ID;
    }

    int Map::DespawnRunes(const glm::ivec2& pos, ObjectID::dtype ID) {
        int count = 0;

        count += tiles(pos.y+0, pos.x+0).DespawnRune(ID);
        count += tiles(pos.y+1, pos.x+0).DespawnRune(ID);
        count += tiles(pos.y-1, pos.x+0).DespawnRune(ID);
        count += tiles(pos.y+0, pos.x-1).DespawnRune(ID);
        count += tiles(pos.y+0, pos.x+1).DespawnRune(ID);

        return count;
    }

    std::vector<ObjectID> Map::ObjectsInArea(const glm::ivec2& start, const glm::ivec2& end, bool ignore_unselectable) {
        glm::ivec2 m = glm::max(start, glm::ivec2(0));
        glm::ivec2 M = glm::min(end, Size());

        std::vector<ObjectID> res = {};

        for(int y = m.y; y < M.y; y++) {
            for(int x = m.x; x < M.x; x++) {
                glm::ivec2 pos = glm::ivec2(x, y);
                TileData& td = tiles(pos);
                bool tile_visible = td.IsVisible();

                for(int i = 0; i < 2; i++) {
                    ObjectInfo& info = td.info[i];
                    
                    if(ObjectID::IsValid(info.id) && ObjectID::IsObject(info.id) && (!ignore_unselectable || (tile_visible && !info.IsUntouchable(playerFactionId))) && std::find(res.begin(), res.end(), info.id) == res.end()) {
                        res.push_back(info.id);
                    }
                }
            }
        }

        return res;
    }

    std::vector<ObjectID> Map::EnemyUnitsInArea(const DiplomacyMatrix& diplomacy, const glm::ivec2& position, int radius, int factionID) {
        glm::ivec2 m = glm::max(position - radius, glm::ivec2(0));
        glm::ivec2 M = glm::min(position + radius, Size());

        std::vector<ObjectID> enemies = {};
        
        for(int y = m.y; y < M.y; y++) {
            for(int x = m.x; x < M.x; x++) {
                glm::ivec2 pos = glm::ivec2(x, y);

                for(int i = 0; i < 2; i++) {
                    ObjectInfo& info = tiles(pos).info[i];
                    if(info.id.type == ObjectType::UNIT && diplomacy.AreHostile(factionID, info.factionId)) {
                        enemies.push_back(info.id);
                    }
                }
            }
        }

        return enemies;
    }

    int Map::SelectByType(const glm::ivec3& num_id, std::array<ObjectID, 9>& results, const glm::ivec2& coords_from, const glm::ivec2& coords_to) {
        glm::ivec2 m = glm::clamp(coords_from, glm::ivec2(0), Size());
        glm::ivec2 M = glm::clamp(coords_to, glm::ivec2(0), Size());

        int i = num_id[0] == ObjectType::UNIT && (num_id[1] == UnitType::EYE || num_id[1] == UnitType::GRYPHON || num_id[1] == UnitType::ROFLCOPTER);

        int j = 0;

        for(int y = m.y; y < M.y; y++) {
            for(int x = m.x; x < M.x; x++) {
                glm::ivec2 pos = glm::ivec2(x, y);
                ObjectInfo& info = tiles(pos).info[i];
                if(info.num_id == num_id) {
                    if(num_id[0] == ObjectType::BUILDING) {
                        bool duplicate = false;
                        for(int k = 0; k < j; k++) {
                            if(results[k] == info.id) {
                                duplicate = true;
                                break;
                            }
                        }
                        if(duplicate)
                            continue;
                    }

                    results[j++] = info.id;
                    if(j >= 9) {
                        return 9;
                    }
                }
                
            }
        }

        return j;
    }

    void Map::UploadOcclusionMask(const OcclusionMask& occlusion, int playerFactionId_) {
        ASSERT_MSG(Size() == occlusion.Size(), "Map size and occlusion mask size do not match.");

        playerFactionId = playerFactionId_;

        //copy the explored bit from occlusion mask, set fog of war everywhere
        for(int y = 0; y < Size().y; y++) {
            for(int x = 0; x < Size().x; x++) {
                // tiles(y,x).vis.occlusion = occlusion(y,x) & (~2);
                tiles(y,x).vis.occlusion = occlusion(y,x) & 1;
            }
        }
        // occlusion.DBG_Print();
    }

    void Map::DownloadOcclusionMask(OcclusionMask& occlusion, int playerFactionId_) {
        ASSERT_MSG(Size() == occlusion.Size(), "Map size and occlusion mask size do not match.");

        for(int y = 0; y < Size().y; y++) {
            for(int x = 0; x < Size().x; x++) {
                occlusion(y,x) = int(tiles(y,x).vis.occlusion & (~2) != 0);
            }
        }
    }

    void Map::VisibilityIncrement(const glm::ivec2& pos, const glm::ivec2& size, int range, int factionID) {
        if(factionID == playerFactionId)
            tiles.VisibilityIncrement(pos, size, range);
    }

    void Map::VisibilityDecrement(const glm::ivec2& pos, const glm::ivec2& size, int range, int factionID) {
        if(factionID == playerFactionId)
            tiles.VisibilityDecrement(pos, size, range);
    }
    
    void Map::VisibilityUpdate(const glm::ivec2& pos_prev, const glm::ivec2& pos_next, int range) {
        tiles.VisibilityUpdate(pos_prev, pos_next, range);
    }

    void Map::VisionRangeUpdate(const glm::ivec2& pos, const glm::ivec2& size, int old_range, int new_range) {
        tiles.VisibilityDecrement(pos, size, old_range);
        tiles.VisibilityIncrement(pos, size, new_range);
    }

    void Map::AddTraversableObject(const ObjectID& id, const glm::ivec2& position, int type) {
        ENG_LOG_FINE("Map::AddTraversableObject - adding {}", id);
        traversableObjects.push_back(TraversableObjectEntry(id, position, type));
    }

    void Map::RemoveTraversableObject(const ObjectID& id) {
        for(size_t i = 0; i < traversableObjects.size(); i++) {
            if(traversableObjects[i].id == id) {
                ENG_LOG_FINE("Map::RemoveTraversableObject - removing {} ({}, type {})", id, traversableObjects[i].pos, traversableObjects[i].type);
                traversableObjects.erase(traversableObjects.begin() + i);
                return;
            }
        }
        ENG_LOG_WARN("Map::RemoveTraversableObject - {} not found", id);
    }

    glm::ivec2 Map::GetPanicMovementLocation(const Unit& src) {
        //TODO:
        return src.Position();
    }

    ObjectID Map::ObjectIDAt(glm::ivec2& out_coords) const {
        bool out_airborne = false;
        return ObjectIDAt(out_coords, out_airborne);
        // return IsWithinBounds(coords) ? (ObjectID::IsValid(tiles(coords).info[0].id) ? tiles(coords).info[0].id : tiles(coords).info[1].id) : ObjectID();
    }

    ObjectID Map::ObjectIDAt(glm::ivec2& out_coords, bool& out_isAirborne) const {
        out_isAirborne = false;
        if(!IsWithinBounds(out_coords))
            return ObjectID();
        
        //regular ground unit
        if(ObjectID::IsValid(tiles(out_coords).info[0].id))
            return tiles(out_coords).info[0].id;
        
        glm::ivec2 orig_coords = out_coords;
        out_coords = make_even(out_coords);
        //water check
        if(tiles(orig_coords).IsEmptyWaterTile() && ObjectID::IsValid(tiles(out_coords).info[0].id)) {
            return tiles(out_coords).info[0].id;
        }
        
        //airborne check
        out_isAirborne = true;
        return tiles(out_coords).info[1].id;
    }

    void Map::UndoChanges(std::vector<TileRecord>& history, bool rewrite_history) {
        for(TileRecord& tr : history) {
            TileData& td = at(tr.pos.y, tr.pos.x);

            int tileType    = tr.tileType;
            int cornerType  = tr.cornerType;
            int variation   = tr.variation;

            if(rewrite_history) {
                tr.tileType     = td.tileType;
                tr.cornerType   = td.cornerType;
                tr.variation    = td.variation;
            }

            td.tileType     = tileType;
            td.cornerType   = cornerType;
            td.variation    = variation;
        }

        DBG_PrintTiles();

        tileset->UpdateTileIndices(tiles);
    }

    void Map::ModifyTiles(PaintBitmap& paint, int tileType, bool randomVariation, int variationValue, std::vector<TileRecord>* history) {
        ASSERT_MSG(paint.Size() == tiles.Size(), "PaintBitmap size doesn't match the map size!");

        std::mt19937 gen = std::mt19937(std::random_device{}());
        std::uniform_int_distribution<int> dist = std::uniform_int_distribution(0, 100);

        std::vector<TileMod> affectedTiles;
        std::vector<TileRecord> modified;

        //the same as tileType, except for walls (walls use grass)
        int cornerType = ResolveCornerType(tileType);

        //iterate through the paint and retrieve all marked tiles & their direct neighbors
        for(int y = 0; y < tiles.Size().y; y++) {
            for(int x = 0; x < tiles.Size().x; x++) {
                int& flag = paint(y, x);

                if(HAS_FLAG(flag, PaintFlags::MARKED_FOR_PAINT) && !HAS_FLAG(flag, PaintFlags::VISITED)) {
                    DetectAffectedTiles(paint, y, x, affectedTiles, cornerType);
                }
            }
        }

        //iterate through the list of affected tiles
        for(size_t i = 0; i < affectedTiles.size(); i++) {
            TileMod& m = affectedTiles[i];
            TileData& td = at(m.y, m.x);

            //skip entries with tiles that are already marked as completed (in case there're duplicate entries in the list)
            if(HAS_FLAG(paint(m.y, m.x), PaintFlags::FINALIZED))
                continue;
            paint(m.y, m.x) |= PaintFlags::FINALIZED;

            //write into history
            modified.push_back(TileRecord(td, m.prevCornerType, m.y, m.x));

            //tile type resolution
            if(m.painted) {
                //directly marked as painted, can just override values
                td.tileType = tileType;
                td.variation = randomVariation ? dist(gen) : variationValue;
            }
            else {
                //border tile; may have been indirectly affected (will become transition tile)

                // DBG_PrintTiles();

                //non-zero depth means that entry was generated through conflict resolution -> use different dominantCornerType
                int cType = (m.depth == 0) ? cornerType : m.dominantCornerType;

                // paint(m.y, m.x) |= PaintFlags::DEBUG;
                int tileType = ResolveCornerConflict(m, affectedTiles, cType, m.depth);
                td.variation = randomVariation ? dist(gen) : variationValue;

                //don't replace tiles that use different corner type
                int cornerType = ResolveCornerType(td.tileType);
                if(cornerType != td.tileType && tileType == cornerType) {
                    tileType = td.tileType;
                }
                td.tileType = tileType;
            }
        }

        //update tile visuals
        tileset->UpdateTileIndices(tiles);

        ENG_LOG_TRACE("Map::ModifyTiles - number of affected tiles = {} ({})", modified.size(), affectedTiles.size());

        if(history != nullptr)
            *history = std::move(modified);
        
        DBG_PrintTiles();
        // if(history != nullptr) {
        //     printf("History (%d):\n", (int)history->size());
        //     for(TileRecord& tr : *history) {
        //         printf("[%d, %d]: t: %d, c: %d, v: %3d\n", tr.pos.y, tr.pos.x, tr.tileType, tr.cornerType, tr.variation);
        //     }
        // }
    }

    bool Map::IsTileVisible(const glm::ivec2& position) const {
        return operator()(position).IsVisible(enable_occlusion);
    }

    bool Map::IsTileVisible(const glm::ivec2& position, const glm::ivec2& size) const {
        for(int y = 0; y < size.y; y++) {
            for(int x = 0; x < size.x; x++) {
                if(operator()(glm::ivec2(position.x + x, position.y + y)).IsVisible(enable_occlusion))
                    return true;
            }
        }
        return false;
    }

    bool Map::IsDockingLocation(const glm::ivec2& bot_left) const {
        return 
            operator()(bot_left.y+0, bot_left.x+0).IsCoastTile() || 
            operator()(bot_left.y+0, bot_left.x+1).IsCoastTile() || 
            operator()(bot_left.y+1, bot_left.x+0).IsCoastTile() || 
            operator()(bot_left.y+1, bot_left.x+1).IsCoastTile();
    }

    void Map::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        ImGui::Begin("Map");

        static int mode = 0;

        ImVec2 btn_size = ImVec2(ImGui::GetWindowSize().x * 0.31f, 0.f);
        ImGui::Text("General");
        if(ImGui::Button("Tiles", btn_size)) mode = 0;
        ImGui::SameLine();
        if(ImGui::Button("Corners", btn_size)) mode = 1;
        ImGui::SameLine();
        if(ImGui::Button("Traversable", btn_size)) mode = 2;

        ImGui::Separator();
        if(ImGui::Button("ObjectIDs", btn_size)) mode = 6;
        ImGui::SameLine();
        if(ImGui::Button("Tile Health", btn_size)) mode = 7;
        ImGui::SameLine();
        if(ImGui::Button("FactionIdx", btn_size)) mode = 9;
        ImGui::Separator();

        ImGui::Text("Pathfinding");
        if(ImGui::Button("Taken", btn_size)) mode = 3;
        ImGui::SameLine();
        if(ImGui::Button("Visited", btn_size)) mode = 4;
        ImGui::SameLine();
        if(ImGui::Button("Distances", btn_size)) mode = 5;
        ImGui::Separator();

        if(ImGui::Button("Part-of-forrest", btn_size)) mode = 8;
        ImGui::SameLine();
        if(ImGui::Button("Visibility Count", btn_size)) mode = 10;
        ImGui::SameLine();
        if(ImGui::Button("FogOfVar corners", btn_size)) mode = 11;
        ImGui::Separator();

        if(ImGui::Button("Coast tiles", btn_size)) mode = 12;
        ImGui::SameLine();
        if(ImGui::Button("Untouchable/Runes", btn_size)) mode = 13;
        ImGui::SameLine();
        if(ImGui::Button("Invisible", btn_size)) mode = 14;
        

        ImGui::Checkbox("render_occlusion", &enable_occlusion);
        

        glm::ivec2 size = (mode != 1) ? tiles.Size() : (tiles.Size()+1);

        float TEXT_BASE_WIDTH = ImGui::CalcTextSize("A").x;
        float cell_width = TEXT_BASE_WIDTH * 3.f;
        int num_cols = size.x;
        switch(mode) {
            case 5:
                cell_width = TEXT_BASE_WIDTH * 6.f;
                break;
            case 6:
                cell_width = TEXT_BASE_WIDTH * 8.f;
                break;
        }
        
        static int navType = NavigationBit::GROUND;
        if(mode == 2 || mode == 3 || mode == 6 || mode == 9) {
            btn_size = ImVec2(ImGui::GetContentRegionAvail().x * 0.31f, 0.f);
            ImGui::Text("Unit Navigation Mode:");
            if(ImGui::Button("Ground", btn_size)) navType = NavigationBit::GROUND;
            ImGui::SameLine();
            if(ImGui::Button("Water", btn_size)) navType = NavigationBit::WATER;
            ImGui::SameLine();
            if(ImGui::Button("Air", btn_size)) navType = NavigationBit::AIR;
            ImGui::Separator();
        }
        int airborne = (navType == NavigationBit::AIR);

        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX;
        // flags |= ImGuiTableFlags_NoHostExtendY;
        flags |= ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersInnerV;

        if (ImGui::BeginTable("table1", num_cols, flags)) {

            for(int x = 0; x < num_cols; x++)
                ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, cell_width);

            ImU32 clr1 = ImGui::GetColorU32(ImVec4(1.0f, 0.1f, 0.1f, 1.0f));
            ImU32 clr2 = ImGui::GetColorU32(ImVec4(0.1f, 1.0f, 0.1f, 1.0f));
            ImU32 clr3 = ImGui::GetColorU32(ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
            ImU32 clr4 = ImGui::GetColorU32(ImVec4(0.30f, 0.53f, 0.91f, 1.0f));
            ImU32 clr5 = ImGui::GetColorU32(ImVec4(0.59f, 0.21f, 0.76f, 1.0f));

            bool b;
            float f;
            float D = 1.f;
            if(mode == 5) {
                for(int y = 0; y < size.y; y++) {
                    for(int x = 0; x < size.x; x++) {
                        if(tiles(y,x).nav.d > D && !std::isinf(tiles(y,x).nav.d))
                            D = tiles(y,x).nav.d;
                    }
                }
            }

            ImGuiStyle& style = ImGui::GetStyle();
            for(int y = size.y-1; y >= 0; y--) {
                ImGui::TableNextRow();
                // ImGui::TableNextRow(ImGuiTableRowFlags_None, cell_size);
                for(int x = 0; x < size.x; x++) {
                    ImGui::TableNextColumn();
                    switch(mode) {
                        case 0:
                            ImGui::Text("%d", tiles(y,x).tileType);
                            break;
                        case 1:
                            ImGui::Text("%d", tiles(y,x).cornerType);
                            break;
                        case 2:
                            ImGui::Text("%d", int(tiles(y,x).Traversable(navType)));
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, tiles(y,x).Traversable(navType) ? clr2 : clr1);
                            break;
                        case 3:
                            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(style.CellPadding.x, 0));
                            if(ImGui::BeginTable("table2", 2, ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX)) {
                                ImU32 taken_clr = HAS_FLAG(tiles(y,x).nav.taken, navType) ? clr1 : clr2;

                                ImGui::TableNextRow();
                                ImGui::TableNextColumn();
                                ImGui::Text("%d", tiles(y,x).nav.taken);
                                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, taken_clr);
                                ImGui::TableNextColumn();
                                ImGui::Text("%d", tiles(y,x).nav.permanent);
                                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, HAS_FLAG(tiles(y,x).nav.permanent, navType) ? clr1 : clr2);
                                ImGui::TableNextRow();
                                ImGui::TableNextColumn();
                                ImGui::Text("%d", int(tiles(y,x).nav.building));
                                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, tiles(y,x).nav.building ? clr4 : taken_clr);
                                ImGui::TableNextColumn();
                                ImGui::Text(" ");
                                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, tiles(y,x).nav.pathtile ? clr5 : taken_clr);
                                ImGui::EndTable();
                            }
                            ImGui::PopStyleVar();
                            break;
                        case 4:
                            ImGui::Text("%d", tiles(y,x).nav.visited);
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, tiles(y,x).nav.visited ? clr2 : clr1);
                            break;
                        case 5:
                            ImGui::Text("%5.1f", tiles(y,x).nav.d);
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(ImVec4(std::min(1.f, std::pow(tiles(y,x).nav.d / D, 2.2f)), 0.1f, 0.1f, 1.0f)));
                            break;
                        case 6:
                            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(style.CellPadding.x, 0));
                            if(tiles(y,x).info[airborne].id.type != ObjectType::INVALID) {
                                ImGui::Text("%d|%d|%d", int(tiles(y,x).info[airborne].id.type), int(tiles(y,x).info[airborne].id.idx), int(tiles(y,x).info[airborne].id.id));
                                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, clr4);
                            }
                            else {
                                ImGui::Text("---");
                            }
                            ImGui::PopStyleVar();
                            break;
                        case 7:
                            ImGui::Text("%d", tiles(y,x).health);
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(ImVec4(tiles(y,x).health / 100.f, 0.1f, 0.1f, 1.0f)));
                            break;
                        case 8:
                            ImGui::Text("%d", int(tiles(y,x).nav.part_of_forrest));
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, tiles(y,x).nav.part_of_forrest ? clr2 : clr1);
                            break;
                        case 9:
                            ImGui::Text("%d", tiles(y,x).info[airborne].factionId);
                            if(tiles(y,x).info[airborne].factionId >= 0) {
                                int a = tiles(y,x).info[airborne].factionId / 4;
                                int b = tiles(y,x).info[airborne].factionId % 4;
                                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImGui::GetColorU32(ImVec4(0.1f, 0.1f + 0.9f * ((1+b)*0.25f), 0.1f, 1.0f)));
                            }
                            else
                                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, clr3);
                            break;
                        case 10:
                            ImGui::Text("%d", tiles(y,x).vis.visionCounter);
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, (tiles(y,x).vis.visionCounter != 0) ? clr2 : clr1);
                            break;
                        case 11:
                            ImGui::Text("%d", tiles(y,x).vis.fog_idx);
                            break;
                        case 12:
                            ImGui::Text("%d", int(tiles(y,x).IsCoastTile()));
                            ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, tiles(y,x).IsCoastTile() ? clr2 : clr1);
                            break;
                        case 13:
                            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(style.CellPadding.x, 0));
                            if(ImGui::BeginTable("table2", 2, ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX)) {
                                ImGui::TableNextRow();
                                ImGui::TableNextColumn();
                                ImGui::Text("%d", tiles(y,x).info[0].untouchable);
                                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, tiles(y,x).info[0].untouchable ? clr2 : clr3);
                                ImGui::TableNextColumn();
                                ImGui::Text("%d", (int)tiles(y,x).rune_id);
                                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, tiles(y,x).rune_id ? clr2 : clr3);
                                ImGui::EndTable();
                            }
                            ImGui::PopStyleVar();
                            break;
                        case 14:
                            ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(style.CellPadding.x, 0));
                            if(ImGui::BeginTable("table2", 2, ImGuiTableFlags_ContextMenuInBody | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_NoHostExtendX)) {
                                ImGui::TableNextRow();
                                ImGui::TableNextColumn();
                                ImGui::Text("%d", tiles(y,x).info[0].invisible);
                                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, tiles(y,x).info[0].invisible ? clr2 : clr1);
                                ImGui::TableNextColumn();
                                ImGui::Text("%d", tiles(y,x).info[1].invisible);
                                ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, tiles(y,x).info[1].invisible ? clr2 : clr1);
                                ImGui::EndTable();
                            }
                            ImGui::PopStyleVar();
                            break;
                        default:
                            ImGui::Text("---");
                            break;
                    }
                    
                }
            }

            ImGui::EndTable();
        }

        ImGui::End();
#endif
    }

    void Map::DetectAffectedTiles(PaintBitmap& paint, int y, int x, std::vector<TileMod>& modified, int cornerType) {
        size_t start = modified.size();

        //add starting tile
        modified.push_back(TileMod::FromFloodFill(y, x, true));

        //flood fill, iterate until there's nothing to go through
        for(size_t i = start; i < modified.size(); i++) {
            int my = modified[i].y;
            int mx = modified[i].x;

            int& flag = paint(my, mx);
            TileData& td = at(my, mx);
            modified[i].prevCornerType = td.cornerType;

            // flag |= PaintFlags::DEBUG;

            if(!HAS_FLAG(flag, PaintFlags::VISITED)) {
                //tile that was directly marked as painted
                if(HAS_FLAG(flag, PaintFlags::MARKED_FOR_PAINT)) {
                    //add neighboring corners
                    if(my > 0) {
                        if(mx > 0)         modified.push_back(TileMod::FromFloodFill(my-1, mx-1, false));
                                           modified.push_back(TileMod::FromFloodFill(my-1, mx+0, false));
                        if(mx < Size().x)  modified.push_back(TileMod::FromFloodFill(my-1, mx+1, false));
                    }

                    if(mx > 0)             modified.push_back(TileMod::FromFloodFill(my+0, mx-1, false));
                    if(mx < Size().x)      modified.push_back(TileMod::FromFloodFill(my+0, mx+1, true));

                    if(my < Size().y) {
                        if(mx > 0)         modified.push_back(TileMod::FromFloodFill(my+1, mx-1, false));
                                           modified.push_back(TileMod::FromFloodFill(my+1, mx+0, true));
                        if(mx < Size().x)  modified.push_back(TileMod::FromFloodFill(my+1, mx+1, true));
                    }

                    modified[i].painted = true;
                }
            }

            //override tile's top-left corner value
            if(modified[i].painted || modified[i].writeCorner) {
                td.cornerType = cornerType;
            }

            flag |= PaintFlags::VISITED;
        }
    }
    
    int Map::ResolveCornerConflict(TileMod m, std::vector<TileMod>& modified, int dominantCornerType, int depth) {
        //boundary tile (used for corner value only), can skip resolution
        if(m.y >= Size().y || m.x >= Size().x) {
            //corner type already was changed from the tile that uses this tile's corner
            return dominantCornerType;
        }

        //this tile's corners
        glm::ivec4 corners = glm::ivec4(
            at(m.y+0, m.x+0).cornerType,
            at(m.y+0, m.x+1).cornerType,
            at(m.y+1, m.x+0).cornerType,
            at(m.y+1, m.x+1).cornerType
        );

        int newTileType, resolutionCornerType;
        bool hasValidCorners = CornersValidationCheck(corners, dominantCornerType, resolutionCornerType, newTileType);
        if(!hasValidCorners) {
            //mark neighboring tiles for corner conflict checkup too
            if(m.y > 0) {
                if(m.x > 0)         modified.push_back(TileMod::FromResolution(m.y-1, m.x-1, at(m.y-1, m.x-1).cornerType, resolutionCornerType, depth+1));
                                    modified.push_back(TileMod::FromResolution(m.y-1, m.x+0, at(m.y-1, m.x+0).cornerType, resolutionCornerType, depth+1));
                if(m.x < Size().x)  modified.push_back(TileMod::FromResolution(m.y-1, m.x+1, at(m.y-1, m.x+1).cornerType, resolutionCornerType, depth+1));
            }

            if(m.x > 0)             modified.push_back(TileMod::FromResolution(m.y+0, m.x-1, at(m.y+0, m.x-1).cornerType, resolutionCornerType, depth+1));
            if(m.x < Size().x)      modified.push_back(TileMod::FromResolution(m.y+0, m.x+1, at(m.y+0, m.x+1).cornerType, resolutionCornerType, depth+1));

            if(m.y < Size().y) {
                if(m.x > 0)         modified.push_back(TileMod::FromResolution(m.y+1, m.x-1, at(m.y+1, m.x-1).cornerType, resolutionCornerType, depth+1));
                                    modified.push_back(TileMod::FromResolution(m.y+1, m.x+0, at(m.y+1, m.x+0).cornerType, resolutionCornerType, depth+1));
                if(m.x < Size().x)  modified.push_back(TileMod::FromResolution(m.y+1, m.x+1, at(m.y+1, m.x+1).cornerType, resolutionCornerType, depth+1));
            }

            //invalid corner combination -> override corners that aren't dominant corner type with new, resolution type
            at(m.y+0, m.x+0).cornerType = (at(m.y+0, m.x+0).cornerType != dominantCornerType) ? resolutionCornerType : dominantCornerType;
            at(m.y+0, m.x+1).cornerType = (at(m.y+0, m.x+1).cornerType != dominantCornerType) ? resolutionCornerType : dominantCornerType;
            at(m.y+1, m.x+0).cornerType = (at(m.y+1, m.x+0).cornerType != dominantCornerType) ? resolutionCornerType : dominantCornerType;
            at(m.y+1, m.x+1).cornerType = (at(m.y+1, m.x+1).cornerType != dominantCornerType) ? resolutionCornerType : dominantCornerType;
        }
        return newTileType;
    }

    int Map::GetTransitionTileType(int cornerType1, int cornerType2) const {
        //lookup table, defines what corner type combinations are allowed for transition tiles
        static constexpr std::array<glm::ivec2, TRANSITION_TILE_OPTIONS> transition_corners = std::array<glm::ivec2, TRANSITION_TILE_OPTIONS> {
            glm::ivec2(TileType::GROUND1, TileType::GROUND2),
            glm::ivec2(TileType::GROUND1, TileType::MUD1),
            glm::ivec2(TileType::GROUND1, TileType::TREES),
            glm::ivec2(TileType::MUD1, TileType::MUD2),
            glm::ivec2(TileType::MUD1, TileType::ROCK),
            glm::ivec2(TileType::MUD1, TileType::WATER)
        };

        glm::ivec2 vec = glm::ivec2(std::min(cornerType1, cornerType2), std::max(cornerType1, cornerType2));
        auto pos = std::find(transition_corners.begin(), transition_corners.end(), vec);
        if(pos != transition_corners.end())
            return (pos - transition_corners.begin());
        else
            return -1;
    }

    bool Map::CornersValidationCheck(const glm::ivec4& c, int dominantCornerType, int& out_resolutionCornerType, int& out_newTileType) const {
        //defines what tileType to assign to the transition tile (indices match array in Map::GetTransitionTileType)
        static constexpr std::array<int, TRANSITION_TILE_OPTIONS> transition_types = {
            TileType::GROUND2,
            TileType::MUD1,
            TileType::TREES,
            TileType::MUD2,
            TileType::ROCK,
            TileType::WATER
        };

        //defines what corner type can input corner type transition to
        static constexpr int resolutionTypes[TileType::COUNT] = {
            TileType::MUD1,
            TileType::GROUND1,
            TileType::GROUND1,
            TileType::MUD1,
            TileType::GROUND1,
            TileType::MUD1,
            TileType::GROUND1,
            TileType::MUD1,
            TileType::MUD1,
            TileType::GROUND1,
            TileType::GROUND1,
            TileType::GROUND1,
            TileType::GROUND1,
            TileType::GROUND1,
        };

        //corner types checkup - 'i' indicates how many unique corner types does the tile have
        int i = 1;
        glm::ivec3 v = glm::ivec3(c[0], c[0], -1);
        if(i < 3 && c[1] != v[0] && c[1] != v[1]) v[i++] = c[1];
        if(i < 3 && c[2] != v[0] && c[2] != v[1]) v[i++] = c[2];
        if(i < 3 && c[3] != v[0] && c[3] != v[1]) v[i++] = c[3];
        
        switch(i) {
            case 1:         //all corners have the same type
                out_resolutionCornerType = dominantCornerType;
                out_newTileType = dominantCornerType;
                return true;
            case 2:         //exactly 2 corner types, need to check if the combination is allowed
            {
                out_resolutionCornerType = resolutionTypes[dominantCornerType];
                int idx = GetTransitionTileType(v[0], v[1]);
                if(idx >= 0) {
                    out_newTileType = transition_types[idx];
                    return true;
                }
                else {
                    idx = GetTransitionTileType(dominantCornerType, out_resolutionCornerType);
                    ASSERT_MSG(idx >= 0, "Corner type combination from resolutionTypes[] doesn't form a valid transition tile (transition_corners[] mismatch).");
                    out_newTileType = transition_types[idx];
                    return false;
                }
            }
            default:        //more than 2 corner types
            {
                out_resolutionCornerType = resolutionTypes[dominantCornerType];
                int idx = GetTransitionTileType(dominantCornerType, out_resolutionCornerType);
                ASSERT_MSG(idx >= 0, "Corner type combination from resolutionTypes[] doesn't form a valid transition tile (transition_corners[] mismatch).");
                out_newTileType = transition_types[idx];
                return false;
            }
        }
    }

    int Map::ResolveCornerType(int tileType) const {
        if((tileType >= TileType::WALL_BROKEN && tileType <= TileType::TREES_FELLED) || (tileType >= TileType::WALL_HU && tileType <= TileType::WALL_OC_DAMAGED))
            return (tileType != TileType::ROCK_BROKEN) ? TileType::GROUND1 : TileType::MUD1;
        else
            return tileType;
    }

    glm::ivec2 Map::MinDistanceNeighbor(const glm::ivec2& center, int step) {
        glm::ivec2 idx = center - step;
        float min_d = std::numeric_limits<float>::infinity();
        
        glm::ivec2 size = tiles.Size();
        for(int y = center.y-step; y <= center.y+step; y+=step) {
            if(y < 0 || y >= size.y)
                continue;
            
            for(int x = center.x-step; x <= center.x+step; x+=step) {
                if(x < 0 || x >= size.x)
                    continue;

                glm::ivec2 pos = glm::ivec2(x, y);
                float d = tiles(pos).nav.d;
                if(d <= min_d) {
                    idx = glm::ivec2(pos);
                    min_d = d;
                }
            }
        }

        ASSERT_MSG(IsWithinBounds(idx), "Map::MinDistanceNeighbor - out-of-bounds coordinates aren't allowed ({})", idx);
        return idx;
    }

    glm::ivec2 Map::MinDistanceNeighbor(const glm::ivec2& center, int nav_type, int step) {
        glm::ivec2 idx = center - step;
        float min_d = std::numeric_limits<float>::infinity();
        
        glm::ivec2 size = tiles.Size();
        for(int sy = -1; sy <= 1; sy++) {
            int y = center.y + sy*step;
            if(y < 0 || y >= size.y)
                continue;
            
            for(int sx = -1; sx <= 1; sx++) {
                int x = center.x + sx*step;
                if(x < 0 || x >= size.x)
                    continue;

                glm::ivec2 pos = glm::ivec2(x, y);
                float d = tiles(pos).nav.d;
                if(d <= min_d) {
                    bool path_valid = true;
                    if(step > 1) {
                        for(int i = 1; i < step; i++) {
                            if(!tiles(glm::ivec2(center.x + sx*i, center.y + sy*i)).Traversable(nav_type)) {
                                path_valid = false;
                                break;
                            }
                        }
                    }

                    if(path_valid) {
                        idx = glm::ivec2(pos);
                        min_d = d;
                    }
                }
            }
        }

        ASSERT_MSG(IsWithinBounds(idx), "Map::MinDistanceNeighbor - out-of-bounds coordinates aren't allowed ({})", idx);
        return idx;
    }

    glm::ivec2 Map::Pathfinding_RetrieveNextPos(const glm::ivec2& pos_src, const glm::ivec2& pos_dst_, int navType) {
        glm::ivec2 pos_dst = pos_dst_;
        glm::ivec2 dir = glm::sign(pos_dst - pos_src);
        int step = 1 + int(navType != NavigationBit::GROUND);

        char buf[256];
        std::stringstream ss;

        if(pos_src == pos_dst)
            return pos_dst;
        
        int j = 0;
        //when the target destination is unreachable - find new (reachable) location along the way from dst to src position
        while(tiles.IsWithinBounds(pos_dst) && (tiles(pos_dst).nav.d == std::numeric_limits<float>::infinity() || !tiles(pos_dst).TraversableOrForrest(navType)) && pos_dst != pos_src) {
            pos_dst -= dir * step;
            j++;
        }

        if(!tiles.IsWithinBounds(pos_dst)) {
            return pos_src;
        }

        glm::ivec2 res = pos_dst;
        glm::ivec2 pos = pos_dst;
        dir = pos_dst;
        
        int i = 0;
        while(pos != pos_src) {
            glm::ivec2 pos_prev = pos;
            tiles(pos_prev).nav.pathtile = true;
            
            //find neighboring tile in the direct neighborhood, that has the lowest distance from the starting location
            pos = MinDistanceNeighbor(pos, step);
            ASSERT_MSG(pos_prev != pos, "Map::Pathfinding - path retrieval is stuck.");
            ASSERT_MSG(tiles(pos).TraversableOrForrest(navType) || pos == pos_src, "Map::Pathfinding - path leads through untraversable tiles ({}).", pos);

            //direction change means corner in the path -> mark as new target position
            glm::ivec2 dir_new = pos_prev - pos;
            if(dir_new != dir) {
                res = pos_prev;
                
                snprintf(buf, sizeof(buf), "(%d, %d)<-", res.x, res.y);
                ss << buf;
            }
            // ENG_LOG_TRACE("{} | {} {} | {} {}", res, pos, pos_prev, dir, dir_new);
            dir = dir_new;
            i++;

            //previous tile was part of forrest (untraversable) -> mark as new target position
            if(tiles(pos_prev).nav.part_of_forrest) {
                res = pos;
            }
        }

        ENG_LOG_FINEST("        full path: {}\b\b  ({} tiles + {} unreachable)", ss.str().c_str(), i, j);
        ENG_LOG_FINER("    Map::Pathfinding::Result | from ({},{}) to ({},{}) | next=({},{}) | (D - next:{:.1f}, total:{:.1f})", pos_src.x, pos_src.y, pos_dst.x, pos_dst.y, res.x, res.y, tiles(res).nav.d, tiles(pos_dst).nav.d);
        return res;
    }

    glm::ivec2 Map::Pathfinding_RetrieveFinalPos(const glm::ivec2& pos_src, const glm::ivec2& pos_dst_, int navType) {
        glm::ivec2 pos_dst = pos_dst_;
        glm::ivec2 dir = glm::sign(pos_dst - pos_src);
        int step = 1 + int(navType != NavigationBit::GROUND);

        char buf[256];
        std::stringstream ss;

        if(pos_src == pos_dst)
            return pos_dst;
        
        int j = 0;
        //when the target destination is unreachable - find new (reachable) location along the way from dst to src position
        while(tiles.IsWithinBounds(pos_dst) && (tiles(pos_dst).nav.d == std::numeric_limits<float>::infinity() || !tiles(pos_dst).TraversableOrForrest(navType)) && pos_dst != pos_src) {
            pos_dst -= dir * step;
            j++;
        }

        return tiles.IsWithinBounds(pos_dst) ? pos_dst : pos_src;
    }

    TileData& Map::at(int y, int x) {
        return tiles(y, x);
    }

    const TileData& Map::at(int y, int x) const {
        return tiles(y, x);
    }

    void Map::DBG_PrintTiles() const {
        // printf("MAP | CORNERS:\n");
        // for(int y = 0; y <= size.y; y++) {
        //     for(int x = 0; x <= size.x; x++) {
        //         printf("%d ", at(size.y-y, x).tileType);
        //     }
        //     printf(" | ");
        //     for(int x = 0; x <= size.x; x++) {
        //         printf("%d ", at(size.y-y, x).cornerType);
        //     }
        //     printf("\n");
        // }
        // printf("-------\n\n");
    }

    void Map::DBG_PrintDistances() const {
        printf("DISTANCES:\n");
        for(int y = 0; y <= Size().y; y++) {
            for(int x = 0; x <= Size().x; x++) {
                printf("%5.1f ", at(Size().y-y, x).nav.d);
            }
            printf("\n");
        }
        printf("-------\n\n");
    }

    Sprite Map::GenOcclusionSprite() {
        int px = 64;
        TextureRef tex = TextureGenerator::GetTexture(TextureGenerator::Params::OcclusionTileset(px, 2));

        SpriteData data = SpriteData();
        data.size                   = glm::ivec2(px);
        data.offset                 = glm::ivec2(0);
        data.frames.line_count      = 2;
        data.frames.line_length     = 16;

        return Sprite(tex, data);
    }

    void Map::Move(Map&& m) noexcept {
        tileset = m.tileset;
        tiles = std::move(m.tiles);
        traversableObjects = std::move(m.traversableObjects);
        playerFactionId = m.playerFactionId;
        enable_occlusion = m.enable_occlusion;
        occlusion = m.occlusion;
        rune_dispatch = m.rune_dispatch;
        
        m.rune_dispatch = {};
        m.traversableObjects = {};
        m.tileset = nullptr;
    }

    //===================================================================================

    Tileset::Data ParseConfig_Tileset(const std::string& config_filepath, int flags) {
        using json = nlohmann::json;

        Tileset::Data data = {};
        data.name = GetFilename(config_filepath, true);

        //load the config and parse it as json file
        json config = json::parse(ReadFile(config_filepath.c_str()));

        try {
            data.type = config.at("type");

            //get texture path & load the texture
            std::string texture_filepath = config.at("texture_filepath");
            // TextureRef spritesheet = std::make_shared<Texture>(texture_filepath, flags);
            // TextureRef spritesheet = std::make_shared<Texture>(texture_filepath, TextureParams(GL_NEAREST, GL_CLAMP_TO_EDGE), flags);
            TextureRef spritesheet = Resources::LoadTexture(texture_filepath, TextureParams(GL_NEAREST, GL_CLAMP_TO_EDGE), flags);

            //parse other spritesheet data
            SpriteData spriteData = {};
            spriteData.name = "tilemap_" + data.name;

            spriteData.size = eng::json::parse_ivec2(config.at("tile_size"));
            spriteData.offset = glm::ivec2(0);

            glm::ivec2 tile_count = eng::json::parse_ivec2(config.at("tile_count"));
            spriteData.frames.line_length = tile_count.x;
            spriteData.frames.line_count = tile_count.y;

            if(config.count("offset")) {
                if(config.at("offset").size() > 1)
                    spriteData.frames.offset = eng::json::parse_ivec2(config.at("offset"));
                else
                    spriteData.frames.offset = glm::ivec2(config.at("offset"));
            }

            //assemble the tilemap sprite
            data.tilemap = Sprite(spritesheet, spriteData);

            //create tmp array with tile descriptions
            TileDescription* tiles = new TileDescription[tile_count.x * tile_count.y];

            //load tile types
            json& info = config.at("tile_types");
            for(int typeIdx = 0; typeIdx < std::min((int)info.size(), (int)(TileType::COUNT)); typeIdx++) {
                int count = 0;
                //each entry contains list of ranges
                auto& desc = info.at(typeIdx);
                for(auto& entry : desc) {
                    //each range marks tiles with this type
                    glm::ivec2 range = eng::json::parse_ivec2(entry);
                    count += range[1] - range[0];
                    for(int idx = range[0]; idx < range[1]; idx++) {
                        tiles[idx].tileType = typeIdx;
                    }
                }

                data.tiles[typeIdx].cornerless = (count == 1);
            }

            //load default tile indices
            if(config.count("default_tiles")) {
                json& def = config.at("default_tiles");
                for(auto& entry : def.items()) {
                    int tileType = std::stoi(entry.key());
                    int idx = entry.value();
                    int y = idx / tile_count.x;
                    int x = idx % tile_count.x;
                    data.tiles[tileType].defaultIdx = glm::ivec2(x, y);
                }
            }

            //load border types
            json& borders = config.at("border_types");
            for(auto& entry : borders.items()) {
                int borderType = std::stoi(entry.key());
                for(int idx : entry.value()) {
                    tiles[idx].borderType = borderType;
                }
            }

            //parse TileGraphics descriptions from tmp array
            for(int y = 0; y < tile_count.y; y++) {
                for(int x = 0; x < tile_count.x; x++) {
                    TileDescription& td = tiles[y*tile_count.x+x];
                    if(td.tileType >= 0)
                        data.tiles[td.tileType].Extend(x, y, td.borderType);
                }
            }

            for(int i = 0; i < data.tiles.size(); i++)
                data.tiles[i].type = i;

            delete[] tiles;

            //warning msgs, possibly malformed tileset description
            if(info.size() > data.tiles.size())
                ENG_LOG_WARN("Tileset '{}' has more tile definitions than there are tiletypes.", config_filepath);
            else if(info.size() < data.tiles.size())
                ENG_LOG_WARN("Tileset '{}' is missing some tile definitions.", config_filepath);
        }
        catch(json::exception& e) {
            ENG_LOG_WARN("Failed to parse '{}' config file - {}", config_filepath.c_str(), e.what());
            throw e;
        }

        return data;
    }

    int GetBorderType(int a_t, int a, int b, int c, int d) {
        int res = 0;

        /*a----b
          |    |
          c----d
        */

        //a,b <-> c,d due to vertical coords flip
        res |= int(a_t != c) << 0;
        res |= int(a_t != d) << 1;
        res |= int(a_t != a) << 2;
        res |= int(a_t != b) << 3;

        return res;
    }

    bool IsWallTile(int tileType) {
        return (tileType >= TileType::WALL_HU && tileType <= TileType::WALL_OC_DAMAGED);
    }

    bool IsMapObject(int tileType) {
        return (tileType >= TileType::ROCK);
    }

    float heuristic_euclidean(const glm::ivec2& src, const glm::ivec2& dst) {
        return glm::length(glm::vec2(dst - src));
    }

    float heuristic_diagonal(const glm::ivec2& src, const glm::ivec2& dst) {
        int dx = std::abs(dst.x - src.x);
        int dy = std::abs(dst.y - src.y);
        return 1*(dx+dy) + (1.141421f - 2.f) * std::min(dx, dy);
    }

    float range_heuristic(const glm::ivec2& src, const glm::ivec2& m, const glm::ivec2& M, heuristic_fn H) {
        return std::min({
            H(src, glm::vec2(m.x, m.y)),
            H(src, glm::vec2(m.x, M.y)),
            H(src, glm::vec2(M.x, m.y)),
            H(src, glm::vec2(M.x, M.y)),
        });
    }

    bool Pathfinding_AStar(MapTiles& tiles, pathfindingContainer& open, const glm::ivec2& pos_src_, const glm::ivec2& pos_dst, int navType, heuristic_fn H) {
        //airborne units only move on even tiles
        int step = 1 + int(navType != NavigationBit::GROUND);
        glm::ivec2 pos_src = (navType != NavigationBit::GROUND) ? make_even(pos_src_) : pos_src_;

        //prep distance values
        tiles.NavDataCleanup();
        tiles(pos_src).nav.d = 0.f;

        //reset the open set tracking
        open = {};
        open.emplace(H(pos_src, pos_dst), 0.f, pos_src);
        glm::ivec2 size = tiles.Size();

        int it = 0;
        bool found = false;
        while (open.size() > 0) {
            NavEntry entry = open.top();
            open.pop();
            TileData& td = tiles(entry.pos);

            //skip if (already visited) or (untraversable & not starting pos (that one's untraversable bcs unit's standing there))
            if(td.nav.visited || !(td.Traversable(navType) || entry.pos == pos_src))
                continue;
            
            it++;
            
            //mark as visited, set distance value
            ASSERT_MSG(fabsf(entry.d - td.nav.d) < 1e-5f, "Value here should already be a minimal distance ({} < {})", entry.d, td.nav.d);
            // if(entry.d > td.nav.d) ENG_LOG_WARN("HOPE IT'S JUST A ROUNDING ERROR (distance mismatch): {}, {}", entry.d, td.nav.d);
            td.nav.visited = true;
            td.nav.d = std::min(entry.d, td.nav.d);

            //path to the target destination found - terminate
            if(entry.pos == pos_dst) {
                found = true;
                break;
            }

            for(int y = -step; y <= step; y += step) {
                for(int x = -step; x <= step; x += step) {
                    glm::ivec2 pos = glm::ivec2(entry.pos.x+x, entry.pos.y+y);

                    //skip out of bounds or central tile
                    if(pos.y < 0 || pos.x < 0 || pos.y >= size.y || pos.x >= size.x || pos == entry.pos)
                        continue;
                    
                    TileData& td = tiles(pos);
                    float d = entry.d + (((std::abs(x+y)/step) % 2 == 0) ? 1.414213f : 1.f) * step;

                    //skip when (untraversable) or (already visited) or (marked as open & current distance is worse than existing distance)
                    if(!td.Traversable(navType) || (td.nav.visited && d > td.nav.d) || (!std::isinf(td.nav.d) && d > td.nav.d))
                        continue;

                    //tmp distance value (tile is still open)
                    td.nav.d = d;
                    
                    float h = H(pos, pos_dst);
                    open.emplace(h+d, d, pos);
                }
            }
        }

        ENG_LOG_FINEST("    Map::Pathfinding::Alg    | algorithm steps: {} (found: {})", it, found);
        return found;
    }

    bool Pathfinding_AStar_Range(MapTiles& tiles, pathfindingContainer& open, const glm::ivec2& pos_src_, int range, const glm::ivec2& m, const glm::ivec2& M, glm::ivec2* result_pos, int navType, heuristic_fn H) {
        //airborne units only move on even tiles
        int step = 1 + int(navType != NavigationBit::GROUND);
        glm::ivec2 pos_src = (navType != NavigationBit::GROUND) ? make_even(pos_src_) : pos_src_;

        //prep distance values
        tiles.NavDataCleanup();
        tiles(pos_src).nav.d = 0.f;

        //reset the open set tracking
        open = {};
        open.emplace(range_heuristic(pos_src, m, M, H), 0.f, pos_src);
        glm::ivec2 size = tiles.Size();

        int it = 0;
        bool found = false;
        while (open.size() > 0) {
            NavEntry entry = open.top();
            open.pop();
            TileData& td = tiles(entry.pos);

            //skip if (already visited) or (untraversable & not starting pos (that one's untraversable bcs unit's standing there))
            if(td.nav.visited || !(td.Traversable(navType) || entry.pos == pos_src))
                continue;
            
            it++;
            
            //mark as visited, set distance value
            ASSERT_MSG(fabsf(entry.d - td.nav.d) < 1e-5f, "Value here should already be a minimal distance ({} < {})", entry.d, td.nav.d);
            // if(entry.d > td.nav.d) ENG_LOG_WARN("HOPE IT'S JUST A ROUNDING ERROR (distance mismatch): {}, {}", entry.d, td.nav.d);
            td.nav.visited = true;
            td.nav.d = std::min(entry.d, td.nav.d);

            //path to the target destination found - terminate
            if(get_range(entry.pos, m, M) <= range) {
                //TODO: seems inefficient to compute this distance for every tile - maybe lazy compute them or somehow utilize heuristic for this?
                //TODO: also might want to search a bit more before terminating (first tile in range might not be the best one)
                *result_pos = entry.pos;
                found = true;
                break;
            }

            for(int y = -step; y <= step; y += step) {
                for(int x = -step; x <= step; x += step) {
                    glm::ivec2 pos = glm::ivec2(entry.pos.x+x, entry.pos.y+y);

                    //skip out of bounds or central tile
                    if(pos.y < 0 || pos.x < 0 || pos.y >= size.y || pos.x >= size.x || pos == entry.pos)
                        continue;
                    
                    TileData& td = tiles(pos);
                    float d = entry.d + (((std::abs(x+y)/step) % 2 == 0) ? 1.414213f : 1.f) * step;

                    //skip when (untraversable) or (already visited) or (marked as open & current distance is worse than existing distance)
                    if(!td.Traversable(navType) || (td.nav.visited && d > td.nav.d) || (!std::isinf(td.nav.d) && d > td.nav.d))
                        continue;

                    //tmp distance value (tile is still open)
                    td.nav.d = d;
                    
                    float h = range_heuristic(pos, m, M, H);
                    open.emplace(h+d, d, pos);
                }
            }
        }

        ENG_LOG_FINEST("    Map::Pathfinding::Alg    | algorithm steps: {} (found: {})", it, found);
        return found;
    }

    bool Pathfinding_AStar_Forrest(MapTiles& tiles, pathfindingContainer& open, const glm::ivec2& pos_src_, const glm::ivec2& pos_dst, int navType, heuristic_fn H) {
        //airborne units only move on even tiles
        int step = 1 + int(navType != NavigationBit::GROUND);
        glm::ivec2 pos_src = (navType != NavigationBit::GROUND) ? make_even(pos_src_) : pos_src_;

        //prep distance values
        tiles.NavDataCleanup();
        tiles(pos_src).nav.d = 0.f;

        //reset the open set tracking
        open = {};
        open.emplace(H(pos_src, pos_dst), 0.f, pos_src);
        glm::ivec2 size = tiles.Size();

        //fillout the entire forrest
        int forrest_size = tiles.NavData_Forrest_FloodFill(pos_dst);
        ENG_LOG_FINEST("    Map::Pathfinding::Alg    | forrest size = {}", forrest_size);
        if(forrest_size < 1) {
            return false;
        }

        int it = 0;
        bool found = false;
        while (open.size() > 0) {
            NavEntry entry = open.top();
            open.pop();
            TileData& td = tiles(entry.pos);

            //skip if (already visited) or (untraversable & not starting pos (that one's untraversable bcs unit's standing there))
            if(td.nav.visited || !(td.TraversableOrForrest(navType) || entry.pos == pos_src))
                continue;
            
            it++;
            
            //mark as visited, set distance value
            ASSERT_MSG(fabsf(entry.d - td.nav.d) < 1e-5f, "Value here should already be a minimal distance ({} < {})", entry.d, td.nav.d);
            // if(entry.d > td.nav.d) ENG_LOG_WARN("HOPE IT'S JUST A ROUNDING ERROR (distance mismatch): {}, {}", entry.d, td.nav.d);
            td.nav.visited = true;
            td.nav.d = std::min(entry.d, td.nav.d);

            //path to the target destination found - terminate
            if(entry.pos == pos_dst) {
                found = true;
                break;
            }

            for(int y = -step; y <= step; y += step) {
                for(int x = -step; x <= step; x += step) {
                    glm::ivec2 pos = glm::ivec2(entry.pos.x+x, entry.pos.y+y);

                    //skip out of bounds or central tile
                    if(pos.y < 0 || pos.x < 0 || pos.y >= size.y || pos.x >= size.x || pos == entry.pos)
                        continue;
                    
                    TileData& td = tiles(pos);
                    float d = entry.d + (((std::abs(x+y)/step) % 2 == 0) ? 1.414213f : 1.f) * step;

                    //penalize pathfinding through forrest in order to properly navigate to border forrest tiles
                    //higher the penalization value, the more likely will worker lookup closest accessible location to the selected tile
                    d += td.nav.part_of_forrest * 10.f;

                    //skip when (untraversable) or (already visited) or (marked as open & current distance is worse than existing distance)
                    if(!td.TraversableOrForrest(navType) || (td.nav.visited && d > td.nav.d) || (!std::isinf(td.nav.d) && d > td.nav.d))
                        continue;

                    //tmp distance value (tile is still open)
                    td.nav.d = d;
                    
                    float h = H(pos, pos_dst);
                    open.emplace(h+d, d, pos);
                }
            }
        }

        ENG_LOG_FINEST("    Map::Pathfinding::Alg    | algorithm steps: {} (found: {})", it, found);
        return found;
    }

    bool Pathfinding_Dijkstra_NearestBuilding(MapTiles& tiles, pathfindingContainer& open, const glm::ivec2& pos_src_, const std::vector<buildingMapCoords>& targets, int unit_resourceType, int navType, glm::ivec2& out_dst_pos) {
        //airborne units only move on even tiles
        int step = 1 + int(navType != NavigationBit::GROUND);
        glm::ivec2 pos_src = (navType != NavigationBit::GROUND) ? make_even(pos_src_) : pos_src_;

        //prep distance values
        tiles.NavDataCleanup();
        tiles(pos_src).nav.d = 0.f;

        //reset the open set tracking
        open = {};
        open.emplace(0.f, 0.f, pos_src);
        glm::ivec2 size = tiles.Size();

        //mark all the buildings in the tile data
        for(auto& [m,M,ID,resType] : targets) {
            if(!HAS_FLAG(resType, unit_resourceType))
                continue;

            for(int y = m.y; y <= M.y; y++) {
                for(int x = m.x; x <= M.x; x++) {
                    tiles(y,x).nav.part_of_forrest = true;
                }
            }
        }

        int it = 0;
        bool found = false;
        while (open.size() > 0) {
            NavEntry entry = open.top();
            open.pop();
            TileData& td = tiles(entry.pos);

            //skip if (already visited) or (untraversable & not starting pos (that one's untraversable bcs unit's standing there))
            if(td.nav.visited || !(td.TraversableOrForrest(navType) || entry.pos == pos_src))
                continue;
            
            it++;
            
            //mark as visited, set distance value
            ASSERT_MSG(fabsf(entry.d - td.nav.d) < 1e-5f, "Value here should already be a minimal distance ({} < {})", entry.d, td.nav.d);
            // if(entry.d > td.nav.d) ENG_LOG_WARN("HOPE IT'S JUST A ROUNDING ERROR (distance mismatch): {}, {}", entry.d, td.nav.d);
            td.nav.visited = true;
            td.nav.d = std::min(entry.d, td.nav.d);

            //path to the target destination found - terminate
            if(td.nav.part_of_forrest) {
                out_dst_pos = entry.pos;
                found = true;
                break;
            }

            for(int y = -step; y <= step; y += step) {
                for(int x = -step; x <= step; x += step) {
                    glm::ivec2 pos = glm::ivec2(entry.pos.x+x, entry.pos.y+y);

                    //skip out of bounds or central tile
                    if(pos.y < 0 || pos.x < 0 || pos.y >= size.y || pos.x >= size.x || pos == entry.pos)
                        continue;
                    
                    TileData& td = tiles(pos);
                    float d = entry.d + (((std::abs(x+y)/step) % 2 == 0) ? 1.414213f : 1.f) * step;

                    //skip when (untraversable) or (already visited) or (marked as open & current distance is worse than existing distance)
                    if(!td.TraversableOrForrest(navType) || (td.nav.visited && d > td.nav.d) || (!std::isinf(td.nav.d) && d > td.nav.d))
                        continue;

                    //tmp distance value (tile is still open)
                    td.nav.d = d;
                    
                    open.emplace(d, d, pos);
                }
            }
        }

        ENG_LOG_FINEST("    Map::Pathfinding::Alg    | algorithm steps: {} (found: {})", it, found);
        return found;
    }

}//namespace eng
