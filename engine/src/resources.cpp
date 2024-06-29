#include "engine/game/resources.h"

#include "engine/core/window.h"
#include "engine/core/renderer.h"
#include "engine/core/cursor.h"

#include "engine/utils/json.h"
#include "engine/utils/utils.h"
#include "engine/utils/timer.h"
#include "engine/utils/dbg_gui.h"

#include "engine/game/gameobject.h"
#include "engine/game/object_parsing.h"

constexpr float DEFAULT_FONT_SCALE = 0.055f;

#define RESEARCH_MAX_LEVEL 10

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
        std::array<std::array<BuildingDataRef, 2>, BuildingType::COUNT> buildings;
        std::array<std::array<UnitDataRef, 2>, UnitType::COUNT> units;
        std::array<UtilityObjectDataRef, SpellID::COUNT> spells;
        std::vector<UtilityObjectDataRef> utilities;

        std::array<UnitDataRef, UnitType::COUNT2> other_units;
        std::array<BuildingDataRef, BuildingType::COUNT2> other_buildings;

        std::array<ResearchVisuals, ResearchType::COUNT> research_viz;
        std::unordered_map<int, ResearchData> research_data;

        CursorIconManager cursorIcons;

        bool linkable = false;
        bool preloaded = false;
    };
    static Data data = {};

    //============

    void ResizeFonts(int windowHeight);

    void PreloadSpritesheets();
    void PreloadObjects();

    void ProcessIndexFile();
    void LoadObjectDefinitions();
    void PreloadResearchDefinitions();
    void FinalizeOthers();
    void MergeSpritesheets();

    void ValidateIndexEntries();
    UtilityObjectDataRef SetupCorpseData();
    void FinalizeButtonDescriptions();

    GameObjectDataRef LinkObject(const std::string& name);
    GameObjectDataRef LinkObject(const glm::ivec3& num_id, bool initialized_only);
    UtilityObjectDataRef LinkSpell(int spellID, bool initialized_only);

    //============

    void Preload() {
        ASSERT_MSG(!data.preloaded, "Calling Resources::Preload multiple times!");

        PreloadSpritesheets();

        PreloadObjects();

        PreloadResearchDefinitions();

        MergeSpritesheets();

        data.cursorIcons = CursorIconManager("res/json/cursors.json");

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

    bool SpriteExists(const std::string& name) {
        size_t pos = name.find_last_of("/");
        if(pos == std::string::npos) {
            return false;
        }

        std::string spritesheet_name = name.substr(0, pos);
        std::string sprite_name = name.substr(pos+1);
        
        if(!data.spritesheets.count(spritesheet_name))
            return false;
        SpritesheetRef spritesheet = data.spritesheets.at(spritesheet_name);

        Sprite sprite;
        return spritesheet->TryGet(sprite_name, sprite);
    }

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

    GameObjectDataRef LinkObject(const std::string& name) {
        if(!data.linkable) {
            ENG_LOG_ERROR("Resources::LinkObject - index needs to be processed before linking is allowed!");
            throw std::runtime_error("");
        }

        if(!data.objects.count(name)) {
            ENG_LOG_ERROR("Resources::LoadObject - object '{}' not found.", name);
            throw std::runtime_error("");
        }

        return data.objects.at(name);
    }

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

    UtilityObjectDataRef LoadUtilityObj(const std::string& name) {
        GameObjectDataRef data = LoadObject(name);
        UtilityObjectDataRef res = std::dynamic_pointer_cast<UtilityObjectData>(data);
        if(res == nullptr) {
            ENG_LOG_ERROR("Resources::LoadUtilityObj - object '{}' is not an utility object.", name);
            throw std::runtime_error("");
        }
        return res;
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
            ENG_LOG_ERROR("Resources::LoadUnit - object '{}' is not an unit.", name);
            throw std::runtime_error("");
        }
        return res;
    }

    GameObjectDataRef LinkObject(const glm::ivec3& num_id, bool initialized_only) {
        if(!data.linkable) {
            ENG_LOG_ERROR("Resources::LinkObject - index needs to be processed before linking is allowed!");
            throw std::runtime_error("");
        }

        GameObjectDataRef obj = nullptr;
        switch(num_id[0]) {
            case ObjectType::BUILDING:
                if((unsigned int)(num_id[1]) >= data.buildings.size()) {
                    ENG_LOG_ERROR("Resources::LinkObject - building - invalid ID (id = {}, max legit value = {})!", num_id[1], data.buildings.size());
                    throw std::runtime_error("");
                }
                obj = data.buildings[num_id[1]][int(bool(num_id[2]))];
                break;
            case ObjectType::UNIT:
                if((unsigned int)(num_id[1]) >= data.units.size()) {
                    ENG_LOG_ERROR("Resources::LinkObject - unit - invalid ID (id = {}, max legit value = {})!", num_id[1], data.units.size());
                    throw std::runtime_error("");
                }
                obj = data.units[num_id[1]][int(bool(num_id[2]))];
                break;
            case ObjectType::UTILITY:
                if((unsigned int)(num_id[1]) >= data.utilities.size()) {
                    ENG_LOG_ERROR("Resources::LinkObject - utility - invalid ID (id = {}, max legit value = {})!", num_id[1], data.utilities.size());
                    throw std::runtime_error("");
                }
                obj = data.utilities[num_id[1]];
                break;
            default:
                ENG_LOG_ERROR("Resources::LinkObject - invalid id, failed to resolve object type (id={})", num_id[0]);
                throw std::runtime_error("");
        }

        if(obj == nullptr || (initialized_only && !obj->initialized)) {
            ENG_LOG_ERROR("Resources::LinkObject - attempting to link object that is not yet initialized (num_id=({},{},{}))", num_id[0], num_id[1], num_id[2]);
            throw std::runtime_error("");
        }

        return obj;
    }

    UtilityObjectDataRef LinkSpell(int spellID, bool initialized_only) {
        if(!data.linkable) {
            ENG_LOG_ERROR("Resources::LinkObject - index needs to be processed before linking is allowed!");
            throw std::runtime_error("");
        }

        if((unsigned int)(spellID) >= data.spells.size()) {
            ENG_LOG_ERROR("Resources::LinkObject - spell - invalid ID (id = {}, max legit value = {})!", spellID, data.spells.size());
            throw std::runtime_error("");
        }
        UtilityObjectDataRef obj = data.spells[spellID];

        if(obj == nullptr || (initialized_only && !obj->initialized)) {
            ENG_LOG_ERROR("Resources::LinkObject - attempting to link object that is not yet initialized (spell - {}))", spellID);
            throw std::runtime_error("");
        }

        return obj;
    }

    GameObjectDataRef LoadObject(const glm::ivec3& num_id) {
        switch(num_id[0]) {
            case ObjectType::BUILDING:
                return LoadBuilding(num_id[1], (bool)num_id[2]);
            case ObjectType::UNIT:
                return LoadUnit(num_id[1], (bool)num_id[2]);
            case ObjectType::UTILITY:
                return LoadUtilityObj(num_id[1]);
            default:
                ENG_LOG_ERROR("Resources::LoadObject - invalid id, failed to resolve object type (id={})", num_id[0]);
                throw std::runtime_error("");
        }
    }

    UtilityObjectDataRef LoadUtilityObj(int num_id) {
        if(!data.preloaded) {
            ENG_LOG_ERROR("Resources::LoadUtilityObj - objects need to be preloaded!");
            throw std::runtime_error("");
        }
        if((unsigned int)(num_id) >= data.utilities.size()) {
            ENG_LOG_ERROR("Resources::LoadUtilityObj - invalid ID (id = {}, max legit value = {})!", num_id, data.utilities.size());
            throw std::runtime_error("");
        }
        return data.utilities[num_id];
    }

    UtilityObjectDataRef LoadSpell(int num_id) {
        if(!data.preloaded) {
            ENG_LOG_ERROR("Resources::LoadSpell - objects need to be preloaded!");
            throw std::runtime_error("");
        }
        if((unsigned int)(num_id) >= data.spells.size()) {
            ENG_LOG_ERROR("Resources::LoadSpell - invalid ID (id = {}, max legit value = {})!", num_id, data.spells.size());
            throw std::runtime_error("");
        }
        return data.spells[num_id];
    }

    BuildingDataRef LoadBuilding(int num_id, bool isOrc) {
        if(!data.preloaded) {
            ENG_LOG_ERROR("Resources::LoadBuilding - objects need to be preloaded!");
            throw std::runtime_error("");
        }
        if((unsigned int)(num_id) < data.buildings.size()) {
            return data.buildings[num_id][(int)(isOrc)];
        }
        else {
            num_id -= 101;
            if(num_id >= 0 && num_id < data.other_buildings.size()) {
                return data.other_buildings[num_id];
            }
        }
        
        ENG_LOG_ERROR("Resources::LoadBuilding - invalid ID (id = {})!", num_id);
        throw std::runtime_error("");
    }

    UnitDataRef LoadUnit(int num_id, bool isOrc) {
        if(!data.preloaded) {
            ENG_LOG_ERROR("Resources::LoadUnit - objects need to be preloaded!");
            throw std::runtime_error("");
        }
        if((unsigned int)(num_id) < data.units.size()) {
            return data.units[num_id][(int)(isOrc)];
        }
        else {
            num_id -= 101;
            if(num_id >= 0 && num_id < data.other_units.size()) {
                return data.other_units[num_id];
            }
        }
        
        ENG_LOG_ERROR("Resources::LoadUnit - invalid ID (id = {})!", num_id);
        throw std::runtime_error("");
    }

    bool LoadResearchInfo(int type, int level, ResearchInfo& out_info) {
        if(!data.preloaded) {
            ENG_LOG_ERROR("Resources::LoadResearchInfo - data need to be preloaded!");
            throw std::runtime_error("");
        }
        if((unsigned int)(type) >= data.research_viz.size()) {
            return false;
        }
        int idx = type * RESEARCH_MAX_LEVEL + level;
        if(!data.research_data.count(idx)) {
            return false;
        }
        
        out_info.viz    = data.research_viz[type];
        out_info.data   = data.research_data[idx];
        
        return true;
    }

    int LoadResearchBonus(int type, int level) {
        int idx = type * RESEARCH_MAX_LEVEL + level;
        if(data.research_data.count(idx)) {
            return data.research_data.at(idx).value;
        }
        return 0;
    }

    int LoadResearchLevels(int type) {
        return data.research_viz.at(type).levels;
    }

    void DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        ImGui::Begin("Resources");

        ImGui::Text("Fonts: %d, Shaders: %d", (int)data.fonts.size(), (int)data.shaders.size());
        ImGui::Text("Textures: %d, Tilesets: %d", (int)data.textures.size(), (int)data.tilesets.size());
        ImGui::Text("Spritesheets: %d", (int)data.spritesheets.size());
        ImGui::Text("Object data: %d", (int)data.objects.size());

        ImGui::Separator();
        ImGui::Text("Buildings: %d, Units: %d", (int)data.buildings.size(), (int)data.units.size());
        ImGui::Text("Utilities: %d, Spells: %d, Research: %d", (int)data.spells.size(), (int)data.utilities.size(), (int)data.research_data.size());

        ImGui::Separator();
        ImGui::Text("Total texture handles: %d", Texture::HandleCount());
        
        ImGui::End();
#endif
    }

    namespace CursorIcons {

        void Update() {
            data.cursorIcons.UpdateIcon();
        }

        void SetIcon(int idx) {
            data.cursorIcons.SetIcon(idx);
        }

    }//namespace CursorIcons

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
        Timer t = {};
        t.Reset();

        //load & process index file
        ProcessIndexFile();
        
        //fully load & prepare object prefab definitions
        LoadObjectDefinitions();

        FinalizeOthers();

        // ValidateIndexEntries();
        SetupCorpseData();
        FinalizeButtonDescriptions();

        float time_elapsed = t.TimeElapsed<Timer::ms>() * 1e-3f;
        ENG_LOG_TRACE("Resources::PreloadObjects - parsed {} object prefab descriptions ({:.2f}s)", data.objects.size(), time_elapsed);
    }

    void ProcessIndexFile() {
        using json = nlohmann::json;

        json config = json::parse(ReadFile("res/json/index.json"));
        int i = 0;
        
        i = 0;
        for(auto& entry : config.at("units")) {
            if(i >= data.units.size()) {
                ENG_LOG_ERROR("Resources::ProcessIndexFile - invalid unit count.");
                throw std::runtime_error("");
            }
            for(int j = 0; j < 2; j++) {
                if(data.objects.count(entry.at(j))) {
                    ENG_LOG_ERROR("Resources::ProcessIndexFile - objects can only be referenced once in the entire index ('{}').", entry.at(j));
                    throw std::runtime_error("");
                }
                data.units[i][j] = std::make_shared<UnitData>();
                data.units[i][j]->SetupID(entry.at(j), glm::ivec3(ObjectType::UNIT, i, j));
                data.objects.insert({ entry.at(j), data.units[i][j] });
            }
            i++;
        }

        i = 0;
        for(auto& entry : config.at("buildings")) {
            if(i >= data.buildings.size()) {
                ENG_LOG_ERROR("Resources::ProcessIndexFile - invalid building count.");
                throw std::runtime_error("");
            }
            for(int j = 0; j < 2; j++) {
                if(data.objects.count(entry.at(j))) {
                    ENG_LOG_ERROR("Resources::ProcessIndexFile - objects can only be referenced once in the entire index ('{}').", entry.at(j));
                    throw std::runtime_error("");
                }
                data.buildings[i][j] = std::make_shared<BuildingData>();
                data.buildings[i][j]->SetupID(entry.at(j), glm::ivec3(ObjectType::BUILDING, i, j));
                data.objects.insert({ entry.at(j), data.buildings[i][j] });
            }
            i++;
        }

        i = 0;
        for(const std::string& entry : config.at("spells")) {
            if(i >= data.spells.size()) {
                ENG_LOG_ERROR("Resources::ProcessIndexFile - invalid spell count.");
                throw std::runtime_error("");
            }
            if(data.objects.count(entry)) {
                ENG_LOG_ERROR("Resources::ProcessIndexFile - objects can only be referenced once in the entire index ('{}').", entry);
                throw std::runtime_error("");
            }
            data.spells[i] = std::make_shared<UtilityObjectData>();
            data.spells[i]->SetupID(entry, glm::ivec3(ObjectType::UTILITY, i, 0));
            data.objects.insert({ entry, data.spells[i] });
            i++;
        }

        i = 0;
        for(auto& entry : config.at("utility")) {
            if(data.objects.count(entry)) {
                ENG_LOG_ERROR("Resources::ProcessIndexFile - objects can only be referenced once in the entire index ('{}').", entry);
                throw std::runtime_error("");
            }
            data.utilities.push_back(std::make_shared<UtilityObjectData>());
            data.utilities.back()->SetupID(entry, glm::ivec3(ObjectType::UTILITY, i, 0));
            data.objects.insert({ entry, data.utilities.back() });
            i++;
        }

        data.linkable = true;
    }

    void LoadObjectDefinitions() {
        using json = nlohmann::json;

        json config = json::parse(ReadFile("res/json/utility.json"));
        for(auto& entry : config) {
            std::string str_id = entry.at("str_id");
            if(!data.objects.count(str_id))
                data.objects.insert({ str_id, GameObjectData_ParseNew(entry) });
            else
                GameObjectData_ParseExisting(entry, data.objects.at(str_id));
        }

        config = json::parse(ReadFile("res/json/spells.json"));
        for(auto& entry : config) {
            std::string str_id = entry.at("str_id");
            if(!data.objects.count(str_id))
                data.objects.insert({ str_id, GameObjectData_ParseNew(entry) });
            else
                GameObjectData_ParseExisting(entry, data.objects.at(str_id));
        }

        //load regular object definitions
        config = json::parse(ReadFile("res/json/objects.json"));
        for(auto& entry : config) {
            std::string str_id = entry.at("str_id");
            if(!data.objects.count(str_id))
                data.objects.insert({ str_id, GameObjectData_ParseNew(entry) });
            else
                GameObjectData_ParseExisting(entry, data.objects.at(str_id));
        }
    }

    void PreloadResearchDefinitions() {
        Timer t = {};
        t.Reset();

        using json = nlohmann::json;
        json config = json::parse(ReadFile("res/json/research.json"));
        if(config.size() != ResearchType::COUNT) {
            ENG_LOG_ERROR("Resources::PreloadResearchDefinitions - invalid number of researches detected.");
            throw std::runtime_error("");
        }
        
        std::vector<ResearchData> level_entries = {};
        for(int i = 0; i < ResearchType::COUNT; i++) {
            auto& entry = config.at(i);
            data.research_viz[i] = ResearchInfo_Parse(entry, i, level_entries);

            for(int j = 0; j < (int)level_entries.size(); j++) {
                data.research_data.insert({ i * RESEARCH_MAX_LEVEL + (j+1), level_entries[j] });
            }
            level_entries.clear();
        }

        float time_elapsed = t.TimeElapsed<Timer::ms>() * 1e-3f;
        ENG_LOG_TRACE("Resources::PreloadResearchDefinitions - parsed {} research descriptions ({:.2f}s)", data.research_viz.size(), time_elapsed);
    }

    void FinalizeOthers() {
        using json = nlohmann::json;
        json config = json::parse(ReadFile("res/json/index.json"));

        int i = 0;
        for(auto& entry : config.at("other")) {
            std::string name        = entry.at("name");
            glm::ivec2 id           = eng::json::parse_ivec2(entry.at("id"));

            if(!data.objects.count(name)) {
                ENG_LOG_ERROR("Resources::FinalizeOthers - object definition mentioned in index wasn't loaded during the regular object loading phase ('{}').", name);
                throw std::runtime_error("");
            }

            GameObjectDataRef obj = data.objects.at(name);
            obj->SetupID(name, glm::ivec3(obj->objectType, id.x, id.y));
            switch(obj->objectType) {
                case ObjectType::UNIT:
                    data.other_units.at(id.x - 101) = std::static_pointer_cast<UnitData>(obj);
                    break;
                case ObjectType::BUILDING:
                    data.other_buildings.at(id.x - 101) = std::static_pointer_cast<BuildingData>(obj);
                    break;
            }
        }

        for(const auto& spell : data.spells) {
            SpellID::SetPrice(spell->num_id[1], spell->cost[3]);
        }
        for(int i = 0; i < SpellID::COUNT; i++) {
            if(SpellID::Price(i) == SpellID::INVALID_PRICE)
                ENG_LOG_WARN("Spell (id={}) data were not found.", i);
        }
    }

    void MergeSpritesheets() {
        ENG_LOG_TRACE("Resources::MergeSpritesheets");
        std::vector<TextureRef> textures = {};
        for(auto& [hash, ss] : data.spritesheets) {
            textures.push_back(ss->Texture());
        }
        Texture::MergeTextures(textures, GL_NEAREST);
    }

    void ValidateIndexEntries() {
        for(auto& entry : data.buildings) {
            for(auto& val : entry) {
                if(!val->initialized) {
                    ENG_LOG_ERROR("Resources::ValidateIndexEntries - index entry for building '{}' has no definition.", val->str_id);
                    throw std::runtime_error("");
                }
            }
        }

        for(auto& entry : data.units) {
            for(auto& val : entry) {
                if(!val->initialized) {
                    ENG_LOG_ERROR("Resources::ValidateIndexEntries - index entry for unit '{}' has no definition.", val->str_id);
                    throw std::runtime_error("");
                }
            }
        }

        for(auto& val : data.utilities) {
            if(!val->initialized) {
                ENG_LOG_ERROR("Resources::ValidateIndexEntries - index entry for utility object '{}' has no definition.", val->str_id);
                throw std::runtime_error("");
            }
        }
    }

    UtilityObjectDataRef SetupCorpseData() {
        UtilityObjectDataRef corpse = std::dynamic_pointer_cast<UtilityObjectData>(Resources::LinkObject("corpse"));
        if(corpse == nullptr) {
            ENG_LOG_ERROR("Resources::SetupCorpseData - 'corpse' prefab not found");
            throw std::runtime_error("");
        }
        
        AnimatorDataRef anim = corpse->animData;
        int idx = anim->ActionCount();

        //scan through all the units & retrieve their death animations
        for(auto& [key, value] : data.objects) {
            UnitDataRef ptr = std::dynamic_pointer_cast<UnitData>(value);
            if(ptr != nullptr && ptr->initialized && ptr->animData->HasGraphics(3)) {
                anim->AddAction(idx, ptr->animData->GetGraphics(3));

                //mark unit's animation index in the corpse object, for easier access
                ptr->deathAnimIdx = idx++;
            }
        }

        return corpse;
    }

    void FinalizeButtonDescriptions() {
        for(auto& [name, entry] : data.objects) {
            GameObjectData_FinalizeButtonDescriptions(entry);
        }
    }

}//namespace eng::Resources
