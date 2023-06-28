#include "context.h"

#include <random>

using namespace eng;

//===== EditorContext =====

EditorContext::EditorContext() : input(EditorInputHandler(*this)), tools(EditorTools(*this)) {
    components.push_back(std::make_shared<HotkeysMenu>(*this));
    components.push_back(std::make_shared<DiplomacyMenu>(*this));
    components.push_back(std::make_shared<TechtreeMenu>(*this));
    components.push_back(std::make_shared<ToolsMenu>(*this));
    components.push_back(std::make_shared<InfoMenu>(*this));
    components.push_back(std::make_shared<FileMenu>(*this));
}

void EditorContext::Terrain_SetupNew(const glm::ivec2& size, const eng::TilesetRef& tileset) {
    level = Level(size, tileset);

    //camera reset
    Camera& camera = Camera::Get();
    camera.SetBounds(size);
    camera.Position(glm::vec2(size) * 0.5f - 0.5f);
    camera.ZoomToFit(glm::vec2(size) + 1.f);

    std::static_pointer_cast<InfoMenu, EditorComponent>(GetComponent<InfoMenu>())->NewLevelCreated();
    tools.NewLevelCreated(size);
}

int EditorContext::Terrain_Load(const std::string& filepath) {
    Level new_level;
    int res = Level::Load(filepath, new_level);
    if(res == 0) {
        level = std::move(new_level);
    }
    return res;
}

int EditorContext::Terrain_Save(const std::string& filepath) {
    return int(!level.Save(filepath));
}

void EditorContext::Render() {
    level.map.Render();
    tools.Render();
    for(EditorComponentRef& comp : components) {
        comp->Render();
    }
}
