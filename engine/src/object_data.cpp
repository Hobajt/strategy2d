#include "engine/game/object_data.h"

#include "engine/utils/randomness.h"
#include "engine/game/map.h"

#define SOUNDS_PATH_PREFIX "res/sounds/Gamesfx"

namespace eng {

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

}//namespace eng
