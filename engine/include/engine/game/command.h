#pragma once

#include "engine/utils/mathdefs.h"
#include "engine/game/object_data.h"

namespace eng {

    //===== Action =====

    class Unit;
    struct Action;
    typedef int(*ActionHandler)(Unit& source, Action& data);

    namespace ActionType { enum { IDLE = 0, MOVE, ACTION, ENTER, UNLOAD, COUNT }; }

    //An atomic unit of work, that a Unit object can do within the game.
    struct Action {
        glm::vec2 target_pos;
        ObjectID target;

        int type;
        ActionHandler handler;

        //maybe add fields to track timing & animation synchronization
    };

    //===== Command =====

    typedef void(*CommandHandler)();

    namespace CommandType { enum {  }; }

    //Describes a more complex work, that Unit objects are tasked with.
    class Command {
    public:
        void Update();
    private:
        Action action;
        CommandHandler handler;
    };

}//namespace eng
