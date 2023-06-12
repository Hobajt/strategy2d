#include "engine/game/map.h"

#include "engine/game/resources.h"
#include "engine/game/camera.h"

#include "engine/utils/utils.h"
#include "engine/utils/json.h"

#include <random>

namespace eng {

    Tileset::Data ParseConfig_Tileset(const std::string& config_filepath, int flags);

    int c2i(const glm::ivec2& size, int y, int x);
    int c2i_b(const glm::ivec2& size, int y, int x);

    int GetBorderType(TileData* tiles, const glm::ivec2& size, int y, int x);
    int GetBorderType_b(TileData* tiles, const glm::ivec2& size, int y, int x);

    //an element of tmp array, used during tileset parsing
    struct TileDescription {
        int tileType = -1;
        std::vector<int> borderTypes;
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

        throw std::runtime_error("");
    }

    void TileGraphics::Extend(int x, int y, const std::vector<int>& borderTypes) {
        glm::ivec2 idx = glm::ivec2(x, y);
        for(int bt : borderTypes) {
            mapping[bt].push_back(idx);
        }
    }

    //===== Tileset =====

    Tileset::Tileset(const std::string& config_filepath, int flags) {
        data = ParseConfig_Tileset(config_filepath, flags);
    }

    void Tileset::UpdateTileIndices(TileData* tiles, const glm::ivec2& size) const {
        for(int y = 1; y < size.y-1; y++) {
            for(int x = 1; x < size.x-1; x++) {
                TileData& tile = tiles[y*size.x+x];
                int border = GetBorderType(tiles, size, y, x);

                //resolve border
                // if(border != ((1<<8)-1)) {
                //     if(data.tileDesc[tile.type].borders.count(border)) {
                //         //tile type has given border type visuals
                //         tile.idx = data.tileDesc[tile.type].borders.at(border);
                //         continue;
                //     }
                // }
                // tile.idx = data.tiles[tile.type].GetTileIdx(borderType, tile.variation);
            }
        }

        //TODO: do the same for tiles on map borders
        // for(int x = 1; x < size.x-1; x++) {
        //     int border;

        //     border = GetBorderType_b(tiles, size, 0, x);
        //     //...

        //     border = GetBorderType_b(tiles, size, size.y-1, x);
        //     //...
        // }
    }

    void Tileset::UpdateTileIndices(TileData* tiles, const glm::ivec2& size, int y, int x) const {
        //TODO: update tile idx & it's neighbors

        int i = y*size.x + x;
        // tiles[i].idx = data.tileDesc[tiles[i].type].idx;
    }

    glm::ivec2 Tileset::GetTileIdx(int tileType, int borderType, int variation) {
        ASSERT_MSG((unsigned int(tileType) < TileType::COUNT), "Attempting to use invalid tile type.");
        return data.tiles.at(tileType).GetTileIdx(borderType, variation);
    }

    //===== Map =====

    Map::Map(const glm::ivec2& size_, const TilesetRef& tileset_) : size(size_) {
        tiles = new TileData[size.x * size.y];
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

    int& Map::operator[](int i) {
        ASSERT_MSG(tiles != nullptr, "Map isn't properly initialized!");
        ASSERT_MSG((unsigned(i) < unsigned(Area())), "Array index out of bounds.");
        return tiles[i].type;    
    }

    const int& Map::operator[](int i) const {
        ASSERT_MSG(tiles != nullptr, "Map isn't properly initialized!");
        ASSERT_MSG((unsigned(i) < unsigned(Area())), "Array index out of bounds (max: {}, idx: {})", Area()-1, i);
        return tiles[i].type;
    }

    int& Map::operator()(int y, int x) {
        return operator[](y * size.x + x);
    }

    const int& Map::operator()(int y, int x) const {
        return operator[](y * size.x + x);
    }

    void Map::Render() {
        Camera& cam = Camera::Get();

        for(int y = 0; y < size.y; y++) {
            for(int x = 0; x < size.x; x++) {
                int i = coord2idx(y, x);
                glm::vec3 pos = glm::vec3(cam.map2screen(glm::vec2(x, y)), 0.f);
                tileset->Tilemap().Render(glm::uvec4(0), pos, cam.Mult(), tiles[i].idx.y, tiles[i].idx.x);
            }
        }
    }

    void Map::RenderRange(int xs, int ys, int w, int h) {
        Camera& cam = Camera::Get();

        for(int y = ys; y < std::min(size.y, h); y++) {
            for(int x = xs; x < std::min(size.x, w); x++) {
                int i = coord2idx(y, x);
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

    Map Map::Clone() const {
        return Map(*this);
    }

    void Map::ModifyTile(int y, int x, int type, int variation, bool update_indices) {
        TileData& tile = tiles[y*size.x + x];
        if(type != tile.type) {
            tile.type = type;
        }
        tile.variation = variation;

        if(update_indices) {
            UpdateTileIndices(y, x);
        }
    }

    void Map::ModifyTiles(bool* bitmap, int type, bool randomVariation, int variation) {
        std::mt19937 gen = std::mt19937(std::random_device{}());
        std::uniform_int_distribution<int> dist = std::uniform_int_distribution(100);

        for(int y = 0; y < size.y; y++) {
            for(int x = 0; x < size.x; x++) {
                if(bitmap[y*size.x+x]) {
                    TileData& tile = tiles[y*size.x + x];
                    if(type != tile.type) {
                        tile.type = type;
                    }
                    int var = randomVariation ? dist(gen) : variation;
                    tile.variation = variation;
                }
                bitmap[y*size.x+x] = false;
            }
        }

        UpdateTileIndices();
    }

    void Map::UpdateTileIndices() {
        tileset->UpdateTileIndices(tiles, size);
    }

    void Map::UpdateTileIndices(int y, int x) {
        tileset->UpdateTileIndices(tiles, size, y, x);
    }

    Map::Map(const Map& m) noexcept : size(m.size), tileset(m.tileset) {
        tiles = new TileData[size.x * size.y];
        for(int i = 0; i < size.x * size.y; i++)
            tiles[i] = m.tiles[i];
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
                    tiles[idx].borderTypes.push_back(borderType);
                }
            }

            //parse TileGraphics descriptions from tmp array
            for(int y = 0; y < tile_count.y; y++) {
                for(int x = 0; x < tile_count.x; x++) {
                    TileDescription& td = tiles[y*tile_count.x+x];
                    if(td.tileType >= 0)
                        data.tiles[td.tileType].Extend(x, y, td.borderTypes);
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

    int c2i(const glm::ivec2& size, int y, int x) {
        return y*size.x + x;
    }

    int c2i_b(const glm::ivec2& size, int y, int x) {
        return std::clamp(y, 0, size.y)*size.x + std::clamp(x, 0, size.x);
    }

    int GetBorderType(TileData* tiles, const glm::ivec2& size, int y, int x) {
        int res = 0;
        int type = tiles[c2i(size, y, x)].type;

        res |= int(tiles[c2i(size, y-1, x-1)].type == type) << 0;
        res |= int(tiles[c2i(size, y-1, x-0)].type == type) << 1;
        res |= int(tiles[c2i(size, y-1, x+1)].type == type) << 2;

        res |= int(tiles[c2i(size, y-0, x-1)].type == type) << 3;
        res |= int(tiles[c2i(size, y-0, x+1)].type == type) << 4;

        res |= int(tiles[c2i(size, y+1, x-1)].type == type) << 5;
        res |= int(tiles[c2i(size, y+1, x-0)].type == type) << 6;
        res |= int(tiles[c2i(size, y+1, x+1)].type == type) << 7;

        return res;
    }

    int GetBorderType_b(TileData* tiles, const glm::ivec2& size, int y, int x) {
        int res = 0;
        int type = tiles[c2i(size, y, x)].type;

        res |= int(tiles[c2i_b(size, y-1, x-1)].type == type) << 0;
        res |= int(tiles[c2i_b(size, y-1, x-0)].type == type) << 1;
        res |= int(tiles[c2i_b(size, y-1, x+1)].type == type) << 2;

        res |= int(tiles[c2i_b(size, y-0, x-1)].type == type) << 3;
        res |= int(tiles[c2i_b(size, y-0, x+1)].type == type) << 4;

        res |= int(tiles[c2i_b(size, y+1, x-1)].type == type) << 5;
        res |= int(tiles[c2i_b(size, y+1, x-0)].type == type) << 6;
        res |= int(tiles[c2i_b(size, y+1, x+1)].type == type) << 7;

        return res;
    }

}//namespace eng
