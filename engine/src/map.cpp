#include "engine/game/map.h"

#include "engine/game/resources.h"
#include "engine/game/camera.h"

#include "engine/utils/utils.h"
#include "engine/utils/json.h"

namespace eng {

    Tileset::Data ParseConfig_Tileset(const std::string& config_filepath, int flags);

    //===== Tileset =====

    Tileset::Tileset(const std::string& config_filepath, int flags) {
        data = ParseConfig_Tileset(config_filepath, flags);
    }

    void Tileset::UpdateTileIndices(TileData* tiles, int count) {
        //TODO:
        for(int i = 0; i < count; i++) {
            // tiles[i].idx = glm::ivec2(0, 0);
            tiles[i].idx = data.tileDesc[3].idx;
        }
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
            tileset->UpdateTileIndices(tiles, Area());
        }
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

            //load tile descriptions
            //TODO: rework to also include tile transitions, etc.
            json info = config.at("tile_info");
            for(int i = 0; i < std::min(info.size(), data.tileDesc.size()); i++) {
                data.tileDesc[i] = Tileset::TileDescription{ eng::json::parse_ivec2(info.at(i)) };
            }
            if(info.size() > data.tileDesc.size()) {
                ENG_LOG_DEBUG("Tileset '{}' has more tile definitions than there are tiletypes.", config_filepath);
            }
            else if(info.size() < data.tileDesc.size()) {
                ENG_LOG_WARN("Tileset '{}' is missing some tile definitions.", config_filepath);
            }
        }
        catch(json::exception& e) {
            ENG_LOG_WARN("Failed to parse '{}' config file - {}", config_filepath.c_str(), e.what());
            throw e;
        }

        return data;
    }

}//namespace eng
