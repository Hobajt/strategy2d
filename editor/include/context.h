#pragma once

#include <engine/engine.h>
#include <random>

#include "tools.h"
#include "menus.h"
#include "input.h"

//===== EditorMap =====

class EditorMap {
public:
    EditorMap();
    EditorMap(const eng::Map& map);
    ~EditorMap();

    EditorMap(const EditorMap&) = delete;
    EditorMap& operator=(const EditorMap&) = delete;

    EditorMap(EditorMap&&) noexcept;
    EditorMap& operator=(EditorMap&&) noexcept;

    eng::Map& Map() { return map; }

    void PaintRegion(const glm::ivec2& coord, int brushLeft, int brushRight, int tileType, bool randomVariations, int variationValue);
    void ClearPaint();

    void Render();

    void DBG_VIZ(bool stroke_path, bool stroke_limits);

    OperationStack::Operation CreateOpRecord();

    void SyncToLevelData(eng::Map& data);
private:
    void Init(const glm::ivec2& size);

    void Move(EditorMap&&) noexcept;
    void Release() noexcept;
private:
    const eng::Map* data = nullptr;

    eng::Map map;
    bool* bitmap = nullptr;

    glm::ivec2 session_min;
    glm::ivec2 session_max;
    int viz_state = 0;

    std::mt19937 gen;
    std::uniform_int_distribution<int> dist;
};

//===== EditorLevel =====

struct EditorLevel {
    eng::Level level;
    EditorMap map;
public:
    void CommitChanges();
};

//===== EditorContext =====

struct EditorContext {
    EditorLevel level;
    EditorTools tools;
    EditorInputHandler input;
    std::vector<EditorComponentRef> components;
public:
    EditorContext();

    template <typename T> EditorComponentRef GetComponent();

    void Terrain_SetupNew(const glm::ivec2& size, const eng::TilesetRef& tileset);
    int Terrain_Load(const std::string& filepath);
    int Terrain_Save(const std::string& filepath);
};

//===========

template <typename T>
EditorComponentRef EditorContext::GetComponent() {
    for(EditorComponentRef& comp : components) {
        if(dynamic_cast<T*>(comp.get()) != nullptr)
            return comp;
    }
    return nullptr;
}
