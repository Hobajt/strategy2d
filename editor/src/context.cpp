#include "context.h"

#include <random>

using namespace eng;

//===== EditorContext =====

EditorContext::EditorContext() : input(EditorInputHandler(*this)), tools(EditorTools(*this)) {
    components.push_back(std::make_shared<HotkeysMenu>(*this));
    components.push_back(std::make_shared<FactionsMenu>(*this));
    components.push_back(std::make_shared<ToolsMenu>(*this));
    components.push_back(std::make_shared<InfoMenu>(*this));
    components.push_back(std::make_shared<FileMenu>(*this));
}

void EditorContext::Terrain_SetupNew(const glm::ivec2& size, const eng::TilesetRef& tileset) {
    level.Release();
    level = Level(size, tileset);
    level.objects.InitObjectCounter_Editor();

    //camera reset
    Camera& camera = Camera::Get();
    camera.SetBounds(size);
    camera.Position(glm::vec2(size) * 0.5f - 0.5f);
    camera.ZoomToFit(glm::vec2(size) + 1.f);

    for(EditorComponentRef& comp : components) {
        comp->NewLevelCreated();
    }
    tools.NewLevelCreated(size);
}

int EditorContext::Terrain_Load(const std::string& filepath) {
    level.Release();
    int res = Level::Load(filepath, level);
    level.map.EnableOcclusion(false);
    return res;
}

int EditorContext::Terrain_Save(const std::string& filepath) {
    //don't store faction entries into custom game file
    Factions backup;
    if(level.info.custom_game) {
        backup = std::move(level.factions);
        level.factions = Factions();
    }
    
    int res = int(!level.Save(filepath));

    if(level.info.custom_game) {
        level.factions = std::move(backup);
    }

    return res;
}

void EditorContext::Render() {
    level.map.Render();
    level.objects.Render();
    tools.Render();
    for(EditorComponentRef& comp : components) {
        comp->Render();
    }
}

void EditorContext::Update() {
    input.Update();
    level.objects.Update();
}
