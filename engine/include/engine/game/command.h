#pragma once

#include "engine/utils/mathdefs.h"
#include "engine/game/object_data.h"

#include <string>

namespace eng {

    class Level;
    class Unit;
    class Building;

    struct Action;
    struct Command;
    struct BuildingAction;

    //===== Action =====

    typedef int(*ActionUpdateHandler)(Unit& source, Level& level, Action& action);
    typedef void(*ActionSignalHandler)(Unit& src, Action& action, int signal, int cmdType, int cmdType_prev);

    //when changing values, also update ResolveUnitAnimationID() (local fn in object_data.cpp)
    namespace ActionType { enum { INVALID = -1, IDLE = 0, MOVE, ACTION, ENTER, UNLOAD, COUNT }; }
    namespace UnitAnimationType { enum { IDLE, MOVE, ACTION, DEATH, IDLE2, MOVE2, IDLE3, MOVE3 }; }

    namespace ActionSignal { enum { CUSTOM = 0, COMMAND_SWITCH, INTERRUPT }; }

    namespace ActionPayloadType { enum { HARVEST = -2, MELEE_ATTACK = -1, RANGED_ATTACK = 0, REPAIR, OTHER = 1 }; }

    namespace WorkerCarryState { enum { NONE = 0, GOLD = 1, WOOD = 2, OIL = 4 }; }

    //An atomic unit of work, that a Unit object can do within the game.
    struct Action {
    public:
        struct Logic {
            int type = ActionType::INVALID;
            ActionUpdateHandler update = nullptr;
            ActionSignalHandler signal = nullptr;
        public:
            Logic() = default;
            Logic(int type, ActionUpdateHandler update, ActionSignalHandler signal);
        };
        struct Data {
            glm::ivec2 target_pos;
            glm::ivec2 move_dir;
            ObjectID target;

            //maybe add fields to track timing & animation synchronization

            //helper state variables for action implementation
            int i = 0;
            int j = 0;
            int k = 0;
            bool b = false;
            bool c = false;
            float t = 0.f;
            int count = 0;
        public:
            void Reset();
            void DBG_Print();
        };
        struct Entry {
            Data data;
            int type;
        };
    public:
        Logic logic;
        Data data;
    public:
        //Creates idle action.
        Action();
        Action(const Action::Entry& entry);

        static Action Idle();
        static Action Move(const glm::ivec2& pos_src, const glm::ivec2& pos_dst, int is_transport = 0);
        static Action Attack(const ObjectID& target_id, const glm::ivec2& target_pos, const glm::ivec2& target_dir, bool is_ranged, bool add_tile_offset);
        static Action Cast(int spellID, const ObjectID& target_id, const glm::ivec2& target_pos, const glm::ivec2& target_dir, bool add_tile_offset);
        static Action Harvest(const glm::ivec2& target_pos, const glm::ivec2& target_dir);
        static Action Repair(const ObjectID& target_id, const glm::ivec2& target_dir);

        //========

        int Update(Unit& source, Level& level);
        void Signal(Unit& source, int signal, int cmdType, int prevCmdType);

        Action::Entry Export() const;
    };

    //===== Command =====

    typedef void(*CommandHandler)(Unit& source, Level& level, Command& cmd, Action& action);

    namespace CommandType { enum { IDLE = 0, MOVE, ATTACK, CAST, STAND_GROUND, PATROL, HARVEST_WOOD, GATHER_RESOURCES, RETURN_GOODS, BUILD, REPAIR, ENTER_TRANSPORT, TRANSPORT_UNLOAD, COUNT }; }

    //Describes a more complex work, that Unit objects are tasked with.
    class Command {
        friend void CommandHandler_Idle(Unit& source, Level& level, Command& cmd, Action& action);
        friend void CommandHandler_StandGround(Unit& source, Level& level, Command& cmd, Action& action);
        friend void CommandHandler_Move(Unit& source, Level& level, Command& cmd, Action& action);
        friend void CommandHandler_Attack(Unit& source, Level& level, Command& cmd, Action& action);
        friend void CommandHandler_Cast(Unit& source, Level& level, Command& cmd, Action& action);
        friend void CommandHandler_Patrol(Unit& source, Level& level, Command& cmd, Action& action);
        friend void CommandHandler_Harvest(Unit& source, Level& level, Command& cmd, Action& action);
        friend void CommandHandler_Gather(Unit& source, Level& level, Command& cmd, Action& action);
        friend void CommandHandler_ReturnGoods(Unit& source, Level& level, Command& cmd, Action& action);
        friend void CommandHandler_Build(Unit& source, Level& level, Command& cmd, Action& action);
        friend void CommandHandler_Repair(Unit& source, Level& level, Command& cmd, Action& action);
        friend void CommandHandler_EnterTransport(Unit& source, Level& level, Command& cmd, Action& action);
        friend void CommandHandler_TransportUnload(Unit& source, Level& level, Command& cmd, Action& action);
    public:
        struct Entry {
            int type;
            glm::ivec2 target_pos;
            ObjectID target_id;
            int flag;
            glm::ivec2 v2;
        };
    public:
        //Creates idle command
        Command();
        Command(const Command::Entry& entry);

        //Switching means auto-transitions from Idle/StandGround to Attack command.
        static bool SwitchingEnabled();
        static void EnableSwitching(bool state);

        static Command Idle();
        static Command Move(const glm::ivec2& target_pos);
        static Command Attack(const ObjectID& target_id, const glm::ivec2& target_pos);
        static Command AttackGround(const glm::ivec2& target_pos);
        static Command StandGround();
        static Command Patrol(const glm::ivec2& target_pos);
        static Command Cast(int payload_id, const ObjectID& target_id, const glm::ivec2& target_pos);

        static Command EnterTransport(const ObjectID& target_id);
        static Command TransportUnload();
        static Command TransportUnload(const ObjectID& target_id);

        //worker-only commands
        static Command Harvest(const glm::ivec2& target_pos);
        static Command Gather(const ObjectID& target_id);
        static Command ReturnGoods();
        static Command ReturnGoods(const glm::ivec2& prev_target);
        static Command Build(int buildingID, const glm::ivec2& target_pos);
        static Command Repair(const ObjectID& target_id);

        void Update(Unit& src, Level& level);

        Command::Entry Export() const;

        int Type() const { return type; }
        glm::ivec2 TargetPos() const { return target_pos; }
        int Flag() const { return flag; }
        ObjectID TargetID() const { return target_id; }

        void UpdateTargetID(const ObjectID& new_id) { target_id = new_id; }

        void RandomizeIdleRotation();

        //only use for debug msgs
        std::string to_string() const;
    private:
        int type;
        CommandHandler handler;

        glm::ivec2 target_pos;
        ObjectID target_id;
        int flag;

        glm::ivec2 v2;
        float t;
    };

    //===== BuildingAction =====

    typedef void(*BuildingActionUpdateHandler)(Building& source, Level& level, BuildingAction& action);

    namespace BuildingActionType { enum { INVALID = -1, IDLE = 0, TRAIN_OR_RESEARCH, CONSTRUCTION_OR_UPGRADE, IDLE_ATTACK, COUNT }; }

    //when changing values, also update ResolveBuildingAnimationID() (local fn in object_data.cpp)
    namespace BuildingAnimationType {
        enum { IDLE, IDLE2, BUILD1, BUILD2, BUILD3, UPGRADE, COUNT };
        enum { FIRE_SMALL = 101, FIRE_BIG = 102 };
    }

    struct BuildingAction {
        struct Logic {
            int type = ActionType::INVALID;
            BuildingActionUpdateHandler update = nullptr;
        public:
            Logic() = default;
            Logic(int type, BuildingActionUpdateHandler update);
        };
        struct Data {
            float t1 = 0.f;
            float t2 = 0.f;
            float t3 = 0.f;
            int i = 0;
            bool flag = false;
            ObjectID target_id = ObjectID();
        };
        struct Entry {
            int type;
            Data data;
        };
    public:
        Logic logic;
        Data data;
    public:
        //Construction action
        BuildingAction();
        BuildingAction(const BuildingAction::Entry& entry);

        static BuildingAction Idle(bool canAttack);
        static BuildingAction Idle();
        static BuildingAction IdleAttack();
        static BuildingAction Construction();

        static BuildingAction TrainOrResearch(bool training, int payload_id, float required_time);
        static BuildingAction Upgrade(int payload_id);

        void Update(Building& src, Level& level);

        BuildingAction::Entry Export() const;

        bool IsConstructionAction() const;
        bool CustomConstructionViz() const;

        bool UseConstructionSize() const { return IsConstructionAction() && !CustomConstructionViz(); }

        std::string to_string() const;
    };

}//namespace eng
