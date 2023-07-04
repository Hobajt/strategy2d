#pragma once

#include "engine/utils/mathdefs.h"

#include "engine/game/object_data.h"

namespace eng {

    class Level;
    class Unit;

    struct Action;
    struct Command;

    //===== Action =====

    typedef int(*ActionHandler)(Unit& source, Action& data, Level& level);

    namespace ActionType { enum { INVALID = -1, IDLE = 0, MOVE, ACTION, ENTER, UNLOAD, COUNT }; }

    //An atomic unit of work, that a Unit object can do within the game.
    struct Action {
        int type = ActionType::INVALID;
        ActionHandler handler = nullptr;

        glm::vec2 target_pos;
        ObjectID target;

        float t = 0.f;
        //maybe add fields to track timing & animation synchronization
    public:
        Action() = default;
        
        static Action Move(const glm::ivec2& target_pos);

        //Return value in general - 0 = ongoing action, 1 = nullptr, other = action specific
        int Update(Unit& source, Level& level);
    };

    //===== Command =====

    typedef void(*CommandHandler)(Unit& source, Command& cmd, Level& level);

    namespace CommandType { enum { IDLE = 0, MOVE, COUNT }; }

    //Describes a more complex work, that Unit objects are tasked with.
    class Command {
        friend void CommandHandler_Move(Unit& src, Command& cmd, Level& level);
    public:
        //Creates idle command
        Command();

        static Command Idle();
        static Command Move(const glm::ivec2& target_pos);

        void Update(Unit& src, Level& level);
    private:
        int type;
        CommandHandler handler;

        Action action;

        glm::ivec2 target_pos;
        ObjectID target_id;
    };

}//namespace eng
