#pragma once

#include "engine/utils/mathdefs.h"
#include "engine/core/animator.h"
#include "engine/core/gui_action_buttons.h"

#include <string>
#include <memory>
#include <ostream>
#include <utility>
#include <vector>
#include <array>

namespace eng {

    namespace BuildingType {
        enum { 
            TOWN_HALL, BARRACKS, FARM, LUMBER_MILL, BLACKSMITH, TOWER, 
            SHIPYARD, FOUNDRY, OIL_REFINERY, OIL_PLATFORM,
            INVENTOR, STABLES, CHURCH, WIZARD_TOWER, DRAGON_ROOST,
            GUARD_TOWER, CANNON_TOWER,
            KEEP, CASTLE,
            COUNT
        };

        enum {
            GOLD_MINE = 101,
            OIL_PATCH = 102,
        };
    }

    namespace UnitType {
        enum {
            PEASANT, FOOTMAN, ARCHER, RANGER, BALLISTA, KNIGHT, PALADIN,
            TANKER, DESTROYER, BATTLESHIP, SUBMARINE, TRANSPORT,
            ROFLCOPTER, DEMOSQUAD,
            MAGE,
            GRYPHON,
            COUNT
        };

        enum {
            SEAL = 101,
            PIG = 102,
            SHEEP = 103,
            SKELETON = 104,
            EYE = 105
        };
    }

    //===== ObjectID =====

    namespace ObjectType { enum { INVALID = 0, GAMEOBJECT, UNIT, BUILDING, UTILITY, MAP_OBJECT }; }

    //Unique identifier for any object within the ObjectPool.
    struct ObjectID {
        using dtype = size_t;
    public:
        dtype type = ObjectType::INVALID;
        dtype idx;
        dtype id;
    public:
        ObjectID() = default;
        ObjectID(dtype type_, dtype idx_, dtype id_) : type(type_), idx(idx_), id(id_) {}
        ObjectID(const glm::ivec3& v) : type(v[0]), idx(v[1]), id(v[2]) {}

        static bool IsValid(const ObjectID& id) { return id.type != ObjectType::INVALID; }

        //True if ID describes a FactionObject or a MapObject which is a wall.
        static bool IsAttackable(const ObjectID& id);

        //true if it describes a unit/building/gameobject.
        static bool IsObject(const ObjectID& id);

        static bool IsHarvestable(const ObjectID& id);

        //for logging purposes
        std::string to_string() const;
        friend std::ostream& operator<<(std::ostream& os, const ObjectID& id);
        friend bool operator==(const ObjectID& lhs, const ObjectID& rhs);
        friend bool operator!=(const ObjectID& lhs, const ObjectID& rhs);
    };

    //===== SoundEffect =====

    struct SoundEffect {
        bool valid = false;
        std::string path;
        int variations;
    public:
        SoundEffect() = default;
        SoundEffect(const std::string& path_, int variations_ = 0) : valid(true), path(path_), variations(variations_) {}

        //Composes path for random variation of given sound effect.
        std::string Random() const;

        static std::string GetPath(const char* name);
        
        static SoundEffect& Wood();
        static SoundEffect& Error();
        static SoundEffect& Dock();
    };

    //===== GameObjectData =====

    struct GameObjectData;
    using GameObjectDataRef = std::shared_ptr<GameObjectData>;

    struct GameObjectData {
    public:
        std::string str_id;         //string identifier  ... format usually smth like "human/town_hall"
        glm::ivec3  num_id;         //numeric identifier ... format {UNIT|BUILDING|UTILITY, type, race}
        int objectType;
        bool initialized = false;

        std::string name;
        glm::ivec4 cost;
        glm::ivec2 icon;

        glm::vec2 size;
        int navigationType;

        float anim_speed;

        AnimatorDataRef animData = nullptr;
    public:
        virtual ~GameObjectData() = default;
        virtual int MaxHealth() const { return 0; }

        void SetupID(const std::string& str_id_, const glm::ivec3& num_id_) { str_id = str_id_; num_id = num_id_; }
    };

    struct ButtonDescriptions {
        using PageEntry = std::vector<std::pair<int, GUI::ActionButtonDescription>>;
        std::vector<PageEntry> pages;
    };

    //===== FactionObjectData =====

    struct UtilityObjectData;
    using UtilityObjectDataRef = std::shared_ptr<UtilityObjectData>;

    struct FactionObjectData : public GameObjectData {
        int health;

        int build_time;
        int upgrade_time;

        int basic_damage;
        int pierce_damage;
        int attack_range;
        int armor;
        int vision_range;

        float cooldown;

        int deathSoundIdx;      //identifies what sound to play on death (from 4 preset sounds)
        int race;

        bool invulnerable;

        UtilityObjectDataRef projectile = nullptr;
        ButtonDescriptions gui_btns;

        SoundEffect sound_yes;
    public:
        virtual int MaxHealth() const override { return health; }
        virtual bool IntegrateInMap() const { return true; }
        virtual bool IsNotableObject() const { return false; }

        bool CanAttack() const { return basic_damage + pierce_damage > 0; }
    };
    using FactionObjectDataRef = std::shared_ptr<FactionObjectData>;

    //===== UnitData =====

    namespace UnitUpgradeSource { enum { NONE = 0, BLACKSMITH, FOUNDRY, CASTER, LUMBERMILL, BLACKSMITH_BALLISTA, COUNT }; }

    struct UnitData : public FactionObjectData {
        SoundEffect sound_attack;
        SoundEffect sound_ready;
        SoundEffect sound_what;
        SoundEffect sound_pissed;

        bool worker = false;
        bool caster = false;
        bool siege = false;
        bool transport = false;
        bool attack_rotated = false;

        int deathAnimIdx = -1;

        float speed;
        float scale = 1.f;

        glm::ivec2 upgrade_src;
    };
    using UnitDataRef = std::shared_ptr<UnitData>;

    //===== BuildingData =====

    struct BuildingData : public FactionObjectData {
        bool traversable = false;           //detaches building from pathfinding
        bool gatherable = false;            //building is treated as a resource (workers can enter it to mine it)
        bool resource = false;
        bool coastal = false;               //building must be placed on at least 1 transition tile between ground & water
        bool requires_foundation = false;   //building can only be built on specific sites (oil patch for example)
        int dropoff_mask = 0;
        float attack_speed = 2.f;
    public:
        virtual bool IntegrateInMap() const override { return !traversable; }
        virtual bool IsNotableObject() const override { return requires_foundation; }
    };
    using BuildingDataRef = std::shared_ptr<BuildingData>;

    //===== UtilityObjectData =====

    namespace UtilityObjectType {
        enum { INVALID = -1, PROJECTILE, CORPSE, VISUALS, SPELL, BUFF, COUNT };
    }

    class UtilityObject;
    class FactionObject;
    class Level;

    struct UtilityObjectData : public GameObjectData {
        typedef void(*InitHandlerFn)(UtilityObject& obj, FactionObject* src);
        typedef bool(*UpdateHandlerFn)(UtilityObject& obj, Level& level);
        typedef void(*RenderHandlerFn)(UtilityObject& obj);
    public:
        int utility_id = UtilityObjectType::INVALID;
        
        InitHandlerFn Init = nullptr;
        UpdateHandlerFn Update = nullptr;
        RenderHandlerFn Render = nullptr;

        float duration = 1.f;
        float scale = 1.f;

        SoundEffect on_spawn;
        SoundEffect on_done;

        bool spawn_followup = false;
        std::string followup_name;

        //generic fields - content is specific to individual UtilityObject types (look in object_parsing.cpp for details)
        bool b1, b2, b3, b4;
        int i1, i2, i3, i4, i5;
        float f1;
    public:
        bool Projectile_IsAutoGuided() const { return (i1 == 0); }
        int Projectile_SplashRadius() const { return i1; }
    };
    using UtilityObjectDataRef = std::shared_ptr<UtilityObjectData>;

}//namespace eng

template <>
struct std::hash<eng::ObjectID> {
    std::size_t operator()(const eng::ObjectID& k) const {
        return ((std::hash<eng::ObjectID::dtype>{}(k.type) ^ (std::hash<eng::ObjectID::dtype>{}(k.idx) << 1) >> 1) ^ std::hash<eng::ObjectID::dtype>{}(k.id));
    }
};
