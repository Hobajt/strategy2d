#include "engine/game/command.h"

#include "engine/core/input.h"
#include "engine/game/level.h"

namespace eng {

#define ACTION_INPROGRESS               0
#define ACTION_FINISHED_INVALID         1
#define ACTION_FINISHED_SUCCESS         2
#define ACTION_FINISHED_INTERRUPTED     3

    //=============================

    int ActionHandler_Idle(Unit& src, Action& action, Level& level);
    int ActionHandler_Move(Unit& src, Action& action, Level& level);

    void CommandHandler_Idle(Unit& src, Command& cmd, Level& level);
    void CommandHandler_Move(Unit& src, Command& cmd, Level& level);


    //===== Action =====

    Action Action::Move(const glm::ivec2& target_pos) {
        Action action = {};

        action.type = ActionType::MOVE;
        action.handler = ActionHandler_Move;
        action.target_pos = target_pos;

        return action;
    }

    int Action::Update(Unit& src, Level& level) {
        if(handler == nullptr)
            return ACTION_FINISHED_INVALID;
        return handler(src, *this, level);
    }

    //===== Command =====

    Command::Command() : type(CommandType::IDLE), handler(CommandHandler_Idle) {}

    Command Command::Idle() {
        return Command();
    }

    Command Command::Move(const glm::ivec2& target_pos) {
        Command cmd = {};

        cmd.type = CommandType::MOVE;
        cmd.handler = CommandHandler_Move;
        cmd.target_pos = target_pos;

        return cmd;
    }

    void Command::Update(Unit& src, Level& level) {
        ASSERT_MSG(handler != nullptr, "Command::Update - command handler should never be null.");
        handler(src, *this, level);
    }

    //=============================

    void CommandHandler_Idle(Unit& src, Command& cmd, Level& level) {
        //TODO:
    }

    void CommandHandler_Move(Unit& src, Command& cmd, Level& level) {
        int res = cmd.action.Update(src, level);

        if(res != ACTION_INPROGRESS) {
            if(glm::distance(glm::vec2(cmd.target_pos), src.Position()) < 1e-3f) {
                //destination reached, start idle command
                cmd = Command::Idle();
            }
            else {
                //consult navmesh - fetch next target position
                glm::ivec2 target_pos = level.map.UnitRoute_NextPos(src, cmd.target_pos);
                ASSERT_MSG(has_valid_direction(target_pos - glm::ivec2(src.Position())), "Command::Move - target position for next action doesn't respect directions.");
                cmd.action = Action::Move(target_pos);
            }
        }
    }

    //=============================

    int ActionHandler_Idle(Unit& src, Action& action, Level& level) {
        //TODO:
        return ACTION_INPROGRESS;
    }

    int ActionHandler_Move(Unit& src, Action& action, Level& level) {
        Input& input = Input::Get();

        //identify the tile you're standing on & the one you're currently moving onto

        //if motion in progress -> keep going
        //if tile finished & moving to next one:
        //      - validate that it's clear ... if it isn't:
        //          - check if it's a final destination
        //              - it is     -> interrupt action (needs rerouting)
        //              - it isn't  -> wait, until it's clear or marked as final destination

        //also update unit's action & orientation

        return ACTION_INPROGRESS;
    }
    

}//namespace eng
