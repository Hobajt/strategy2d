#pragma once

#include <string>
#include <memory>

#include <engine/engine.h>

//Basic resources manager. Nothing fancy, mostly for resource sharing and name2path translation.
namespace Resources {

    void Initialize();

    void Release();

    void OnResize(int width, int height);

    //============

    //Defines height of the font (relative to window height).
    //Scale=1.f in RenderText() calls translates to this height.
    //forceResize - triggers font resizing even if the scale matches previous scale.
    void SetFontScale(float scale, bool forceResize = false);

    eng::ShaderRef LoadShader(const std::string& name, bool forceReload = false);

    eng::TextureRef LoadTexture(const std::string& name);

    eng::FontRef LoadFont(const std::string& name);
    eng::FontRef DefaultFont();

}//namespace Resources
