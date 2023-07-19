#include "engine/game/object_data.h"

#include "engine/utils/json.h"
#include "engine/utils/utils.h"

#include "engine/game/resources.h"
#include "engine/game/command.h"
#include "engine/game/gameobject.h"

namespace eng {

    GameObjectDataRef ParseConfig_Object(const nlohmann::json& config);

    GameObjectDataRef ParseConfig_Building(const nlohmann::json& config);
    GameObjectDataRef ParseConfig_Unit(const nlohmann::json& config);
    GameObjectDataRef ParseConfig_Utility(const nlohmann::json& config);

    int ResolveUnitAnimationID(const std::string& name);
    int ResolveBuildingAnimationID(const std::string& name);
    std::pair<std::string,std::string> SplitName(const std::string& name);
    void LoadBuildingSprites(std::map<int, SpriteGroup>& animations, int id, const std::string& prefix, const std::string& suffix, bool repeat);

    //===== ObjectID =====

    std::ostream& operator<<(std::ostream& os, const ObjectID& id) {
        os << "(" << id.type << "," << id.idx << "," << id.id << ")";
        return os;
    }

    //=============================

    GameObjectDataRef ParseConfig_Object(const nlohmann::json& config) {
        GameObjectDataRef data = nullptr;

        int objectType = config.at("type");

        //proper type instantiation & specific fields parsing
        switch(objectType) {
            case ObjectType::BUILDING:
                data = ParseConfig_Building(config);
                break;
            case ObjectType::UNIT:
                data = ParseConfig_Unit(config);
                break;
            case ObjectType::UTILITY:
                data = ParseConfig_Utility(config);
                break;
            default:
                ENG_LOG_ERROR("Encountered an unrecognized object type when parsing GameObjectData (type = {})", objectType);
                throw std::runtime_error("");
        }

        //parsing generic gameobject fields

        data->objectType = objectType;
        data->name = config.at("name");
        data->size = config.count("size") ? eng::json::parse_vec2(config.at("size")) : glm::vec2(1.f);
        data->navigationType = config.at("nav_type");

        ASSERT_MSG(data != nullptr, "ObjectData parsing failed.");
        return data;
    }

    GameObjectDataRef ParseConfig_Building(const nlohmann::json& config) {
        BuildingDataRef data = std::make_shared<BuildingData>();

        std::string building_name = config.at("name");

        auto [name_prefix, name_suffix] = SplitName(building_name);
        int wo = Building::WinterSpritesOffset();

        //animation parsing
        std::map<int, SpriteGroup> animations = {};

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
            try {
                LoadBuildingSprites(animations, BuildingAnimationType::BUILD3, name_prefix, name_suffix + "_build", false);
            } catch(std::exception&) {}
        }
        if(!animations.count(BuildingAnimationType::UPGRADE)) {
            try {
                LoadBuildingSprites(animations, BuildingAnimationType::UPGRADE, name_prefix, name_suffix + "_upgrade", false);
            } catch(std::exception&) {}
        }

        //load construction sprites
        animations.insert({ BuildingAnimationType::BUILD1, SpriteGroup(SpriteGroupData(BuildingAnimationType::BUILD1, Resources::LoadSprite("misc/buildings/construction1"), false, 1.f)) });
        animations.insert({ BuildingAnimationType::BUILD2, SpriteGroup(SpriteGroupData(BuildingAnimationType::BUILD2, Resources::LoadSprite("misc/buildings/construction2"), false, 1.f)) });
        animations.insert({ BuildingAnimationType::BUILD1 + wo, SpriteGroup(SpriteGroupData(BuildingAnimationType::BUILD1 + wo, Resources::LoadSprite("misc/buildings/construction1_winter"), false, 1.f)) });
        animations.insert({ BuildingAnimationType::BUILD2 + wo, SpriteGroup(SpriteGroupData(BuildingAnimationType::BUILD2 + wo, Resources::LoadSprite("misc/buildings/construction2_winter"), false, 1.f)) });

        data->animData = std::make_shared<AnimatorData>(building_name, std::move(animations));

        return std::static_pointer_cast<GameObjectData>(data);
    }
    
    GameObjectDataRef ParseConfig_Unit(const nlohmann::json& config) {
        UnitDataRef data = std::make_shared<UnitData>();

        std::string unit_name = config.at("name");

        //animation parsing
        std::map<int, SpriteGroup> animations = {};
        for(auto& [anim_name, anim_data] : config.at("animations").items()) {
            //animation ID - either given in JSON or derive from the animation name
            int anim_id = anim_data.count("id") ? anim_data.at("id") : ResolveUnitAnimationID(anim_name);

            //path to the sprite - either explicitly defined in JSON or created by combining unit name & animation name
            std::string sprite_path = anim_data.count("sprite_path") ? anim_data.at("sprite_path") : unit_name;
            std::string sprite_name = anim_data.count("sprite_name") ? anim_data.at("sprite_name") : anim_name;
            
            float duration = anim_data.at("duration");
            bool repeat = anim_data.count("repeat") ? anim_data.at("repeat") : true;
            animations.insert({ anim_id, SpriteGroup(SpriteGroupData(anim_id, Resources::LoadSprite(sprite_path + "/" + sprite_name), repeat, duration)) });
        }

        data->animData = std::make_shared<AnimatorData>(unit_name, std::move(animations));

        return std::static_pointer_cast<GameObjectData>(data);
    }

    GameObjectDataRef ParseConfig_Utility(const nlohmann::json& config) {
        // BuildingDataRef data = std::make_shared<BuildingData>();
        BuildingDataRef data = nullptr;

        return std::static_pointer_cast<GameObjectData>(data);
    }

    int ResolveUnitAnimationID(const std::string& name) {
        constexpr static std::array<const char*,4> anim_names = { "idle", "walk", "action", "death" };

        const char* n = name.c_str();
        for(int i = 0; i < anim_names.size(); i++) {
            if(strncmp(n, anim_names[i], name.length()) == 0) {
                return i;
            }
        }

        return -1;
    }

    int ResolveBuildingAnimationID(const std::string& name) {
        constexpr static std::array<const char*,5> anim_names = { "idle", "build1", "build2", "build3", "upgrade" };

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
    
    void LoadBuildingSprites(std::map<int, SpriteGroup>& animations, int id, const std::string& prefix, const std::string& suffix, bool repeat) {
        //summer sprite
        std::string sprite_path = prefix + "/buildings_summer/" + suffix;
        animations.insert({ id, SpriteGroup(SpriteGroupData(id, Resources::LoadSprite(sprite_path), repeat, 1.f)) });

        //winter sprite
        sprite_path = prefix + "/buildings_winter/" + suffix;
        int wo = Building::WinterSpritesOffset();
        animations.insert({ id + wo, SpriteGroup(SpriteGroupData(id + wo, Resources::LoadSprite(sprite_path), repeat, 1.f)) });
    }


}//namespace eng
