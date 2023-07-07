#include "engine/game/command.h"

#include "engine/core/input.h"
#include "engine/game/level.h"

namespace eng {

#define ACTION_INPROGRESS               0
#define ACTION_FINISHED_SUCCESS         1
#define ACTION_FINISHED_INTERRUPTED     2

    int DirVectorCoord(int orientation);
    glm::ivec2 DirectionVector(int orientation);
    int VectorOrientation(const glm::ivec2& v);
    

    /* GROUND RULES:
        - command cannot override action when it's returning ACTION_INPROGRESS (use action.Signal() for that)
    
    */

    //=============================

    /* IDLE ACTION DESCRIPTION:
    */

    int  IdleAction_Update(Unit& src, Level& level, Action& action);
    void IdleAction_Signal(Unit& src, Action& action, int signal, int cmdType, int cmdType_prev);

    
    /* MOVE ACTION DESCRIPTION:
        - move action moves the unit tile by tile, ...
        - signaling causes the unit to stop at current location (finishes movement to the center of tile)
    */

    int  MoveAction_Update(Unit& src, Level& level, Action& action);
    void MoveAction_Signal(Unit& src, Action& action, int signal, int cmdType, int cmdType_prev);


    //=============================

    /* IDLE COMMAND DESCRIPTION:
        - 
    */

    void CommandHandler_Idle(Unit& src, Level& level, Command& cmd, Action& action);

    /* MOVE COMMAND DESCRIPTION:
        - 
    */

    void CommandHandler_Move(Unit& src, Level& level, Command& cmd, Action& action);


    //===== Action =====

    Action::Logic::Logic(int type_, ActionUpdateHandler update_, ActionSignalHandler signal_) : type(type_), update(update_), signal(signal_) {}

    void Action::Data::Reset() {
        t = 0.f;
        i = 0;
    }

    Action::Action() : logic(Action::Logic(ActionType::IDLE, IdleAction_Update, IdleAction_Signal)), data(Action::Data{}) {}

    Action Action::Idle() {
        return Action();
    }

    Action Action::Move(const glm::ivec2& pos_src, const glm::ivec2& pos_dst) {
        Action action = {};

        action.logic = Action::Logic(ActionType::MOVE, MoveAction_Update, MoveAction_Signal);

        action.data.Reset();
        action.data.target_pos = pos_dst;
        action.data.move_dir = glm::sign(pos_dst - pos_src);
        action.data.i = VectorOrientation(action.data.move_dir);

        return action;
    }

    int Action::Update(Unit& source, Level& level) {
        ASSERT_MSG(logic.update != nullptr, "Action::Update - handler should never be null.");
        return logic.update(source, level, *this);
    }

    void Action::Signal(Unit& source, int signal, int cmdType, int prevCmdType) {
        ASSERT_MSG(logic.signal != nullptr, "Action::Signal - handler should never be null.");
        logic.signal(source, *this, signal, cmdType, prevCmdType);
    }

    //=======================================

    int IdleAction_Update(Unit& src, Level& level, Action& action) {
        //TODO:

        src.act() = action.logic.type;

        //should probably always return finished, so that other commands can cancel it
        return ACTION_FINISHED_SUCCESS;
    }

    void IdleAction_Signal(Unit& src, Action& action, int signal, int cmdType, int cmdType_prev) {

    }

    int MoveAction_Update(Unit& src, Level& level, Action& action) {
        Input& input = Input::Get();

        //interpolation limits, based on whether the movement goes along the diagonal or not
        constexpr static float lim[2] = { 1.f, 1.41421f };

        //data.i = movement orientation idx, data.t = motion interpolation value between tiles
        int orientation = action.data.i;
        float& t = action.data.t;
        float l = lim[orientation % 2];

        //animation values update
        src.act() = action.logic.type;
        src.ori() = orientation;

        //identify the tile you're standing on & the one you're currently moving onto
        glm::ivec2& pos = src.pos();
        glm::vec2& move_offset = src.m_offset();

        //either action started, or target tile reached (target = next in line, not the final target)
        if(t >= 0.f) {
            if(pos == action.data.target_pos) {
                //unit is at the final target
                t = 0.f;
                move_offset = glm::vec2(0.f);
                return ACTION_FINISHED_SUCCESS;
            }
            else {
                const TileData& td = level.map(pos);
                int navType = src.NavigationType();
                if(!td.Traversable(navType)) {
                    //next tile is untraversable
                    t = 0.f;
                    move_offset = glm::vec2(0.f);

                    //either stop the action or wait, based on whether the tile is expected to be freed soon
                    return td.PermanentlyTaken(navType) ? ACTION_FINISHED_INTERRUPTED : ACTION_INPROGRESS;
                }
                else {
                    //next tile is traversable

                    //map update - claim the next tile as taken by this unit & free the previous one
                    //TODO:

                    //update position & animation offset & keep moving
                    pos += action.data.move_dir;
                    t = -l;
                }
            }
        }

        //motion update tick
        t += input.deltaTime * src.MoveSpeed();
        move_offset = (t/l) * glm::vec2(action.data.move_dir);

        return ACTION_INPROGRESS;
    }

    void MoveAction_Signal(Unit& src, Action& action, int signal, int cmdType, int cmdType_prev) {
        action.data.target_pos = src.Position();
    }

    //=======================================

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
        ASSERT_MSG(handler != nullptr, "Command::Update - handler should never be null.");
        handler(src, level, *this, src.action);
    }

    std::string Command::to_string() const {
        char buf[2048];

        switch(type) {
        case CommandType::IDLE:
            snprintf(buf, sizeof(buf), "Command: Idle");
            break;
        case CommandType::MOVE:
            snprintf(buf, sizeof(buf), "Command: Move to (%d, %d)", target_pos.x, target_pos.y);
            break;
        default:
            snprintf(buf, sizeof(buf), "Command: Unknown type (%d)", type);
            break;
        }

        return std::string(buf);
    }

    //=======================================

    void CommandHandler_Idle(Unit& src, Level& level, Command& cmd, Action& action) {
        //TODO:
        int res = action.Update(src, level);
        if(res != ACTION_INPROGRESS && action.logic.type != ActionType::IDLE) {
            action = Action::Idle();
        }
    }

    void CommandHandler_Move(Unit& src, Level& level, Command& cmd, Action& action) {
        int res = action.Update(src, level);

        if(res != ACTION_INPROGRESS) {
            if(cmd.target_pos == src.Position()) {
                //destination reached, start idle command
                cmd = Command::Idle();
            }
            else {
                //consult navmesh - fetch next target position
                glm::ivec2 target_pos = level.map.Pathfinding_NextPosition(src, cmd.target_pos);
                ASSERT_MSG(has_valid_direction(target_pos - src.Position()), "Command::Move - target position for next action doesn't respect directions.");
                action = Action::Move(src.Position(), target_pos);
            }
        }

        //TODO: add handling for unreachable locations - both in navigation & in command
    }

    //=======================================

    int DirVectorCoord(int ori) {
        ori = ori % 8;
        return 1 - 2*(ori/5) - int(ori % 4 == 0);
    }

    glm::ivec2 DirectionVector(int orientation) {
        return glm::ivec2( DirVectorCoord(orientation), -DirVectorCoord(orientation+6) );
    }

    int VectorOrientation(const glm::ivec2& v) {
        int orientation = int(4.f * (1.f + (std::atan2f(v.y, v.x) * glm::one_over_pi<float>())));
        ASSERT_MSG(orientation >= 1 && orientation <= 8, "VectorOrientation - invalid conversion.");
        orientation = (8-orientation+6) % 8;
        return orientation;
    }

}//namespace eng
