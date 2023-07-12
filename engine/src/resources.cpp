#include "engine/game/resources.h"

#include "engine/core/window.h"
#include "engine/core/renderer.h"

#include "engine/utils/json.h"
#include "engine/utils/utils.h"
#include "engine/utils/timer.h"

constexpr float DEFAULT_FONT_SCALE = 0.055f;

namespace eng {
    //defined in sprite.cpp:280 (+-)
    SpritesheetData ParseConfig_Spritesheet(const std::string& name, const nlohmann::json& config, int texture_flags);
}

namespace eng::Resources {

    struct Data {
        float fontScale = DEFAULT_FONT_SCALE;

        std::unordered_map<std::string, FontRef> fonts;
        std::unordered_map<std::string, ShaderRef> shaders;
        std::unordered_map<std::string, TextureRef> textures;

        std::unordered_map<std::string, SpritesheetRef> spritesheets;
        std::unordered_map<std::string, TilesetRef> tilesets;

        std::unordered_map<std::string, GameObjectDataRef> objects;
        bool preloaded = false;
    };
    static Data data = {};

    //============

    void ResizeFonts(int windowHeight);

    void PreloadSpritesheets();
    void PreloadObjects();

    //============

    void Preload() {
        ASSERT_MSG(!data.preloaded, "Calling Resources::PreloadObjects multiple times!");

        PreloadSpritesheets();

        PreloadObjects();

        ENG_LOG_INFO("[R] Resources::Preload");
        data.preloaded = true;
    }

    void Release() {
        data = {};
    }

    void OnResize(int width, int height) {
        ResizeFonts(height);
    }

    //============

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

    //===== Textures =====

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

    //===== Fonts =====

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

    void SetFontScale(float scale, bool forceResize) {
        if(scale != data.fontScale) {
            data.fontScale = scale;
            forceResize = true;
        }

        if(forceResize) {
            ResizeFonts(Window::Get().Height());
        }
    }

    //===== Tilesets =====

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

    //===== Sprites =====

    Sprite LoadSprite(const std::string& name) {
        size_t pos = name.find_last_of("/");
        if(pos == std::string::npos) {
            ENG_LOG_ERROR("Resources::LoadSprite - invalid sprite name '{}' (missing spritesheet identifier).", name);
            throw std::runtime_error("");
        }

        std::string spritesheet_name = name.substr(0, pos);
        std::string sprite_name = name.substr(pos+1);
        
        if(!data.spritesheets.count(spritesheet_name)) {
            ENG_LOG_ERROR("Resources::LoadSprite - spritesheet '{}' not found.", spritesheet_name);
            throw std::runtime_error("");
        }
        SpritesheetRef spritesheet = data.spritesheets.at(spritesheet_name);

        Sprite sprite;
        if(!spritesheet->TryGet(sprite_name, sprite)) {
            ENG_LOG_ERROR("Resources::LoadSprite - sprite '{}' not found in '{}'.", sprite_name, spritesheet_name);
            throw std::runtime_error("");
        }

        return sprite;
    }

    SpritesheetRef LoadSpritesheet(const std::string& name) {
        if(!data.spritesheets.count(name)) {
            ENG_LOG_ERROR("Resources::LoadSpritesheet - spritesheet '{}' not found.", name);
            throw std::runtime_error("");
        }
        return data.spritesheets.at(name);
    }

    //===== Object Prefabs =====

    GameObjectDataRef LoadObject(const std::string& name) {
        if(!data.preloaded) {
            ENG_LOG_ERROR("Resources::LoadObject - objects need to be preloaded!");
            throw std::runtime_error("");
        }

        if(!data.objects.count(name)) {
            ENG_LOG_ERROR("Resources::LoadObject - object '{}' not found.", name);
            throw std::runtime_error("");
        }

        return data.objects.at(name);
    }

    BuildingDataRef LoadBuilding(const std::string& name) {
        GameObjectDataRef data = LoadObject(name);
        BuildingDataRef res = std::dynamic_pointer_cast<BuildingData>(data);
        if(res == nullptr) {
            ENG_LOG_ERROR("Resources::LoadBuilding - object '{}' is not a building.", name);
            throw std::runtime_error("");
        }
        return res;
    }

    UnitDataRef LoadUnit(const std::string& name) {
        GameObjectDataRef data = LoadObject(name);
        UnitDataRef res = std::dynamic_pointer_cast<UnitData>(data);
        if(res == nullptr) {
            ENG_LOG_ERROR("Resources::LoadUnit - object '{}' is not a building.", name);
            throw std::runtime_error("");
        }
        return res;
    }

    //============

    void ResizeFonts(int windowHeight) {
        for(auto& [name, font] : data.fonts) {
            font->Resize(data.fontScale * windowHeight);
        }
    }
    
    void PreloadSpritesheets() {
        using json = nlohmann::json;

        size_t sprite_count = 0;
        Timer t = {};

        t.Reset();
        json config = json::parse(ReadFile("res/json/spritesheets.json"));
        for(auto& entry : config) {
            std::string name = entry.at("name");

            SpritesheetData sData = ParseConfig_Spritesheet(name, entry, 0);
            SpritesheetRef spritesheet = std::make_shared<Spritesheet>(sData);

            data.spritesheets.insert({ name, spritesheet });
            sprite_count += spritesheet->Size();
        }
        float time_elapsed = t.TimeElapsed<Timer::ms>() * 1e-3f;
        ENG_LOG_TRACE("Resources::PreloadSpritesheets - parsed {} spritesheets ({} sprites in total, {:.2f}s)", data.spritesheets.size(), sprite_count, time_elapsed);

        //TODO: texture merging
    }
    
    void PreloadObjects() {
        //TODO: read & parse the objects JSONs
    }

}//namespace eng::Resources
