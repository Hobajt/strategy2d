#include "engine/game/map.h"

#include "engine/game/resources.h"
#include "engine/game/camera.h"

#include "engine/utils/utils.h"
#include "engine/utils/json.h"

#include <random>

namespace eng {

    Tileset::Data ParseConfig_Tileset(const std::string& config_filepath, int flags);

    //Generates tile's border type from corner tile types.
    int GetBorderType(int t_a, int a, int b, int c, int d);

    bool IsWallTile(int tileType);

    //an element of tmp array, used during tileset parsing
    struct TileDescription {
        int tileType = -1;
        int borderType = 0;
    };

    //===== TileGraphics =====

    glm::ivec2 TileGraphics::GetTileIdx(int borderType, int variation) const {
        if(mapping.count(borderType)) {
            const std::vector<glm::ivec2>& indices = mapping.at(borderType);
            return indices.at(std::min((size_t)variation, indices.size()-1));
        }

        //TODO: what to do if given border type isn't specified?
        //  - can pick closest existing borderType (based on hamming distance)
        //  - can return some default index (maybe insert magenta tile at (0,0)??)
        //  - or???

        ENG_LOG_ERROR("Missing tileset index for borderType={}", borderType);
        // throw std::runtime_error("");
        return glm::ivec2(0, 0);
    }

    void TileGraphics::Extend(int x, int y, int borderType) {
        glm::ivec2 idx = glm::ivec2(x, y);
        mapping[borderType].push_back(idx);
    }

    //===== Tileset =====

    Tileset::Tileset(const std::string& config_filepath, int flags) {
        data = ParseConfig_Tileset(config_filepath, flags);
    }

    void Tileset::UpdateTileIndices(TileData* tiles, const glm::ivec2& size) const {
        for(int y = 0; y < size.y; y++) {
            //adressing 4 tiles at a time to retrieve all cornerTypes for tile 'a'
            TileData *a, *b, *c, *d;
            a = &tiles[c2i(y+0, 0, size)];
            c = &tiles[c2i(y+1, 0, size)];

            for(int x = 0; x < size.x; x++) {
                b = &tiles[c2i(y+0, x+1, size)];
                d = &tiles[c2i(y+1, x+1, size)];

                if(!IsWallTile(a->tileType)) {
                    //not a wall tile -> pick index based on corners
                    int borderType = GetBorderType(a->tileType, a->cornerType, b->cornerType, c->cornerType, d->cornerType);
                    a->idx = data.tiles[a->tileType].GetTileIdx(borderType, a->variation);
                }
                else {
                    //wall tile -> pick index based on neighboring tiles
                    //TODO:
                    ENG_LOG_ERROR("NOT IMPLEMENTED!!!");
                    throw std::runtime_error("");
                }

                
                a = b;
                c = d;
            }
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

    Map::Map(const glm::ivec2& size_, const TilesetRef& tileset_) : size(size_) {
        tiles = new TileData[(size.x+1) * (size.y+1)];
        ChangeTileset(tileset_);
    }

    Map::~Map() {
        Release();
    }

    Map::Map(Map&& m) noexcept {
        Move(std::move(m));
    }

    Map& Map::operator=(Map&& m) noexcept {
        Release();
        Move(std::move(m));
        return *this;
    }

    void Map::Move(Map&& m) noexcept {
        size = m.size;
        tiles = m.tiles;
        tileset = m.tileset;
        
        m.tiles = nullptr;
        m.tileset = nullptr;
    }
    void Map::Release() noexcept {
        delete[] tiles;
    }

    int& Map::operator()(int y, int x) {
        //.at() does similar assert, but it also allows access to [size.y, size.x], which isn't a valid tile (it's there for corner value)
        ASSERT_MSG((unsigned(y) < unsigned(size.y)) || (unsigned(x) < unsigned(size.x)), "Array index out of bounds.");
        return at(y,x).tileType;
    }

    const int& Map::operator()(int y, int x) const {
        //.at() does similar assert, but it also allows access to [size.y, size.x], which isn't a valid tile (it's there for corner value)
        ASSERT_MSG((unsigned(y) < unsigned(size.y)) || (unsigned(x) < unsigned(size.x)), "Array index out of bounds.");
        return at(y,x).tileType;
    }

    void Map::Render() {
        Camera& cam = Camera::Get();

        for(int y = 0; y < size.y; y++) {
            for(int x = 0; x < size.x; x++) {
                int i = c2i(y, x);
                glm::vec3 pos = glm::vec3(cam.map2screen(glm::vec2(x, y)), 0.f);
                tileset->Tilemap().Render(glm::uvec4(0), pos, cam.Mult(), tiles[i].idx.y, tiles[i].idx.x);
            }
        }
    }

    void Map::RenderRange(int xs, int ys, int w, int h) {
        Camera& cam = Camera::Get();

        for(int y = ys; y < std::min(size.y, h); y++) {
            for(int x = xs; x < std::min(size.x, w); x++) {
                int i = c2i(y, x);
                glm::vec3 pos = glm::vec3((glm::vec2(x, y) - 0.5f - cam.Position()) * cam.Mult(), 0.f);
                tileset->Tilemap().Render(glm::uvec4(0), pos, cam.Mult(), tiles[i].idx.y, tiles[i].idx.x);
            }
        }
    }

    void Map::ChangeTileset(const TilesetRef& tilesetNew) {
        TilesetRef ts = (tilesetNew != nullptr) ? tilesetNew : Resources::DefaultTileset();
        if(tileset != tilesetNew) {
            tileset = tilesetNew;
            tileset->UpdateTileIndices(tiles, size);
        }
    }

    void Map::UndoChanges(std::vector<TileRecord>& history, bool rewrite_history) {
        //TODO:
    }

    void Map::ModifyTiles(PaintBitmap& paint, int tileType, bool randomVariation, int variationValue, std::vector<TileRecord>* history) {
        ASSERT_MSG(paint.Size() == size, "PaintBitmap size doesn't match the map size!");

        std::mt19937 gen = std::mt19937(std::random_device{}());
        std::uniform_int_distribution<int> dist = std::uniform_int_distribution(100);

        if(history != nullptr)
            history->clear();

        int affectedCount = 0;
        std::vector<TileMod> modified;

        //the same as tileTpye, except for walls (walls use grass)
        int cornerType = ResolveCornerType(tileType);

        //iterate through the paint and retrieve all marked tiles & their direct neighbors
        for(int y = 0; y < size.y; y++) {
            for(int x = 0; x < size.x; x++) {
                int& flag = paint(y, x);

                if(HAS_FLAG(flag, PaintFlags::MARKED_FOR_PAINT) && !HAS_FLAG(flag, PaintFlags::VISITED)) {
                    ApplyPaint_FloodFill(paint, y, x, modified, cornerType);
                }
            }
        }

        //iterate through the list of affected tiles
        for(size_t i = 0; i < modified.size(); i++) {
            TileMod& m = modified[i];
            TileData& td = at(m.y, m.x);

            //skip entries with tiles that are already marked as completed
            if(HAS_FLAG(paint(m.y, m.x), PaintFlags::FINALIZED))
                continue;
            paint(m.y, m.x) |= PaintFlags::FINALIZED;

            affectedCount++;

            //non-zero depth means that entry was generated through conflict resolution
            int cType = (m.depth == 0) ? cornerType : m.dominantCornerType;

            //write into history
            if(history != nullptr) {
                history->push_back(TileRecord(td, m.prevCornerType, m.y, m.x));
            }

            //tile type resolution
            if(m.painted) {
                //directly marked as painted, can just override values
                td.tileType = tileType;
                td.variation = randomVariation ? dist(gen) : variationValue;
            }
            else {
                // paint(m.y, m.x) |= PaintFlags::DEBUG;

                //border tile, may have been indirectly modified
                ResolveCornerConflict(m, modified, cType, m.depth);
            }
        }

        //TODO: update tile types (include walls handling)

        tileset->UpdateTileIndices(tiles, size);

        ENG_LOG_TRACE("Map::ModifyTiles - number of affected tiles = {} ({})", affectedCount, modified.size());
    }

    void Map::ApplyPaint_FloodFill(PaintBitmap& paint, int y, int x, std::vector<TileMod>& modified, int cornerType) {
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
                        if(mx > 0)       modified.push_back(TileMod::FromFloodFill(my-1, mx-1, false));
                                         modified.push_back(TileMod::FromFloodFill(my-1, mx+0, false));
                        if(mx < size.x)  modified.push_back(TileMod::FromFloodFill(my-1, mx+1, false));
                    }

                    if(mx > 0)           modified.push_back(TileMod::FromFloodFill(my+0, mx-1, false));
                    if(mx < size.x)      modified.push_back(TileMod::FromFloodFill(my+0, mx+1, true));

                    if(my < size.y) {
                        if(mx > 0)       modified.push_back(TileMod::FromFloodFill(my+1, mx-1, false));
                                         modified.push_back(TileMod::FromFloodFill(my+1, mx+0, true));
                        if(mx < size.x)  modified.push_back(TileMod::FromFloodFill(my+1, mx+1, true));
                    }

                    modified[i].painted = true;
                }

                //override tile's top-left corner value
                if(modified[i].painted || modified[i].writeCorner) {
                    td.cornerType = cornerType;
                }
            }
            flag |= PaintFlags::VISITED;
        }
    }
    
    void Map::ResolveCornerConflict(TileMod m, std::vector<TileMod>& modified, int cornerType, int depth) {
        static int resolutionTypes[TileType::COUNT] = {
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

        //boundary tile (used for corner value only), can skip resolution
        if(m.y >= size.y || m.x >= size.x) {
            at(m.y, m.x).cornerType = (at(m.y, m.x).cornerType != cornerType) ? resolutionTypes[cornerType] : cornerType;
            return;
        }

        //this tile's corners
        glm::ivec4 corners = glm::ivec4(
            at(m.y+0, m.x+0).cornerType,
            at(m.y+0, m.x+1).cornerType,
            at(m.y+1, m.x+0).cornerType,
            at(m.y+1, m.x+1).cornerType
        );

        if(!HasValidCorners(corners)) {
            int resType = resolutionTypes[cornerType];
            at(m.y+0, m.x+0).cornerType = (at(m.y+0, m.x+0).cornerType != cornerType) ? resType : cornerType;
            at(m.y+0, m.x+1).cornerType = (at(m.y+0, m.x+1).cornerType != cornerType) ? resType : cornerType;
            at(m.y+1, m.x+0).cornerType = (at(m.y+1, m.x+0).cornerType != cornerType) ? resType : cornerType;
            at(m.y+1, m.x+1).cornerType = (at(m.y+1, m.x+1).cornerType != cornerType) ? resType : cornerType;

            //mark neighbors for corner conflict checkup too
            if(m.y > 0) {
                if(m.x > 0)       modified.push_back(TileMod::FromResolution(m.y-1, m.x-1, at(m.y-1, m.x-1).cornerType, resType, depth+1));
                                  modified.push_back(TileMod::FromResolution(m.y-1, m.x+0, at(m.y-1, m.x+0).cornerType, resType, depth+1));
                if(m.x < size.x)  modified.push_back(TileMod::FromResolution(m.y-1, m.x+1, at(m.y-1, m.x+1).cornerType, resType, depth+1));
            }

            if(m.x > 0)           modified.push_back(TileMod::FromResolution(m.y+0, m.x-1, at(m.y+0, m.x-1).cornerType, resType, depth+1));
            if(m.x < size.x)      modified.push_back(TileMod::FromResolution(m.y+0, m.x+1, at(m.y+0, m.x+1).cornerType, resType, depth+1));

            if(m.y < size.y) {
                if(m.x > 0)       modified.push_back(TileMod::FromResolution(m.y+1, m.x-1, at(m.y+1, m.x-1).cornerType, resType, depth+1));
                                  modified.push_back(TileMod::FromResolution(m.y+1, m.x+0, at(m.y+1, m.x+0).cornerType, resType, depth+1));
                if(m.x < size.x)  modified.push_back(TileMod::FromResolution(m.y+1, m.x+1, at(m.y+1, m.x+1).cornerType, resType, depth+1));
            }
        }
    }

    bool Map::HasValidCorners(const glm::ivec4& c) const {
        static std::vector<glm::ivec2> allowed_corners = std::vector<glm::ivec2> {
            glm::ivec2(TileType::GROUND1, TileType::GROUND2),
            glm::ivec2(TileType::GROUND1, TileType::MUD1),
            glm::ivec2(TileType::GROUND1, TileType::TREES),
            glm::ivec2(TileType::MUD1, TileType::MUD2),
            glm::ivec2(TileType::MUD1, TileType::ROCK),
            glm::ivec2(TileType::MUD1, TileType::WATER)
        };

        int i = 1;
        glm::ivec3 v = glm::ivec3(c[0], c[0], -1);
        if(i < 3 && c[1] != v[0] && c[1] != v[1]) v[i++] = c[1];
        if(i < 3 && c[2] != v[0] && c[2] != v[1]) v[i++] = c[2];
        if(i < 3 && c[3] != v[0] && c[3] != v[1]) v[i++] = c[3];
        
        switch(i) {
            case 1:
                //all corners have the same type
                return true;
            case 2:
            {
                //exactly 2 corner types, need to check if the combination is allowed
                glm::ivec2 vec = glm::ivec2(std::min(v[0], v[1]), std::max(v[0], v[1]));
                return (std::find(allowed_corners.begin(), allowed_corners.end(), vec) != allowed_corners.end());
            }
            default:
                //more than 2 corner types
                return false;
        }
    }

    int Map::ResolveCornerType(int tileType) const {
        if((tileType >= TileType::WALL_BROKEN && tileType <= TileType::TREES_FELLED) || (tileType >= TileType::WALL_HU && tileType <= TileType::WALL_OC_DAMAGED))
            return TileType::GROUND1;
        else
            return tileType;
    }

    TileData& Map::at(int y, int x) {
        ASSERT_MSG(tiles != nullptr, "Map isn't properly initialized!");
        ASSERT_MSG(((unsigned(y) <= unsigned(size.y)) && (unsigned(x) <= unsigned(size.x))), "Array index ({}, {}) is out of bounds.", y, x);
        return tiles[y*(size.x+1)+x];
    }

    const TileData& Map::at(int y, int x) const {
        ASSERT_MSG(tiles != nullptr, "Map isn't properly initialized!");
        ASSERT_MSG(((unsigned(y) <= unsigned(size.y)) && (unsigned(x) <= unsigned(size.x))), "Array index ({}, {}) is out of bounds.", y, x);
        return tiles[y*(size.x+1)+x];
    }

    //===================================================================================

    Tileset::Data ParseConfig_Tileset(const std::string& config_filepath, int flags) {
        using json = nlohmann::json;

        Tileset::Data data = {};
        data.name = GetFilename(config_filepath, true);

        //load the config and parse it as json file
        json config = json::parse(ReadFile(config_filepath.c_str()));

        try {
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

            if(config.count("offset"))
                spriteData.frames.offset = config.at("offset");

            //assemble the tilemap sprite
            data.tilemap = Sprite(spritesheet, spriteData);

            //create tmp array with tile descriptions
            TileDescription* tiles = new TileDescription[tile_count.x * tile_count.y];

            //load tile types
            json& info = config.at("tile_types");
            for(int typeIdx = 0; typeIdx < std::min((int)info.size(), (int)(TileType::COUNT)); typeIdx++) {
                //each entry contains list of ranges
                auto& desc = info.at(typeIdx);
                for(auto& entry : desc) {
                    //each range marks tiles with this type
                    glm::ivec2 range = eng::json::parse_ivec2(entry);
                    for(int idx = range[0]; idx < range[1]; idx++) {
                        tiles[idx].tileType = typeIdx;
                    }
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

        res |= int(a_t != a) << 0;
        res |= int(a_t != b) << 1;
        res |= int(a_t != c) << 2;
        res |= int(a_t != d) << 3;

        return res;
    }

    bool IsWallTile(int tileType) {
        return (tileType >= TileType::WALL_HU && tileType <= TileType::WALL_OC_DAMAGED);
    }

}//namespace eng
