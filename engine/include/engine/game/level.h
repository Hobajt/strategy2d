#pragma once

#include <vector>

#include "engine/utils/mathdefs.h"

#include "engine/game/gameobject.h"
#include "engine/game/faction.h"
#include "engine/game/map.h"
#include "engine/game/object_pool.h"
#include "engine/game/player_controller.h"

namespace eng {

    //===== EndCondition =====

    namespace EndConditionType { enum { WIN, LOSE }; }

    class EndCondition {
    public:
        EndCondition() = default;
        EndCondition(const std::vector<ObjectID>& objects, bool any);
        EndCondition(const std::vector<int>& factions, bool any);
        EndCondition(int factionID);

        void Update(Level& level);

        void UpdateLinkage(const idMappingType& id_mapping);

        bool IsMet() const { return state && !disabled; }
        void Set() { state = true; }

        void DBG_GUI();
    private:
        void CheckObjects(Level& level);
        void CheckFactions(Level& level);
    public:
        std::vector<ObjectID> objects;
        std::vector<int> factions;
        bool objects_any = false;
        bool factions_any = false;

        bool state = false;
        bool disabled = false;
    };

    using EndConditions = std::array<EndCondition, 2>;

    //===== LevelInfo =====

    //Container for various level metadata.
    struct LevelInfo {
        int campaignIdx = -1;
        int race;
        bool custom_game = true;
        int preferred_opponents = 1;
        std::vector<glm::ivec2> startingLocations;
        EndConditions end_conditions;
    public:
        void DBG_GUI();
    };

    //===== Savefile =====

    struct Savefile {
        Mapfile map;
        LevelInfo info;
        FactionsFile factions;
        ObjectsFile objects;
    public:
        Savefile() = default;
        Savefile(const std::string& filepath);

        void Save(const std::string& filepath);
    };

    //===== Level =====

    class Level {
    public:
        Level();
        Level(const glm::vec2& mapSize, const TilesetRef& tileset);
        Level(Savefile& savefile);

        void Update();
        void Render();

        bool Save(const std::string& filepath);
        static int Load(const std::string& filepath, Level& out_level);
        void Release();

        glm::ivec2 MapSize() const { return map.Size(); }

        void CustomGame_InitFactions(int playerRace, int opponentCount);
        void CustomGame_InitEndConditions();

        void EndConditionsEnabled(bool enabled);
    private:
        Savefile Export();
        void LevelPtrUpdate();
        void ConditionsUpdate();
    public:
        //dont change the order !!!
        Map map;
        LevelInfo info;
        Factions factions;
        ObjectPool objects;

        bool initialized = false;
        float lastConditionsUpdate = 0.f;
    };

}//namespace eng
