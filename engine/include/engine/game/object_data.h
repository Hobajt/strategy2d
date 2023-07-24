#pragma once

#include "engine/utils/mathdefs.h"
#include "engine/core/animator.h"

#include <string>
#include <memory>
#include <ostream>

namespace eng {

    //===== ObjectID =====

    namespace ObjectType { enum { INVALID, GAMEOBJECT, UNIT, BUILDING, UTILITY }; }

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

        //for logging purposes
        std::string to_string() const;
        friend std::ostream& operator<<(std::ostream& os, const ObjectID& id);
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
    };

    //===== GameObjectData =====

    struct GameObjectData {
        std::string name;
        int objectType;

        glm::vec2 size;
        int navigationType;

        AnimatorDataRef animData = nullptr;
    public:
        virtual ~GameObjectData() = default;
        virtual int MaxHealth() const { return 0; }
    };
    using GameObjectDataRef = std::shared_ptr<GameObjectData>;

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
    public:
        virtual int MaxHealth() const override { return health; }

        bool CanAttack() const { return basic_damage + pierce_damage > 0; }
    };
    using FactionObjectDataRef = std::shared_ptr<FactionObjectData>;

    //===== UnitData =====

    struct UnitData : public FactionObjectData {
        SoundEffect sound_attack;
        SoundEffect sound_ready;
        SoundEffect sound_yes;
        SoundEffect sound_what;
        SoundEffect sound_pissed;
    public:
        const SoundEffect& AttackSound() const { return sound_attack; }
    };
    using UnitDataRef = std::shared_ptr<UnitData>;

    //===== BuildingData =====

    struct BuildingData : public FactionObjectData {
    };
    using BuildingDataRef = std::shared_ptr<BuildingData>;

}//namespace eng
