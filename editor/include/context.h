#pragma once

#include <engine/engine.h>

#include "tools.h"
#include "menus.h"
#include "input.h"

//===== EditorContext =====

struct EditorContext {
    eng::Level level;
    EditorTools tools;
    EditorInputHandler input;
    std::vector<EditorComponentRef> components;
public:
    EditorContext();

    template <typename T> EditorComponentRef GetComponent();

    void Terrain_SetupNew(const glm::ivec2& size, const eng::TilesetRef& tileset);
    int Terrain_Load(const std::string& filepath);
    int Terrain_Save(const std::string& filepath);

    void Render();
    void Update();
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
