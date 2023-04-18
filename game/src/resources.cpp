#include "resources.h"

#include <engine/engine.h>

constexpr float DEFAULT_FONT_SCALE = 0.055f;

using namespace eng;

namespace Resources {

    struct Data {
        float fontScale = DEFAULT_FONT_SCALE;

        std::unordered_map<std::string, FontRef> fonts;
        std::unordered_map<std::string, ShaderRef> shaders;
        std::unordered_map<std::string, TextureRef> textures;
    };
    static Data data = {};

    //============

    void ResizeFonts(int windowHeight);

    //============

    void Initialize() {
        //TODO: generate GUI textures & bake them into single texture
        //will need to use something like sprites to access them tho

        /*spritesheets won't be managed by this manager at all
            - spritesheets will be separate for individual units
            - will bake all used objects into a single spritesheet on game start
            - this will be handled by some other class
            - 
        */

       /*name2path - there probably is no need for this
            - will just use filenames as names
            - directory path can be inferred based on data type (texture/shared/font/...)
       */
        
    }

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

    //============

    void ResizeFonts(int windowHeight) {
        for(auto& [name, font] : data.fonts) {
            font->Resize(data.fontScale * windowHeight);
        }
    }

}//namespace Resources