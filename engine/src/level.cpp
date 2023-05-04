#include "engine/game/level.h"

#include "engine/game/resources.h"
#include "engine/game/camera.h"

#include "engine/core/renderer.h"

namespace eng {

    //===== MapTiles =====

    MapTiles::MapTiles(const glm::ivec2& size, const SpritesheetRef& tileset) {
        tiles = new Tile[size.x * size.y];
        ChangeTileset(tileset);
    }

    MapTiles::~MapTiles() {
        delete[] tiles;
    }

    int& MapTiles::operator[](int i) {
        ASSERT_MSG(tiles != nullptr, "MapTiles were not properly initialized!");
        ASSERT_MSG((unsigned(i) < unsigned(Area())), "Array index out of bounds.");
        return tiles[i].id;    
    }

    const int& MapTiles::operator[](int i) const {
        ASSERT_MSG(tiles != nullptr, "MapTiles were not properly initialized!");
        ASSERT_MSG((unsigned(i) < unsigned(Area())), "Array index out of bounds (max: {}, idx: {})", Area()-1, i);
        return tiles[i].id;
    }

    int& MapTiles::operator()(int y, int x) {
        return operator[](y * size.x + x);
    }

    const int& MapTiles::operator()(int y, int x) const {
        return operator[](y * size.x + x);
    }

    void MapTiles::Render() {
        Camera& cam = Camera::Get();

        for(int y = 0; y < size.y; y++) {
            for(int x = 0; x < size.x; x++) {
                // Renderer::RenderQuad(Quad::FromCenter());
            }
        }
    }

    void MapTiles::RenderRange() {}

    void MapTiles::ChangeTileset(const SpritesheetRef& tileset_) {
        // if(tileset_ == nullptr) {
        //     tileset = Resources::DefaultSpritesheet();
        // }
        
        //update tileset ptr
        //update tileSprites vector
        //update ptrs from tiles[] to tileSprites vector (based on tile ID)
    }

    //===== Map =====

    Map::Map(const glm::vec2& mapSize, const SpritesheetRef& tileset) : tiles(MapTiles(mapSize, tileset)) {}

    void Map::ChangeTileset(const SpritesheetRef& tileset) {
        tiles.ChangeTileset(tileset);
    }

    void Map::Render() {}

    void Map::RenderRange() {}

    //===== Level =====

    Level::Level() : map(Map(glm::ivec2(0,0), nullptr)) {}

    Level::Level(const glm::vec2& mapSize, const SpritesheetRef& tileset) : map(Map(mapSize, tileset)) {}

    // Level::Level(const Savefile& savefile) {}

}//namespace eng
