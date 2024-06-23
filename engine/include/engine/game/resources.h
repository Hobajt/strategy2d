#pragma once

#include <string>
#include <memory>

#include "engine/core/texture.h"
#include "engine/core/text.h"
#include "engine/core/shader.h"
#include "engine/core/sprite.h"

#include "engine/game/map.h"
#include "engine/game/object_data.h"
#include "engine/game/techtree.h"

//Basic resources manager. Nothing fancy, mostly for resource sharing and name2path translation.
namespace eng::Resources {

    void Preload();

    void Release();

    void OnResize(int width, int height);

    void TextureMergingUpdate();

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
    bool SpriteExists(const std::string& full_name);
    Sprite LoadSprite(const std::string& full_name);
    SpritesheetRef LoadSpritesheet(const std::string& name);

    //===== Object Prefabs =====

    //lookup in the map
    GameObjectDataRef LoadObject(const std::string& str_id);
    UtilityObjectDataRef LoadUtilityObj(const std::string& str_id);
    BuildingDataRef LoadBuilding(const std::string& str_id);
    UnitDataRef LoadUnit(const std::string& str_id);

    //lookup in the index
    GameObjectDataRef LoadObject(const glm::ivec3& num_id);
    UtilityObjectDataRef LoadUtilityObj(int num_id);
    UtilityObjectDataRef LoadSpell(int num_id);
    BuildingDataRef LoadBuilding(int num_id, bool isOrc);
    UnitDataRef LoadUnit(int num_id, bool isOrc);

    bool LoadResearchInfo(int type, int level, ResearchInfo& out_info);
    int LoadResearchBonus(int type, int level);
    int LoadResearchLevels(int type);

    //===== Cursor icons =====

    namespace CursorIcons {
        void Update();
        void SetIcon(int idx);
    }

}//namespace eng::Resources

namespace eng::CursorIconName { enum { HAND_HU, HAND_OC, EAGLE_YELLOW, EAGLE_GREEN, MAGNIFYING_GLASS, CROSSHAIR }; }
