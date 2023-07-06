#pragma once

#include "engine/utils/mathdefs.h"

#include "engine/game/object_data.h"

#include <string>

namespace eng {

    class Level;
    class Unit;

    struct Action;
    struct Command;

    //===== Action =====

    typedef int(*ActionUpdateHandler)(Unit& source, Level& level, Action& action);
    typedef void(*ActionSignalHandler)(Unit& src, Action& action, int signal, int cmdType, int cmdType_prev);

    namespace ActionType { enum { INVALID = -1, IDLE = 0, MOVE, ACTION, ENTER, UNLOAD, COUNT }; }
    namespace ActionSignal { enum { CUSTOM = 0, COMMAND_SWITCH }; }

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
            float t = 0.f;
            int i = 0;
        public:
            void Reset();
        };
    public:
        Logic logic;
        Data data;
    public:
        //Creates idle action.
        Action();

        static Action Idle();
        static Action Move(const glm::ivec2& pos_src, const glm::ivec2& pos_dst);

        //========

        int Update(Unit& source, Level& level);
        void Signal(Unit& source, int signal, int cmdType, int prevCmdType);
    };

    //===== Command =====

    typedef void(*CommandHandler)(Unit& source, Level& level, Command& cmd, Action& action);

    namespace CommandType { enum { IDLE = 0, MOVE, COUNT }; }

    //Describes a more complex work, that Unit objects are tasked with.
    class Command {
        friend void CommandHandler_Move(Unit& source, Level& level, Command& cmd, Action& action);
    public:
        //Creates idle command
        Command();

        static Command Idle();
        static Command Move(const glm::ivec2& target_pos);

        void Update(Unit& src, Level& level);

        int Type() const { return type; }

        //only use for debug msgs
        std::string to_string() const;
    private:
        int type;
        CommandHandler handler;

        glm::ivec2 target_pos;
        ObjectID target_id;
    };



    

}//namespace eng
