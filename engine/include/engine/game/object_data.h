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

#define MAX_OBJECT_REFS 6

namespace eng {

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
    };

    //===== GameObjectData =====

    struct GameObjectData;
    using GameObjectDataRef = std::shared_ptr<GameObjectData>;

    struct GameObjectData {
        using referencesRecord = std::pair<std::array<std::string, MAX_OBJECT_REFS>, int>;
        using referencesMapping = std::vector<std::pair<std::string, referencesRecord>>;
        using objectReferences = std::array<GameObjectDataRef, MAX_OBJECT_REFS>;
    public:
        std::string name;
        int objectType;

        glm::vec2 size;
        int navigationType;

        AnimatorDataRef animData = nullptr;
    public:
        virtual ~GameObjectData() = default;
        virtual int MaxHealth() const { return 0; }

        //Used during resources initialization.
        virtual void SetupObjectReferences(const referencesRecord& refs) {}
    };

    struct ButtonDescriptions {
        using PageEntry = std::vector<std::pair<int, GUI::ActionButtonDescription>>;
        std::vector<PageEntry> pages;
    };

    //===== FactionObjectData =====

    struct FactionObjectData : public GameObjectData {
        int health;
        int mana;

        int build_time;
        int upgrade_time;
        glm::ivec3 cost;

        int basic_damage;
        int pierce_damage;
        int attack_range;
        int armor;
        int vision_range;

        float cooldown;

        int deathSoundIdx;      //identifies what sound to play on death (from 4 preset sounds)
        int race;
        glm::ivec2 icon;

        objectReferences refs;
        ButtonDescriptions gui_btns;

        SoundEffect sound_yes;
    public:
        virtual int MaxHealth() const override { return health; }
        virtual bool IntegrateInMap() const { return true; }

        bool CanAttack() const { return basic_damage + pierce_damage > 0; }

        virtual void SetupObjectReferences(const referencesRecord& refs) override;
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

        int deathAnimIdx = -1;

        glm::ivec2 upgrade_src;
    };
    using UnitDataRef = std::shared_ptr<UnitData>;

    //===== BuildingData =====

    struct BuildingData : public FactionObjectData {
        bool traversable = false;       //detaches building from pathfinding
        bool gatherable = false;        //building is treated as a resource (workers can enter it to mine it)
        int dropoff_mask = 0;
        float attack_speed = 2.f;
    public:
        virtual bool IntegrateInMap() const override { return !traversable; }
    };
    using BuildingDataRef = std::shared_ptr<BuildingData>;

    //===== UtilityObjectData =====

    namespace UtilityObjectType {
        enum { INVALID = -1, PROJECTILE, CORPSE, COUNT };
    }

    class UtilityObject;
    class FactionObject;
    class Level;

    struct UtilityObjectData : public GameObjectData {
        typedef void(*InitHandlerFn)(UtilityObject& obj, FactionObject& src);
        typedef bool(*UpdateHandlerFn)(UtilityObject& obj, Level& level);
        typedef void(*RenderHandlerFn)(UtilityObject& obj);
    public:
        int utility_id = UtilityObjectType::INVALID;
        
        InitHandlerFn Init = nullptr;
        UpdateHandlerFn Update = nullptr;
        RenderHandlerFn Render = nullptr;

        float duration = 1.f;

        SoundEffect on_spawn;
    };
    using UtilityObjectDataRef = std::shared_ptr<UtilityObjectData>;

}//namespace eng
