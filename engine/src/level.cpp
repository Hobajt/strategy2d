#include "engine/game/level.h"

#include "engine/game/resources.h"
#include "engine/game/camera.h"

#include "engine/core/renderer.h"

namespace eng {

    //===== Level =====

    Level::Level() : map(Map(glm::ivec2(0,0), nullptr)) {}

    Level::Level(const glm::vec2& mapSize, const TilesetRef& tileset) : map(Map(mapSize, tileset)) {}

    // Level::Level(const Savefile& savefile) {}

    void Level::Render() {
        map.Render();
    }

}//namespace eng
