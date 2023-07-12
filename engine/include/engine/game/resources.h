#pragma once

#include <string>
#include <memory>

#include "engine/core/texture.h"
#include "engine/core/text.h"
#include "engine/core/shader.h"
#include "engine/core/sprite.h"

#include "engine/game/map.h"
#include "engine/game/object_data.h"

//Basic resources manager. Nothing fancy, mostly for resource sharing and name2path translation.
namespace eng::Resources {

    void Preload();

    void Release();

    void OnResize(int width, int height);

    //============

    ShaderRef LoadShader(const std::string& name, bool forceReload = false);

    //===== Textures =====

    TextureRef LoadTexture(const std::string& name, bool skipCache = false);
    TextureRef LoadTexture(const std::string& name, const TextureParams& params, int flags, bool skipCache = false);

    //===== Fonts =====

    FontRef LoadFont(const std::string& name);
    FontRef DefaultFont();

    //Defines height of the font (relative to window height).
    //Scale=1.f in RenderText() calls translates to this height.
    //forceResize - triggers font resizing even if the scale matches previous scale.
    void SetFontScale(float scale, bool forceResize = false);

    //===== Tilesets =====

    TilesetRef LoadTileset(const std::string& name, bool forceReload = false);
    TilesetRef DefaultTileset();

    //===== Sprites =====

    //name format = spritesheet_name/sprite_name
    Sprite LoadSprite(const std::string& full_name);
    SpritesheetRef LoadSpritesheet(const std::string& name);

    //===== Object Prefabs =====

    GameObjectDataRef LoadObject(const std::string& name);
    BuildingDataRef LoadBuilding(const std::string& name);
    UnitDataRef LoadUnit(const std::string& name);

}//namespace eng::Resources
