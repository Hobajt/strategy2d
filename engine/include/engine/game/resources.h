#pragma once

#include <string>
#include <memory>

#include "engine/core/texture.h"
#include "engine/core/text.h"
#include "engine/core/shader.h"
#include "engine/core/sprite.h"

#include "engine/game/map.h"

//Basic resources manager. Nothing fancy, mostly for resource sharing and name2path translation.
namespace eng::Resources {

    void Release();

    void OnResize(int width, int height);

    //============

    //Defines height of the font (relative to window height).
    //Scale=1.f in RenderText() calls translates to this height.
    //forceResize - triggers font resizing even if the scale matches previous scale.
    void SetFontScale(float scale, bool forceResize = false);

    ShaderRef LoadShader(const std::string& name, bool forceReload = false);

    TextureRef LoadTexture(const std::string& name, bool skipCache = false);

    FontRef LoadFont(const std::string& name);
    FontRef DefaultFont();

    SpritesheetRef LoadSpritesheet(const std::string& name);
    SpritesheetRef DefaultSpritesheet();

    TilesetRef LoadTileset(const std::string& name);
    TilesetRef DefaultTileset();

}//namespace eng::Resources

