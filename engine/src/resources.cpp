#include "engine/game/resources.h"

#include "engine/core/window.h"
#include "engine/core/renderer.h"

constexpr float DEFAULT_FONT_SCALE = 0.055f;

namespace eng::Resources {

    struct Data {
        float fontScale = DEFAULT_FONT_SCALE;

        std::unordered_map<std::string, FontRef> fonts;
        std::unordered_map<std::string, ShaderRef> shaders;
        std::unordered_map<std::string, TextureRef> textures;

        std::unordered_map<std::string, SpritesheetRef> spritesheets;
        std::unordered_map<std::string, TilesetRef> tilesets;
    };
    static Data data = {};

    //============

    void ResizeFonts(int windowHeight);

    //============

    void Release() {
        data = {};
    }

    void OnResize(int width, int height) {
        ResizeFonts(height);
    }

    //============

    void SetFontScale(float scale, bool forceResize) {
        if(scale != data.fontScale) {
            data.fontScale = scale;
            forceResize = true;
        }

        if(forceResize) {
            ResizeFonts(Window::Get().Height());
        }
    }

    eng::ShaderRef LoadShader(const std::string& name, bool forceReload) {
        if(!data.shaders.count(name) || forceReload) {
            std::string filepath = std::string("res/shaders/") + name;

            if(!data.shaders.count(name))
                data.shaders.insert({ name, std::make_shared<Shader>(filepath) });
            else
                *data.shaders.at(name) = Shader(filepath);
            
            data.shaders.at(name)->InitTextureSlots(Renderer::TextureSlotsCount());
        }

        return data.shaders.at(name);
    }

    eng::TextureRef LoadTexture(const std::string& name, bool skipCache) {
        if(!data.textures.count(name)) {
            std::string filepath = std::string("res/textures/") + name;
            TextureRef tex = std::make_shared<Texture>(filepath);
            if(!skipCache)
                data.textures.insert({ name, tex });
            return tex;
        }

        return data.textures.at(name);
    }

    eng::TextureRef LoadTexture(const std::string& name, const TextureParams& params, int flags, bool skipCache) {
        if(!data.textures.count(name)) {
            std::string filepath = std::string("res/textures/") + name;
            TextureRef tex = std::make_shared<Texture>(filepath, params, flags);
            if(!skipCache)
                data.textures.insert({ name, tex });
            return tex;
        }

        return data.textures.at(name);
    }

    eng::FontRef LoadFont(const std::string& name) {
        if(!data.fonts.count(name)) {
            std::string filepath = std::string("res/fonts/") + name;
            data.fonts.insert({ name, std::make_shared<Font>(filepath, int(data.fontScale * Window::Get().Height())) });
        }

        return data.fonts.at(name);
    }

    eng::FontRef DefaultFont() {
        return LoadFont("PermanentMarker-Regular.ttf");
    }

    SpritesheetRef LoadSpritesheet(const std::string& name) {
        //TODO:
        return nullptr;
    }

    SpritesheetRef DefaultSpritesheet() {
        //TODO:
        return nullptr;
    }

    TilesetRef LoadTileset(const std::string& name, bool forceReload) {
        if(!data.tilesets.count(name) || forceReload) {
            char buf[2048];
            snprintf(buf, sizeof(buf), "res/json/tilemaps/%s.json", name.c_str());
            std::string filepath = std::string(buf);

            data.tilesets.insert({ name, std::make_shared<Tileset>(filepath) });
        }
        return data.tilesets.at(name);
    }

    TilesetRef DefaultTileset() {
        return LoadTileset("summer");
    }

    //============

    void ResizeFonts(int windowHeight) {
        for(auto& [name, font] : data.fonts) {
            font->Resize(data.fontScale * windowHeight);
        }
    }

}//namespace eng::Resources
