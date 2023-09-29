#include "engine/game/object_data.h"

#include "engine/utils/json.h"
#include "engine/utils/utils.h"

#include "engine/game/resources.h"
#include "engine/game/command.h"
#include "engine/game/gameobject.h"
#include "engine/game/level.h"

#include "engine/utils/randomness.h"
#include "engine/core/input.h"

#include <tuple>

#define SOUNDS_PATH_PREFIX "res/sounds/Gamesfx"

namespace eng {

    GameObjectDataRef ParseConfig_Object(const nlohmann::json& config, GameObjectData::referencesMapping& refMapping);

    GameObjectDataRef ParseConfig_Building(const nlohmann::json& config);
    GameObjectDataRef ParseConfig_Unit(const nlohmann::json& config);
    void ParseConfig_FactionObject(const nlohmann::json& config, FactionObjectData& data);
    GameObjectDataRef ParseConfig_Utility(const nlohmann::json& config);

    SoundEffect ParseSoundEffect(const nlohmann::json& json);

    int ResolveUnitAnimationID(const std::string& name);
    int ResolveBuildingAnimationID(const std::string& name);
    std::pair<std::string,std::string> SplitName(const std::string& name);
    void LoadBuildingSprites(std::map<int, SpriteGroup>& animations, int id, const std::string& prefix, const std::string& suffix, bool repeat, bool throwOnMiss = true);
    void ResolveUtilityHandlers(UtilityObjectDataRef& data, int id);

    //==================================

    void UtilityHandler_Default_Render(UtilityObject& obj);

    void UtilityHandler_Projectile_Init(UtilityObject& obj, FactionObject& src);
    bool UtilityHandler_Projectile_Update(UtilityObject& obj, Level& level);

    void UtilityHandler_Corpse_Init(UtilityObject& obj, FactionObject& src);
    bool UtilityHandler_Corpse_Update(UtilityObject& obj, Level& level);
    void UtilityHandler_Corpse_Render(UtilityObject& obj);

    //===== ObjectID =====

    std::string ObjectID::to_string() const { 
        char buf[512];
        snprintf(buf, sizeof(buf), "(%zd, %zd, %zd)", type, idx, id);
        return std::string(buf);
    }

    std::ostream& operator<<(std::ostream& os, const ObjectID& id) {
        os << id.to_string();
        return os;
    }

    bool operator==(const ObjectID& lhs, const ObjectID& rhs) {
        return lhs.id == rhs.id && lhs.idx == rhs.idx && lhs.type == rhs.type;
    }

    bool ObjectID::IsAttackable(const ObjectID& id) {
        return ((unsigned int)(id.type - 1) < ObjectType::UTILITY) || (id.type == ObjectType::MAP_OBJECT && IsWallTile(id.id));
    }

    bool ObjectID::IsObject(const ObjectID& id) {
        return ((unsigned int)(id.type - 1) < ObjectType::UTILITY);
    }

    bool ObjectID::IsHarvestable(const ObjectID& id) {
        return (id.type == ObjectType::MAP_OBJECT && id.id == TileType::TREES);
    }

    //===== SoundEffect =====

    std::string SoundEffect::Random() const {
        char buf[512];
        if(variations > 0)
            snprintf(buf, sizeof(buf), "%s/%s%d.wav", SOUNDS_PATH_PREFIX, path.c_str(), Random::UniformInt(1, variations));
        else
            snprintf(buf, sizeof(buf), "%s/%s.wav", SOUNDS_PATH_PREFIX, path.c_str());
        return std::string(buf);
    }

    std::string SoundEffect::GetPath(const char* name) {
        char buf[512];
        snprintf(buf, sizeof(buf), "%s/%s.wav", SOUNDS_PATH_PREFIX, name);
        return std::string(buf);
    }

    SoundEffect& SoundEffect::Wood() {
        static SoundEffect sound = SoundEffect("misc/tree", 4);
        return sound;
    }

    //===== FactionObjectData =====

    void FactionObjectData::SetupObjectReferences(const referencesRecord& ref_names) {
        for(int i = 0; i < ref_names.second; i++) {
            refs[i] = Resources::LoadObjectReference(ref_names.first[i]);
        }
    }

    //=============================

    GameObjectDataRef ParseConfig_Object(const nlohmann::json& config, GameObjectData::referencesMapping& refMapping) {
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

        //table of references to other object prefabs (only names loading here)
        if(config.count("refs")) {
            GameObjectData::referencesRecord refs = {};

            auto& refs_json = config.at("refs");
            refs.second = std::min(refs_json.size(), refs.first.size());

            for(int i = 0; i < refs.second; i++) {
                refs.first[i] = refs_json.at(i);
            }

            if(refs_json.size() > refs.first.size()) {
                ENG_LOG_WARN("ParseConfig_Object - too many references to other objects (some will be discarded).");
            }

            refMapping.push_back({data->name, refs });
        }

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

        if(config.count("custom_sprites") && config.at("custom_sprites")) {
            //custom sprites loading
            std::string sprites_prefix = config.at("sprites_prefix");

            if(config.count("animations")) {
                for(auto& [anim_name, anim_data] : config.at("animations").items()) {
                    int anim_id = ResolveBuildingAnimationID(anim_data.at("id"));
                    bool is_summer = anim_data.count("summer") ? anim_data.at("summer") : true;
                    bool both_weathers = anim_data.count("both_weathers") ? anim_data.at("both_weathers") : false;
                    bool anim_repeat = anim_data.count("repeat") ? anim_data.at("repeat") : false;

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

        //parse general unit/building parameters
        ParseConfig_FactionObject(config, (FactionObjectData&)*data.get());

        data->traversable = config.count("traversable") ? config.at("traversable") : false;
        data->gatherable = config.count("gatherable") ? config.at("gatherable") : false;
        data->dropoff_mask = config.count("dropoff_mask") ? config.at("dropoff_mask") : 0;
        data->attack_speed = config.count("attack_speed") ? config.at("attack_speed") : 2.f;

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
            float keyframe = anim_data.count("keyframe") ? anim_data.at("keyframe") : 1.f;
            animations.insert({ anim_id, SpriteGroup(SpriteGroupData(anim_id, Resources::LoadSprite(sprite_path + "/" + sprite_name), repeat, duration, keyframe)) });
        }
        data->animData = std::make_shared<AnimatorData>(unit_name, std::move(animations));

        //parse general unit/building parameters
        ParseConfig_FactionObject(config, (FactionObjectData&)*data.get());

        //sound paths parsing
        auto& sounds = config.at("sounds");
        if(sounds.count("attack"))  data->sound_attack  = ParseSoundEffect(sounds.at("attack"));
        if(sounds.count("ready"))   data->sound_ready   = ParseSoundEffect(sounds.at("ready"));
        if(sounds.count("yes"))     data->sound_yes     = ParseSoundEffect(sounds.at("yes"));
        if(sounds.count("what"))    data->sound_what    = ParseSoundEffect(sounds.at("what"));
        if(sounds.count("pissed"))  data->sound_pissed  = ParseSoundEffect(sounds.at("pissed"));

        if(config.count("worker")) data->worker = config.at("worker");

        return std::static_pointer_cast<GameObjectData>(data);
    }

    void ParseConfig_FactionObject(const nlohmann::json& config, FactionObjectData& data) {
        data.health = config.at("health");
        data.mana = config.at("mana");

        data.build_time = config.at("build_time");
        data.upgrade_time = config.count("upgrade_time") ? config.at("upgrade_time") : 0;
        data.cost = eng::json::parse_ivec3(config.at("cost"));

        data.basic_damage = config.at("basic_damage");
        data.pierce_damage = config.at("pierce_damage");
        data.attack_range = config.at("attack_range");
        data.armor = config.at("armor");
        data.vision_range = config.at("vision_range");
        data.deathSoundIdx = config.count("death_sound") ? config.at("death_sound") : 0;

        data.cooldown = config.count("cooldown") ? config.at("cooldown") : 0.f;
        data.race = config.count("race") ? config.at("race") : 0;
    }

    GameObjectDataRef ParseConfig_Utility(const nlohmann::json& config) {
        UtilityObjectDataRef data = std::make_shared<UtilityObjectData>();

        //utility object type ID + handlers
        data->utility_id = config.at("utility_id");
        ResolveUtilityHandlers(data, data->utility_id);

        //animation parsing
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

        switch(data->utility_id) {
            case UtilityObjectType::PROJECTILE:
                data->duration = config.at("duration");
                break;
        }

        return std::static_pointer_cast<GameObjectData>(data);
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

    SoundEffect ParseSoundEffect(const nlohmann::json& json) {
        return json.is_array() ? SoundEffect(json.at(0), json.at(1)) : SoundEffect(json);
    }

    void ResolveUtilityHandlers(UtilityObjectDataRef& data, int id) {
        using handlerRefs = std::tuple<UtilityObjectData::InitHandlerFn, UtilityObjectData::UpdateHandlerFn, UtilityObjectData::RenderHandlerFn>;

        switch(id) {
            case UtilityObjectType::PROJECTILE:
                std::tie(data->Init, data->Update, data->Render) = handlerRefs{ UtilityHandler_Projectile_Init, UtilityHandler_Projectile_Update, UtilityHandler_Default_Render };
                break;
            case UtilityObjectType::CORPSE:
                std::tie(data->Init, data->Update, data->Render) = handlerRefs{ UtilityHandler_Corpse_Init, UtilityHandler_Corpse_Update, UtilityHandler_Corpse_Render };
                break;
            default:
                ENG_LOG_ERROR("UtilityObject - invalid utility type ID ({})", id);
                throw std::runtime_error("");
        }
    }

    //======================================================

    void UtilityHandler_Default_Render(UtilityObject& obj) {
        obj.RenderAt(obj.real_pos(), obj.real_size());
    }

    void UtilityHandler_Projectile_Init(UtilityObject& obj, FactionObject& src) {
        UtilityObject::LiveData& d = obj.LD();

        //set projectile orientation, start & end time, starting position and copy unit's damage values
        glm::vec2 target_dir = d.target_pos - d.source_pos;
        d.i1 = VectorOrientation(target_dir / glm::length(target_dir));
        d.f1 = d.f2 = (float)Input::CurrentTime();
        obj.real_pos() = glm::vec2(src.Position());
        obj.real_size() = obj.Data()->size;
        d.i2 = src.BasicDamage();
        d.i3 = src.PierceDamage();
    }

    bool UtilityHandler_Projectile_Update(UtilityObject& obj, Level& level) {
        UtilityObject::LiveData& d = obj.LD();
        Input& input = Input::Get();

        obj.act() = 0;
        obj.ori() = d.i1;

        d.f2 += input.deltaTime;
        float t = (d.f2 - d.f1) / obj.UData()->duration;

        obj.real_pos() = d.InterpolatePosition(t);
        if(t >= 1.f) {
            bool attack_happened = ApplyDamage(level, d.i2, d.i3, d.targetID, d.target_pos);
            return true;
        }

        return false;
    }

    namespace CorpseAnimID { enum { CORPSE1_HU = 0, CORPSE1_OC, CORPSE2, CORPSE_WATER, RUINS_GROUND, RUINS_GROUND_WASTELAND, RUINS_WATER, EXPLOSION }; }

    void UtilityHandler_Corpse_Init(UtilityObject& obj, FactionObject& src) {
        UtilityObject::LiveData& d = obj.LD();
        AnimatorDataRef anim = obj.Data()->animData;

        //store object's orientation
        d.i4 = src.Orientation();
        //TODO: maybe round up the orientation, as there are usually only 2 directions for dying animations

        obj.real_pos() = glm::vec2(src.Position());
        obj.real_size() = obj.Data()->size;

        //1st & 2nd anim indices
        d.i1 = src.DeathAnimIdx();
        d.i2 = -1;

        if(d.i1 < 0) {
            //1st anim has invalid idx -> play explosion animation
            d.i3 = 1;
            d.f2 = Input::GameTimeDelay(anim->GetGraphics(CorpseAnimID::EXPLOSION).Duration());

            if(!src.IsUnit()) {
                //src is a building -> display crater animation (along with explosion)
                int is_wasteland = int(obj.lvl()->map.GetTileset()->GetType() == TilesetType::WASTELAND);
                d.i1 = (src.NavigationType() == NavigationBit::GROUND) ? (CorpseAnimID::RUINS_GROUND + is_wasteland) : CorpseAnimID::RUINS_WATER;
                d.f1 = Input::GameTimeDelay(anim->GetGraphics(d.i1).Duration());

                obj.real_pos() = glm::vec2(src.Position()) + (src.Data()->size - glm::vec2(2.f)) * 0.5f;
                obj.real_size() = glm::vec2(2.f);
            }
        }
        else {
            d.f1 = Input::GameTimeDelay(anim->GetGraphics(d.i1).Duration() + 1.f);
            if(src.IsUnit()) {
                //2nd animation - transitioned to from the 1st one
                if(src.NavigationType() == NavigationBit::GROUND) {
                    d.i2 = CorpseAnimID::CORPSE1_HU + src.Race();
                    d.f2 =  anim->GetGraphics(d.i2).Duration();
                }
                else if(src.NavigationType() == NavigationBit::WATER) {
                    d.i2 = CorpseAnimID::CORPSE_WATER;
                    d.f2 = anim->GetGraphics(d.i2).Duration();
                }
            }
        }

        // ENG_LOG_TRACE("CORPSE OBJECT - INIT: {}, {}, {}, {}, {}", d.i1, d.i2, d.i3, d.f1, d.f2);
    }

    bool UtilityHandler_Corpse_Update(UtilityObject& obj, Level& level) {
        UtilityObject::LiveData& d = obj.LD();
        Input& input = Input::Get();

        float t = (float)Input::CurrentTime();

        //switch from 1st to 2nd animation when the time runs out
        if(t >= d.f1) {
            d.i1 = d.i2;
            d.f1 = t + d.f2;
            d.i2 = -1;

            //dying ground unit - queue 3rd animation (generic, decayed corpse)
            if(d.i1 == CorpseAnimID::CORPSE1_HU || d.i1 == CorpseAnimID::CORPSE1_OC) {
                d.i2 = CorpseAnimID::CORPSE2;
                d.f2 = obj.Data()->animData->GetGraphics(d.i2).Duration();
            }
        }

        //terminate explosion animation
        if(d.i3 && t >= d.f2) {
            d.i3 = 0;
        }
        
        obj.act() = d.i1;
        obj.ori() = d.i4;

        //terminate if ran out of animations to play
        return (d.i1 < 0);
    }

    void UtilityHandler_Corpse_Render(UtilityObject& obj) {
        //regular corpse animation
        if(obj.LD().i1 >= 0) {
            obj.RenderAt(obj.real_pos(), obj.real_size());
        }
        
        //explosion animation
        if(obj.LD().i3) {
            int prev_id = obj.act();
            obj.act() = CorpseAnimID::EXPLOSION;
            obj.RenderAt(obj.real_pos(), obj.real_size(), -1e-3f);
            obj.act() = prev_id;
        }
    }

}//namespace eng
