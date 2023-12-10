#include "engine/game/object_parsing.h"

#include "engine/utils/utils.h"

#include "engine/core/input.h"
#include "engine/game/resources.h"
#include "engine/game/gameobject.h"
#include "engine/game/utility_handlers.h"

namespace eng {

    namespace Resources {
        //defined in resources.cpp
        GameObjectDataRef LinkObject(const std::string& name);
        GameObjectDataRef LinkObject(const glm::ivec3& num_id, bool initialized_only);
    }

    //=======
    
    void Parse_GameObjectData(const nlohmann::json& config, GameObjectDataRef data);
    void Parse_FactionObjectData(const nlohmann::json& config, FactionObjectData& data);
    void Parse_BuildingData(const nlohmann::json& config, GameObjectDataRef data);
    void Parse_UnitData(const nlohmann::json& config, GameObjectDataRef data);
    void Parse_UtilityData(const nlohmann::json& config, GameObjectDataRef data);
    
    //=======

    int ObjectTypeFromCommandID(int cmd_id);
    void ParseButtonDescriptions(const nlohmann::json& config, FactionObjectData& data);
    void ParseAnimations_Building(const nlohmann::json& config, BuildingDataRef& data);
    void ParseAnimations_Unit(const nlohmann::json& config, UnitDataRef& data);
    void ParseAnimations_Utility(const nlohmann::json& config, UtilityObjectDataRef& data);

    SoundEffect ParseSoundEffect(const nlohmann::json& json);
    glm::ivec4 ParseCost(const nlohmann::json& json);
    int ResolveUnitAnimationID(const std::string& name);
    int ResolveBuildingAnimationID(const std::string& name);
    std::pair<std::string,std::string> SplitName(const std::string& name);
    void LoadBuildingSprites(std::map<int, SpriteGroup>& animations, int id, const std::string& prefix, const std::string& suffix, bool repeat, bool throwOnMiss = true);

    //========================= INTERFACE FUNCTIONS =================================

    GameObjectDataRef GameObjectData_ParseNew(const nlohmann::json& config) {
        int objectType = config.at("type");
        GameObjectDataRef data = nullptr;

        switch(objectType) {
            case ObjectType::BUILDING:
                data = std::static_pointer_cast<GameObjectData>(std::make_shared<BuildingData>());
                break;
            case ObjectType::UNIT:
                data = std::static_pointer_cast<GameObjectData>(std::make_shared<UnitData>());
                break;
            case ObjectType::UTILITY:
                data = std::static_pointer_cast<GameObjectData>(std::make_shared<UtilityObjectData>());
                break;
            default:
                ENG_LOG_ERROR("Encountered an unrecognized object type when parsing GameObjectData (type = {})", objectType);
                throw std::runtime_error("");
        }

        data->str_id = config.at("str_id");
        GameObjectData_ParseExisting(config, data);

        return data;
    }

    void GameObjectData_ParseExisting(const nlohmann::json& config, GameObjectDataRef data) {
        int objectType = config.at("type");

        switch(objectType) {
            case ObjectType::BUILDING:
                Parse_BuildingData(config, data);
                break;
            case ObjectType::UNIT:
                Parse_UnitData(config, data);
                break;
            case ObjectType::UTILITY:
                Parse_UtilityData(config, data);
                break;
            default:
                ENG_LOG_ERROR("Encountered an unrecognized object type when parsing GameObjectData (type = {})", objectType);
                throw std::runtime_error("");
        }

        Parse_GameObjectData(config, data);
    }

    void GameObjectData_FinalizeButtonDescriptions(GameObjectDataRef& dt) {
        FactionObjectDataRef data = std::dynamic_pointer_cast<FactionObjectData>(dt);
        if(data == nullptr)
            return;
        
        for(auto& page : data->gui_btns.pages) {
            for(auto& [idx, btn] : page) {
                if(btn.price == GUI::ActionButtonDescription::MissingVisuals()) {
                    GameObjectDataRef obj = Resources::LinkObject(glm::ivec3(ObjectTypeFromCommandID(btn.command_id), btn.payload_id, data->race), true);
                    btn.name    = obj->name;
                    btn.price   = obj->cost;
                    btn.icon    = obj->icon;
                }
            }
        }
    }

    //=======================================================================

    void Parse_GameObjectData(const nlohmann::json& config, GameObjectDataRef data) {
        data->objectType        = config.at("type");
        data->str_id            = config.at("str_id");
        data->size              = config.count("size") ? eng::json::parse_vec2(config.at("size")) : glm::vec2(1.f);
        data->navigationType    = config.at("nav_type");

        data->icon              = config.count("icon") ? json::parse_ivec2(config.at("icon")) : glm::ivec2(0);
        data->cost              = config.count("cost") ? ParseCost(config.at("cost")) : glm::ivec4(0);
        data->name              = config.at("name");
        data->initialized       = true;
    }

    void Parse_FactionObjectData(const nlohmann::json& config, FactionObjectData& data) {
        data.health         = config.at("health");
        data.mana           = config.at("mana");

        //construction related fields
        data.build_time     = config.at("build_time");
        data.upgrade_time   = config.count("upgrade_time") ? config.at("upgrade_time") : 0;

        //ingame object statistics
        data.basic_damage       = config.at("basic_damage");
        data.pierce_damage      = config.at("pierce_damage");
        data.attack_range       = config.at("attack_range");
        data.armor              = config.at("armor");
        data.vision_range       = config.at("vision_range");
        data.cooldown           = config.count("cooldown") ? config.at("cooldown") : 0.f;

        data.deathSoundIdx      = config.count("death_sound") ? config.at("death_sound") : 0;
        data.race               = config.count("race") ? config.at("race") : 0;

        //sounds
        if(config.count("sounds")) {
            auto& sounds = config.at("sounds");
            if(sounds.count("yes"))     data.sound_yes     = ParseSoundEffect(sounds.at("yes"));
        }

        //table of references to other object prefabs
        if(config.count("refs")) {
            auto& refs_json = config.at("refs");
            size_t count = std::min(data.refs.size(), refs_json.size());
            for(size_t i = 0; i < count; i++) {
                data.refs[i] = Resources::LinkObject(refs_json.at(i));
            }

            if(refs_json.size() > data.refs.size()) {
                ENG_LOG_WARN("Parse_FactionObjectData - too many references to other objects (some will be discarded).");
            }
        }

        //GUI button descriptions
        ParseButtonDescriptions(config, data);
    }

    void Parse_BuildingData(const nlohmann::json& config, GameObjectDataRef dt) {
        BuildingDataRef data = std::dynamic_pointer_cast<BuildingData>(dt);
        if(data == nullptr || config.at("type") != ObjectType::BUILDING) {
            ENG_LOG_ERROR("Parse_BuildingData - type mismatch.");
            throw std::runtime_error("");
        }
        ASSERT_MSG(data->str_id == config.at("str_id"), "Parse_BuildingData - provided data object was not initialized for this data entry.");

        //parse general FactionObject parameters
        Parse_FactionObjectData(config, (FactionObjectData&)*data.get());

        ParseAnimations_Building(config, data);

        data->traversable           = config.count("traversable") ? config.at("traversable") : false;
        data->gatherable            = config.count("gatherable") ? config.at("gatherable") : false;
        data->resource              = (config.count("resource") ? config.at("resource") : false) || data->gatherable;
        data->dropoff_mask          = config.count("dropoff_mask") ? config.at("dropoff_mask") : 0;
        data->attack_speed          = config.count("attack_speed") ? config.at("attack_speed") : 2.f;
    }

    void Parse_UnitData(const nlohmann::json& config, GameObjectDataRef dt) {
        UnitDataRef data = std::dynamic_pointer_cast<UnitData>(dt);
        if(data == nullptr || config.at("type") != ObjectType::UNIT) {
            ENG_LOG_ERROR("Parse_UnitData - type mismatch.");
            throw std::runtime_error("");
        }
        ASSERT_MSG(data->str_id == config.at("str_id"), "Parse_UnitData - provided data object was not initialized for this data entry.");

        //parse general FactionObject parameters
        Parse_FactionObjectData(config, (FactionObjectData&)*data.get());

        //animation parsing
        ParseAnimations_Unit(config, data);

        //sound paths parsing
        auto& sounds = config.at("sounds");
        if(sounds.count("attack"))  data->sound_attack  = ParseSoundEffect(sounds.at("attack"));
        if(sounds.count("ready"))   data->sound_ready   = ParseSoundEffect(sounds.at("ready"));
        if(sounds.count("what"))    data->sound_what    = ParseSoundEffect(sounds.at("what"));
        if(sounds.count("pissed"))  data->sound_pissed  = ParseSoundEffect(sounds.at("pissed"));

        if(config.count("worker")) data->worker = config.at("worker");
        if(config.count("caster")) data->caster = config.at("caster");

        data->speed = config.count("speed") ? config.at("speed") : 10;
        data->scale = config.count("scale") ? config.at("scale") : 1.f;

        data->upgrade_src = config.count("upgrade_src") ? json::parse_ivec2(config.at("upgrade_src")) : glm::ivec2(UnitUpgradeSource::NONE);
    }

    void Parse_UtilityData(const nlohmann::json& config, GameObjectDataRef dt) {
        UtilityObjectDataRef data = std::dynamic_pointer_cast<UtilityObjectData>(dt);
        if(data == nullptr || config.at("type") != ObjectType::UTILITY) {
            ENG_LOG_ERROR("Parse_UtilityData - type mismatch.");
            throw std::runtime_error("");
        }
        ASSERT_MSG(data->str_id == config.at("str_id"), "Parse_UtilityData - provided data object was not initialized for this data entry.");

        //utility object type ID + handlers
        data->utility_id = config.at("utility_id");
        ResolveUtilityHandlers(data, data->utility_id);

        //animation parsing
        ParseAnimations_Utility(config, data);

        if(config.count("sounds")) {
            auto& sounds = config.at("sounds");
            if(sounds.count("on_spawn")) data->on_spawn = ParseSoundEffect(sounds.at("on_spawn"));
        }

        //parse fields specific to different utility object types
        switch(data->utility_id) {
            case UtilityObjectType::PROJECTILE:
                data->duration = config.at("duration");
                break;
        }
    }

    //=======================================================================

    int ObjectTypeFromCommandID(int cmd_id) {
        switch(cmd_id) {
            case GUI::ActionButton_CommandType::CAST:
            case GUI::ActionButton_CommandType::RESEARCH:
                return ObjectType::UTILITY;
            case GUI::ActionButton_CommandType::BUILD:
            case GUI::ActionButton_CommandType::UPGRADE:
                return ObjectType::BUILDING;
            case GUI::ActionButton_CommandType::TRAIN:
                return ObjectType::UNIT;
            default:
                ENG_LOG_ERROR("GameObjectData_FinalizeButtonDescriptions - cannot retrieve object from payloadID when command={}.", cmd_id);
                throw std::runtime_error("");
        }
    }

    void ParseButtonDescriptions(const nlohmann::json& config, FactionObjectData& data) {
        if(config.count("buttons")) {
            int page_idx = 0;
            for(auto& page_entry : config.at("buttons")) {
                if((++page_idx) > 3) {
                    ENG_LOG_WARN("ParseConfig_Faction - object contains too many button page definitions (some will be discarded)");
                    break;
                }

                ButtonDescriptions::PageEntry page_data = {};
                for(auto& btn_entry : page_entry) {
                    int btn_idx = btn_entry.at("idx");
                    int cmd_id  = btn_entry.at("cmd");
                    int payload = btn_entry.count("pid") ? btn_entry.at("pid") : -1;
                    
                    int hotkey_idx = -1;
                    char hotkey = '-';
                    bool has_hotkey = false;
                    if(btn_entry.count("hotkey")) {
                        auto& hk = btn_entry.at("hotkey");
                        has_hotkey = true;
                        hotkey = std::string(hk.at(0)).at(0);
                        hotkey_idx = hk.at(1);
                    }
                    
                    std::string name = "";
                    glm::ivec2 icon = glm::ivec2(-1);
                    glm::ivec4 price = GUI::ActionButtonDescription::MissingVisuals();
                    if(btn_entry.count("visuals")) {
                        auto& visuals = btn_entry.at("visuals");
                        name = visuals.at(0);
                        icon = json::parse_ivec2(visuals.at(1));
                        price = (visuals.size() > 2) ? json::parse_ivec4(visuals.at(2)) : glm::ivec4(0);
                    }

                    page_data.push_back({ btn_idx, GUI::ActionButtonDescription(cmd_id, payload, has_hotkey, hotkey, hotkey_idx, name, icon, price) });
                }


                data.gui_btns.pages.push_back(page_data);
            }
        }
    }
    
    void ParseAnimations_Building(const nlohmann::json& config, BuildingDataRef& data) {
        int wo = Building::WinterSpritesOffset();
        std::string building_name = config.at("str_id");
        auto [name_prefix, name_suffix] = SplitName(building_name);
        
        std::map<int, SpriteGroup> animations = {};
        if(config.count("custom_sprites") && config.at("custom_sprites")) {
            //custom sprites loading
            std::string sprites_prefix = config.at("sprites_prefix");

            if(config.count("animations")) {
                for(auto& [anim_name, anim_data] : config.at("animations").items()) {
                    int anim_id         = ResolveBuildingAnimationID(anim_data.at("id"));
                    bool is_summer      = anim_data.count("summer")         ? anim_data.at("summer") : true;
                    bool both_weathers  = anim_data.count("both_weathers")  ? anim_data.at("both_weathers") : false;
                    bool anim_repeat    = anim_data.count("repeat")         ? anim_data.at("repeat") : false;

                    if(!is_summer && !both_weathers)
                        anim_id += wo;

                    std::string sprite_path = sprites_prefix + "/" + anim_name;
                    animations.insert({ anim_id, SpriteGroup(SpriteGroupData(anim_id, Resources::LoadSprite(sprite_path), anim_repeat, 1.f)) });

                    if(both_weathers)
                        anim_id += wo;
                    animations.insert({ anim_id, SpriteGroup(SpriteGroupData(anim_id, Resources::LoadSprite(sprite_path), anim_repeat, 1.f)) });
                }
            }
        }
        else {
            //animations list is optional as most buildings can name all animations from their name
            if(config.count("animations")) {
                for(auto& [anim_name, anim_data] : config.at("animations").items()) {
                    //animation ID - either given in JSON or derive from the animation name
                    int anim_id = anim_data.count("id") ? anim_data.at("id") : ResolveBuildingAnimationID(anim_name);

                    std::string sprite_name = anim_data.count("sprite_name") ? anim_data.at("sprite_name") : anim_name;
                    bool anim_repeat = anim_data.count("repeat") ? anim_data.at("repeat") : false;

                    LoadBuildingSprites(animations, anim_id, name_prefix, sprite_name, anim_repeat);
                }
            }

            //try to load defaults for each animation (unless it was explicity defined)
            if(!animations.count(BuildingAnimationType::IDLE)) {
                LoadBuildingSprites(animations, BuildingAnimationType::IDLE, name_prefix, name_suffix, false);
            }
            if(!animations.count(BuildingAnimationType::BUILD3)) {
                LoadBuildingSprites(animations, BuildingAnimationType::BUILD3, name_prefix, name_suffix + "_build", false, false);
            }
            if(!animations.count(BuildingAnimationType::UPGRADE)) {
                LoadBuildingSprites(animations, BuildingAnimationType::UPGRADE, name_prefix, name_suffix + "_upgrade", false, false);
            }

            //load construction sprites
            animations.insert({ BuildingAnimationType::BUILD1, SpriteGroup(SpriteGroupData(BuildingAnimationType::BUILD1, Resources::LoadSprite("misc/buildings/construction1"), false, 1.f)) });
            animations.insert({ BuildingAnimationType::BUILD2, SpriteGroup(SpriteGroupData(BuildingAnimationType::BUILD2, Resources::LoadSprite("misc/buildings/construction2"), false, 1.f)) });
            animations.insert({ BuildingAnimationType::BUILD1 + wo, SpriteGroup(SpriteGroupData(BuildingAnimationType::BUILD1 + wo, Resources::LoadSprite("misc/buildings/construction1_winter"), false, 1.f)) });
            animations.insert({ BuildingAnimationType::BUILD2 + wo, SpriteGroup(SpriteGroupData(BuildingAnimationType::BUILD2 + wo, Resources::LoadSprite("misc/buildings/construction2_winter"), false, 1.f)) });
        }
        data->animData = std::make_shared<AnimatorData>(building_name, std::move(animations));
    }

    void ParseAnimations_Unit(const nlohmann::json& config, UnitDataRef& data) {
        std::string unit_name = data->str_id;
        std::map<int, SpriteGroup> animations = {};
        for(auto& [anim_name, anim_data] : config.at("animations").items()) {
            //animation ID - either given in JSON or derive from the animation name
            int anim_id = anim_data.count("id") ? anim_data.at("id") : ResolveUnitAnimationID(anim_name);

            //path to the sprite - either explicitly defined in JSON or created by combining unit name & animation name
            std::string sprite_path = anim_data.count("sprite_path") ? anim_data.at("sprite_path") : unit_name;
            std::string sprite_name = anim_data.count("sprite_name") ? anim_data.at("sprite_name") : anim_name;
            
            float duration = anim_data.at("duration");
            bool repeat = anim_data.count("repeat") ? anim_data.at("repeat") : true;
            float keyframe = anim_data.count("keyframe") ? anim_data.at("keyframe") : 1.f;
            animations.insert({ anim_id, SpriteGroup(SpriteGroupData(anim_id, Resources::LoadSprite(sprite_path + "/" + sprite_name), repeat, duration, keyframe)) });
        }
        data->animData = std::make_shared<AnimatorData>(unit_name, std::move(animations));
    }

    void ParseAnimations_Utility(const nlohmann::json& config, UtilityObjectDataRef& data) {
        std::map<int, SpriteGroup> animations = {};
        if(config.count("animations")) {
            int anim_id = 0;
            for(auto& anim_data : config.at("animations")) {
                float duration = 1.f;
                float keyframe = 1.f;
                bool repeat = true;
                std::string sprite_name;
                if(anim_data.is_object()) {
                    sprite_name = anim_data.at("sprite_name");
                    repeat = anim_data.count("repeat") ? anim_data.at("repeat") : false;
                    keyframe = anim_data.count("keyframe") ? anim_data.at("keyframe") : 1.f;
                    duration = anim_data.count("duration") ? anim_data.at("duration") : 1.f;
                }
                else {
                    sprite_name = anim_data;
                }
                animations.insert({ anim_id, SpriteGroup(SpriteGroupData(anim_id, Resources::LoadSprite(sprite_name), repeat, duration, keyframe)) });
                anim_id++;
            }
        }
        data->animData = std::make_shared<AnimatorData>(config.at("name"), std::move(animations));
    }

    SoundEffect ParseSoundEffect(const nlohmann::json& json) {
        return json.is_array() ? SoundEffect(json.at(0), json.at(1)) : SoundEffect(json);
    }

    glm::ivec4 ParseCost(const nlohmann::json& data) {
        return data.is_array() ? ((data.size() == 4) ? json::parse_ivec4(data) : glm::ivec4(json::parse_ivec3(data), 0)) : glm::ivec4(0,0,0, (int)data);
    }

    int ResolveUnitAnimationID(const std::string& name) {
        constexpr static std::array<const char*,8> anim_names = { "idle", "walk", "action", "death", "idle2", "walk2", "idle3", "walk3" };

        const char* n = name.c_str();
        for(int i = 0; i < anim_names.size(); i++) {
            if(strncmp(n, anim_names[i], name.length()) == 0) {
                return i;
            }
        }

        return -1;
    }

    int ResolveBuildingAnimationID(const std::string& name) {
        constexpr static std::array<const char*,6> anim_names = { "idle", "idle2", "build1", "build2", "build3", "upgrade" };

        const char* n = name.c_str();
        for(int i = 0; i < anim_names.size(); i++) {
            if(strncmp(n, anim_names[i], name.length()) == 0) {
                return i;
            }
        }

        return -1;
    }

    std::pair<std::string,std::string> SplitName(const std::string& name) {
        using dtype = std::pair<std::string,std::string>;

        size_t pos = name.find_last_of("/");
        return (pos != std::string::npos) ? dtype{ name.substr(0, pos), name.substr(pos+1) } : dtype{ name, name };
    }
    
    void LoadBuildingSprites(std::map<int, SpriteGroup>& animations, int id, const std::string& prefix, const std::string& suffix, bool repeat, bool throwOnMiss) {
        //summer sprite
        std::string sprite_path = prefix + "/buildings_summer/" + suffix;
        if(throwOnMiss || Resources::SpriteExists(sprite_path)) {
            animations.insert({ id, SpriteGroup(SpriteGroupData(id, Resources::LoadSprite(sprite_path), repeat, 1.f)) });
        }

        //winter sprite
        sprite_path = prefix + "/buildings_winter/" + suffix;
        int wo = Building::WinterSpritesOffset();
        if(throwOnMiss || Resources::SpriteExists(sprite_path)) {
            animations.insert({ id + wo, SpriteGroup(SpriteGroupData(id + wo, Resources::LoadSprite(sprite_path), repeat, 1.f)) });
        }
    }

}//namespace eng
