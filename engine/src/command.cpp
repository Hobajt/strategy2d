#include "engine/game/command.h"

#include "engine/core/input.h"
#include "engine/game/level.h"
#include "engine/core/audio.h"

namespace eng {

#define ACTION_INPROGRESS               0
#define ACTION_FINISHED_SUCCESS         1
#define ACTION_FINISHED_INTERRUPTED     2

#define CONSTRUCTION_SPEED 5.f

    /* GROUND RULES:
        - command cannot override action when it's returning ACTION_INPROGRESS (use action.Signal() for that)
    */

    //=============================

    /* IDLE ACTION DESCRIPTION:
        - TODO:
    */

    int  IdleAction_Update(Unit& src, Level& level, Action& action);
    void IdleAction_Signal(Unit& src, Action& action, int signal, int cmdType, int cmdType_prev);

    
    /* MOVE ACTION DESCRIPTION:
        - moves the unit in a tile by tile fashion
            - unit position is tracked in integers (no midway between tiles, unit always occupies a single tile)
            - animation position is handled through move_offset variable
            - when the motion starts:
                - position is updated to the next tile
                - move_offset is set to the offset of previous tile & slowly interpolates to vec2(0,0) - this makes the unit move
                - map tile ownership is updated
            - after every tile, the action checks next tile's availability:
                - if destination is reached -> action finishes succcessfully
                - if next tile is traversable, initiates movement onto the next tile
                - if it's unreachable, either finishes unsuccessfully or waits until the tile is freed
        - signaling causes the unit to stop at current location (finishes movement to the center of tile)
    */

    int  MoveAction_Update(Unit& src, Level& level, Action& action);
    void MoveAction_Signal(Unit& src, Action& action, int signal, int cmdType, int cmdType_prev);

    /* "ACTION" ACTION DESCRIPTION:
        - action for generic activities
            - manages synchronization between unit's animation and the effect of given activity (ie. ranged attack -> spawn arrow at proper time)
            - also manages cooldown after the action has been done
            - supports 3 types of activities:
                - melee attack - applies damage to the target
                - wood harvest - decrements wood resource
                - effect - spawns an utility object with given effect
            - returns:
                - in_progress while in progress or when on cooldown
                - finished once the cooldown wears off
                - for harvest, returns tick_signal when target wood block has exactly 0 health
        - signaling causes the unit to cancel the attack (effect doesn't happen, but animation still finishes)
    */

    int  ActionAction_Update(Unit& src, Level& level, Action& action);
    void ActionAction_Signal(Unit& src, Action& action, int signal, int cmdType, int cmdType_prev);


    //=============================

    /* IDLE COMMAND DESCRIPTION:
        - TODO:
    */

    void CommandHandler_Idle(Unit& src, Level& level, Command& cmd, Action& action);
    
    /* MOVE COMMAND DESCRIPTION:
        - generates move actions that move unit in one direction
        - after each completed move action, it checks:
            - did unit reach it's destination? -> command finished
            - otherwise it checks pathfinding & either starts new move action or terminates bcs the destination is unreachable
    */

    void CommandHandler_Move(Unit& src, Level& level, Command& cmd, Action& action);

    /* ATTACK COMMAND DESCRIPTION:
        - if the target is within range, periodically generates attack actions
        - between the attack actions:
            - checks for target existence
            - validates that the target is still within range
                - if it isn't issues move actions in order to move to valid range
    */ 

    void CommandHandler_Attack(Unit& src, Level& level, Command& cmd, Action& action);

    //=============================

    void BuildingAction_Idle(Building& src, Level& level, BuildingAction& action);
    void BuildingAction_Attack(Building& src, Level& level, BuildingAction& action);
    void BuildingAction_TrainOrResearch(Building& src, Level& level, BuildingAction& action);
    void BuildingAction_ConstructOrUpgrade(Building& src, Level& level, BuildingAction& action);


    //===== Action =====

    Action::Logic::Logic(int type_, ActionUpdateHandler update_, ActionSignalHandler signal_) : type(type_), update(update_), signal(signal_) {}

    void Action::Data::Reset() {
        i = j = k = 0;
        b = c = false;
        t = 0.f;
    }

    void Action::Data::DBG_Print() {
        ENG_LOG_TRACE("i={}, j={}, k={}, b={}, c={}, t={}", i, j, k, b, c, t);
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

    Action Action::Attack(const ObjectID& target_id, const glm::ivec2& target_pos, const glm::ivec2& target_dir, bool is_ranged) {
        Action action = {};

        action.logic = Action::Logic(ActionType::ACTION, ActionAction_Update, ActionAction_Signal);
        action.data.Reset();
        action.data.target = target_id;
        action.data.target_pos = target_pos;

        action.data.i = VectorOrientation(glm::vec2(target_dir) / glm::length(glm::vec2(target_dir)));  //animation orientation
        action.data.j = is_ranged ? ActionPayloadType::RANGED_ATTACK : ActionPayloadType::MELEE_ATTACK; //payload type identifier

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
                glm::ivec2 next_pos = pos + action.data.move_dir;
                const TileData& td = level.map(next_pos);
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
                    level.map.MoveUnit(navType, pos, next_pos, next_pos == action.data.target_pos);

                    //update position & animation offset & keep moving
                    pos = next_pos;
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

    int ActionAction_Update(Unit& src, Level& level, Action& action) {
        Input& input = Input::Get();
        int orientation = action.data.i;
        int payload_id = action.data.j;
        bool& delivered = action.data.b;
        bool& anim_ended = action.data.c;
        float& action_stop_time = action.data.t;
        //===========

        // action.data.DBG_Print();

        //payload delivery check
        if(src.AnimKeyframeSignal() && !delivered) {
            delivered = true;
            // ENG_LOG_INFO("ATTACK ACTION PAYLOAD");

            switch(payload_id) {
                case ActionPayloadType::HARVEST:        //harvest action payload
                    //TODO:
                    //decrement value in the map data, send out signal when
                    //harvest could return from here (wood tick - when the tree is felled)
                    ENG_LOG_INFO("ACTION PAYLOAD - HARVEST");
                    break;
                case ActionPayloadType::MELEE_ATTACK:   //melee attack action payload
                {
                    ENG_LOG_INFO("ACTION PAYLOAD - MELEE ATTACK");
                    
                    bool attack_happened = ApplyDamage(level, src, action.data.target, action.data.target_pos);

                    //play units attack sound if the attack happened
                    if(attack_happened && src.UData()->AttackSound().valid) {
                        Audio::Play(src.UData()->AttackSound().Random(), src.Position());
                    }
                }
                    break;
                default:                                //effect action payload (includes ranged attacks)
                {
                    ENG_LOG_INFO("ACTION PAYLOAD - EFFECT/PROJECTILE");
                    //use payload_id to get the prefab from Unit's data
                    UtilityObjectDataRef obj = std::dynamic_pointer_cast<UtilityObjectData>(src.UData()->refs[payload_id]);
                    if(obj != nullptr) {
                        //spawn an utility object (projectile/spell effect/buff)
                        level.objects.EmplaceUtilityObj(level, obj, action.data.target_pos, action.data.target, src);

                        if(src.UData()->AttackSound().valid)
                            Audio::Play(src.UData()->AttackSound().Random(), src.Position());
                    }
                    else {
                        ENG_LOG_ERROR("ActionAction - action payload not delivered - invalid object.");
                    }
                }
                    break;
            }
        }

        //logic for transition from the action to idling (action cooldown)
        if(!anim_ended && src.AnimationFinished()) {
            anim_ended = true;
            action_stop_time = (float)input.CurrentTime() + src.Cooldown();
        }

        //animation values update
        src.act() = anim_ended ? ActionType::IDLE : action.logic.type;
        src.ori() = orientation;

        //terminate action only once the cooldown is over
        return (anim_ended && action_stop_time < (float)input.CurrentTime()) ? ACTION_FINISHED_SUCCESS : ACTION_INPROGRESS;
    }

    void ActionAction_Signal(Unit& src, Action& action, int signal, int cmdType, int cmdType_prev) {
        //mark payload as already delivered - action won't happen but the animation continues
        action.data.b = true;
        //TODO: could also start counting cooldown from now (or make cooldown shorter to switch action faster)
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

    Command Command::Attack(const ObjectID& target_id, const glm::ivec2& target_pos) {
        Command cmd = {};

        cmd.type = CommandType::ATTACK;
        cmd.handler = CommandHandler_Attack;
        cmd.target_id = target_id;
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
        case CommandType::ATTACK:
            snprintf(buf, sizeof(buf), "Command: Attack %s", target_id.to_string().c_str());
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
                if(target_pos == src.Position()) {
                    //target destination unreachable
                    cmd = Command::Idle();
                }
                else {
                    //initiate new move action
                    ASSERT_MSG(has_valid_direction(target_pos - src.Position()), "Command::Move - target position for next action doesn't respect directions.");
                    action = Action::Move(src.Position(), target_pos);
                }
            }
        }
    }

    void CommandHandler_Attack(Unit& src, Level& level, Command& cmd, Action& action) {
        int res = action.Update(src, level);

        if(res != ACTION_INPROGRESS) {

            //target existence check & value retrieval
            glm::vec2 pos_min, pos_max;
            if(cmd.target_id.type != ObjectType::MAP_OBJECT) {
                //target is regular object

                FactionObject* target;
                if(!level.objects.GetObject(cmd.target_id, target)) {
                    //existence check failed - target no longer exists
                    cmd = Command::Idle();
                    return;
                }

                pos_min = target->MinPos();
                pos_max = target->MaxPos();
            }
            else {
                //target is an attackable map object (wall)

                const TileData& tile = level.map(cmd.target_pos);
                if(!IsWallTile(tile.tileType)) {
                    //existence check failed - target no longer exists
                    cmd = Command::Idle();
                    return;
                }

                pos_min = pos_max = glm::vec2(cmd.target_pos);
            }

            //range check & action issuing
            if(!src.RangeCheck(pos_min, pos_max)) {
                //range check failed -> lookup new possible location to attack from & start moving there
                glm::ivec2 move_pos = level.map.Pathfinding_NextPosition_Range(src, pos_min, pos_max);
                if(move_pos == src.Position()) {
                    //no reachable destination found
                    cmd = Command::Idle();
                }
                else {
                    //initiate new move action
                    ASSERT_MSG(has_valid_direction(move_pos - src.Position()), "Command::Move - target position for next action doesn't respect directions.");
                    action = Action::Move(src.Position(), move_pos);
                }
                return;
            }
            else {
                //within range -> iniate new attack action
                action = Action::Attack(cmd.target_id, glm::ivec2(pos_min), glm::ivec2(pos_min) - src.Position(), src.AttackRange() > 1);
            }
        }
    }

    //=======================================

    //===== BuildingAction =====

    BuildingAction::Logic::Logic(int type_, BuildingActionUpdateHandler handler_) : type(type_), update(handler_) {}

    BuildingAction::BuildingAction() : logic(BuildingAction::Logic(BuildingActionType::CONSTRUCTION_OR_UPGRADE, BuildingAction_ConstructOrUpgrade)), data(BuildingAction::Data{}) {
        data.t1 = 0.f;          //initial "progress" value
        data.flag = false;      //marks the action as construction
    }

    BuildingAction BuildingAction::Construction() {
        return BuildingAction();
    }

    BuildingAction BuildingAction::Idle(bool canAttack) {
        return canAttack ? BuildingAction::Idle() : BuildingAction::IdleAttack();
    }

    BuildingAction BuildingAction::Idle() {
        BuildingAction act = {};
        act.logic = BuildingAction::Logic(BuildingActionType::IDLE, BuildingAction_Idle);
        act.data = {};
        return act;
    }

    BuildingAction BuildingAction::IdleAttack() {
        BuildingAction act = {};
        act.logic = BuildingAction::Logic(BuildingActionType::IDLE_ATTACK, BuildingAction_Attack);
        act.data = {};
        return act;
    }
    
    void BuildingAction::Update(Building& source, Level& level) {
        ASSERT_MSG(logic.update != nullptr, "BuildingAction::Update - handler should never be null.");
        logic.update(source, level, *this);
    }

    std::string BuildingAction::to_string() const {
        char buf[2048];

        switch(logic.type) {
        case BuildingActionType::IDLE:
        case BuildingActionType::IDLE_ATTACK:
            snprintf(buf, sizeof(buf), "Action: Idle");
            break;
        case BuildingActionType::CONSTRUCTION_OR_UPGRADE:
            snprintf(buf, sizeof(buf), "Action: %s - %.0f/%.0f (%d%%)", data.flag ? "Upgrade" : "Construction", data.t1, data.t3, int((data.t1/data.t3)*100.f));
            break;
        default:
            snprintf(buf, sizeof(buf), "Action: Unknown type (%d)", logic.type);
            break;
        }

        return std::string(buf);
    }

    //=======================================

    void BuildingAction_Idle(Building& src, Level& level, BuildingAction& action) {
        //swap to attack idle if the building can attack
        if(src.CanAttack()) {
            action = BuildingAction::IdleAttack();
        }

        src.act() = BuildingAnimationType::IDLE;
    }

    void BuildingAction_Attack(Building& src, Level& level, BuildingAction& action) {
        if(!src.CanAttack()) {
            action = BuildingAction::Idle();
        }

        src.act() = BuildingAnimationType::IDLE;
        //TODO:
    }

    void BuildingAction_TrainOrResearch(Building& src, Level& level, BuildingAction& action) {
        //TODO:
    }

    void BuildingAction_ConstructOrUpgrade(Building& src, Level& level, BuildingAction& action) {
        Input& input = Input::Get();
        float& progress = action.data.t1;
        float& prev_health = action.data.t2;
        int& next_tick = action.data.i;
        bool is_construction = !action.data.flag;
        int target = is_construction ? src.BuildTime() : src.UpgradeTime();
        action.data.t3 = target;        //displayed in debug messages
        //=====

        //compute the uptick for this frame
        float uptick = input.deltaTime * CONSTRUCTION_SPEED;

        //increment construction tracking as well as building health
        progress += uptick;

        if(is_construction) {
            //health uptick logic
            if(next_tick < progress) {
                float health_now = ((src.MaxHealth() - src.StartingHealth()) * progress) / target;
                src.AddHealth(int(health_now) - int(prev_health));
                prev_health = health_now;
                next_tick = int(progress)+1;        //increment every second
            }

            //compute animation based on construction percentual progress
            int state = std::min(int((progress / target) * 4.f), 2);
            src.act() = BuildingAnimationType::BUILD1 + state;
        }
        else {
            src.act() = BuildingAnimationType::UPGRADE;
        }

        //construction finished condition
        if(progress >= target) {
            action = BuildingAction::Idle(src.CanAttack());

            if(!is_construction) {
                //TODO: modify building object by changing the data pointers (update animator too)
            }
        }
    }

}//namespace eng
