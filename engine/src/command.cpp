#include "engine/game/command.h"

#include "engine/core/input.h"
#include "engine/game/level.h"
#include "engine/core/audio.h"
#include "engine/game/config.h"

#include "engine/game/player_controller.h"
#include "engine/game/resources.h"

namespace eng {

#define ACTION_INPROGRESS               0
#define ACTION_FINISHED_SUCCESS         1
#define ACTION_FINISHED_INTERRUPTED     2
#define ACTION_FINISHED_FAILURE         4

#define CONSTRUCTION_SPEED 15.f
#define HARVEST_WOOD_SCAN_RADIUS 6

#define BUILDING_ATTACK_SCAN_INTERVAL 5
#define BUILDING_ATTACK_SPEED 5.f

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

    void CommandHandler_Cast(Unit& src, Level& level, Command& cmd, Action& action);

    /* HARVEST COMMAND DESCRIPTION:
        - auto-cancels for non worker units
        - automatically transitions to ReturnGoods command when the worker is already carrying
        - checks the selected tile for trees, looks up new nearby location if there are none
        - then checks the distance, possibly moves towards the tile & cycles Harvest actions on the tile
    */

    void CommandHandler_Harvest(Unit& src, Level& level, Command& cmd, Action& action);

    /* GATHER COMMAND DESCRIPTION:
        - auto-cancels for non worker units
        - automatically transitions to ReturnGoods command when the worker is already carrying
        - validates that the resource building still exists, then controls movement towards the resource
        - once standing next to it, requests entrance to the resource
    */

    void CommandHandler_Gather(Unit& source, Level& level, Command& cmd, Action& action);

    /* RETURN GOODS DESCRIPTION:
        - auto-cancels for non worker units & when the unit isn't carrying a resource
        - looks up nearest dropoff point for the resource that is being carried (pathfinding)
            - repeats this lookup whenever the old one is found invalid
        - moves towards the building
        - once the worker is standing next to the building, requests entry on the EntranceController and stops the command
            - cycling back to previous gather/harvest command is handled by EntranceController (when the worker leaves the building)
            - previous command info is passed through target_pos variable
    */

    void CommandHandler_ReturnGoods(Unit& src, Level& level, Command& cmd, Action& action);

    /* BUILD COMMAND DESCRIPTION:
        - 
    */

    void CommandHandler_Build(Unit& src, Level& level, Command& cmd, Action& action);

    void CommandHandler_Repair(Unit& src, Level& level, Command& cmd, Action& action);

    void CommandHandler_EnterTransport(Unit& source, Level& level, Command& cmd, Action& action);

    void CommandHandler_TransportUnload(Unit& source, Level& level, Command& cmd, Action& action);

    void CommandHandler_Patrol(Unit& src, Level& level, Command& cmd, Action& action);
    void CommandHandler_StandGround(Unit& src, Level& level, Command& cmd, Action& action);

    //=============================

    void BuildingAction_Idle(Building& src, Level& level, BuildingAction& action);
    void BuildingAction_Attack(Building& src, Level& level, BuildingAction& action);
    void BuildingAction_TrainOrResearch(Building& src, Level& level, BuildingAction& action);
    void BuildingAction_ConstructOrUpgrade(Building& src, Level& level, BuildingAction& action);


    //===== Action =====

    Action::Logic::Logic(int type_, ActionUpdateHandler update_, ActionSignalHandler signal_) : type(type_), update(update_), signal(signal_) {}

    void Action::Data::Reset() {
        count = i = j = k = 0;
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

    Action Action::Move(const glm::ivec2& pos_src, const glm::ivec2& pos_dst, int is_transport) {
        Action action = {};

        action.logic = Action::Logic(ActionType::MOVE, MoveAction_Update, MoveAction_Signal);

        action.data.Reset();
        action.data.target_pos = pos_dst;
        action.data.move_dir = glm::sign(pos_dst - pos_src);
        action.data.i = VectorOrientation(action.data.move_dir);
        action.data.j = is_transport;

        return action;
    }

    Action Action::Attack(const ObjectID& target_id, const glm::ivec2& target_pos, const glm::ivec2& target_dir, bool is_ranged, bool add_tile_offset) {
        Action action = {};

        action.logic = Action::Logic(ActionType::ACTION, ActionAction_Update, ActionAction_Signal);
        action.data.Reset();
        action.data.target = target_id;
        action.data.target_pos = target_pos;

        action.data.i = VectorOrientation(glm::vec2(target_dir) / glm::length(glm::vec2(target_dir)));  //animation orientation
        action.data.j = is_ranged ? ActionPayloadType::RANGED_ATTACK : ActionPayloadType::MELEE_ATTACK; //payload type identifier
        action.data.k = int(add_tile_offset);

        return action;
    }

    Action Action::Cast(int spellID, const ObjectID& target_id, const glm::ivec2& target_pos, const glm::ivec2& target_dir, bool add_tile_offset) {
        Action action = {};

        action.logic = Action::Logic(ActionType::ACTION, ActionAction_Update, ActionAction_Signal);
        action.data.Reset();
        action.data.target = target_id;
        action.data.target_pos = target_pos;

        action.data.i = VectorOrientation(glm::vec2(target_dir) / glm::length(glm::vec2(target_dir)));  //animation orientation
        action.data.j = spellID + 2;        //adding 2, because 0,1 mean ranged attack and repair in the Action::Action handler
        action.data.k = int(add_tile_offset);

        return action;
    }

    Action Action::Harvest(const glm::ivec2& target_pos, const glm::ivec2& target_dir) {
        Action action = {};

        action.logic = Action::Logic(ActionType::ACTION, ActionAction_Update, ActionAction_Signal);
        action.data.Reset();
        action.data.target_pos = target_pos;

        action.data.i = VectorOrientation(glm::vec2(target_dir) / glm::length(glm::vec2(target_dir)));  //animation orientation
        action.data.j = ActionPayloadType::HARVEST;

        return action;
    }

    Action Action::Repair(const ObjectID& target_id, const glm::ivec2& target_dir) {
        Action action = {};

        action.logic = Action::Logic(ActionType::ACTION, ActionAction_Update, ActionAction_Signal);
        action.data.Reset();
        action.data.target = target_id;

        action.data.i = VectorOrientation(glm::vec2(target_dir) / glm::length(glm::vec2(target_dir)));  //animation orientation
        action.data.j = ActionPayloadType::REPAIR;

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
        int& counter = action.data.count;
        float l = lim[orientation % 2];
        bool is_transport = bool(action.data.j);

        //animation values update
        src.act() = action.logic.type;
        src.ori() = orientation;

        //identify the tile you're standing on & the one you're currently moving onto
        glm::ivec2& pos = src.pos();
        glm::vec2& move_offset = src.m_offset();

        //either action started, or target tile reached (target = next in line, not the final target)
        if(t >= 0.f) {
            counter++;
            if(pos == action.data.target_pos) {
                //unit is at the final target
                t = 0.f;
                move_offset = glm::vec2(0.f);
                return ACTION_FINISHED_SUCCESS;
            }
            else {
                int navType = src.NavigationType();
                int step = 1 + int(navType != NavigationBit::GROUND);
                glm::ivec2 next_pos = pos + action.data.move_dir * step;
                const TileData& td = level.map(next_pos);

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
                    level.map.MoveUnit(navType, pos, next_pos, next_pos == action.data.target_pos, src.VisionRange());

                    //docking sound for transport ships
                    if(is_transport && (level.map(next_pos).IsCoastTile() ^ level.map(pos).IsCoastTile())) {
                        Audio::Play(SoundEffect::Dock().Random(), next_pos);
                    }

                    //update position & animation offset & keep moving
                    pos = next_pos;
                    t = -l * step;
                }
            }
        }

        //motion update tick
        t += input.deltaTime * src.MoveSpeed_Real();
        move_offset = (t/l) * glm::vec2(action.data.move_dir);

        return ACTION_INPROGRESS;
    }

    void MoveAction_Signal(Unit& src, Action& action, int signal, int cmdType, int cmdType_prev) {
        action.data.target_pos = src.Position();
    }

    int ActionAction_Update(Unit& src, Level& level, Action& action) {
        Input& input = Input::Get();
        int& orientation = action.data.i;
        int payload_id = action.data.j;
        bool& delivered = action.data.b;
        bool& anim_ended = action.data.c;
        float& action_stop_time = action.data.t;
        //===========

        int action_result = ACTION_FINISHED_SUCCESS;

        // action.data.DBG_Print();

        //forcefully switch action to update actionID in the animator (to use the right action's values to compute keyframe)
        if(!anim_ended) src.SwitchAnimatorAction(action.logic.type);

        //payload delivery check
        if(src.AnimKeyframeSignal() && !delivered) {
            // src.Anim_DBG_Print();
            // src.Anim_DBG_Print(action.logic.type);
            delivered = true;
            // ENG_LOG_INFO("ATTACK ACTION PAYLOAD");

            //break invisibility
            src.SetInvisible(false);

            if(src.RotateWhenAttacking())
                orientation = (orientation + 2) % 8;

            switch(payload_id) {
                case ActionPayloadType::HARVEST:        //harvest action payload
                {
                    //harvest could return from here (wood tick - when the tree is felled)
                    // ENG_LOG_INFO("ACTION PAYLOAD - HARVEST");
                    Audio::Play(SoundEffect::Wood().Random(), src.Position());
                    if(level.map.HarvestTile(action.data.target_pos)) {
                        src.ChangeCarryStatus(WorkerCarryState::WOOD);
                    }
                }
                    break;
                case ActionPayloadType::MELEE_ATTACK:   //melee attack action payload
                {
                    // ENG_LOG_INFO("ACTION PAYLOAD - MELEE ATTACK");
                    
                    bool attack_happened = ApplyDamage(level, src, action.data.target, action.data.target_pos);

                    //play units attack sound if the attack happened
                    if(attack_happened && src.Sound_Attack().valid) {
                        Audio::Play(src.Sound_Attack().Random(), src.Position());
                    }
                }
                    break;
                case ActionPayloadType::REPAIR:
                {
                    Building* target = nullptr;
                    if(level.objects.GetBuilding(action.data.target, target)) {
                        target->AddHealth(4.f);
                    }
                    break;
                }
                default:                                //effect action payload (includes ranged attacks)
                {
                    ENG_LOG_INFO("ACTION PAYLOAD - EFFECT/PROJECTILE");
                    //use payload_id to get the prefab from Unit's data

                    bool oom = false;
                    int spellID = -1;
                    UtilityObjectDataRef obj = nullptr;
                    if(payload_id == ActionPayloadType::RANGED_ATTACK) {
                        obj = src.FetchProjectileRef();
                    }
                    else {
                        spellID = payload_id-2;
                        int price = SpellID::Price(spellID);
                        oom = price > src.Mana();
                        if(!oom) {
                            obj = Resources::LoadSpell(spellID);

                            //subtract spell resources (heal has special handling and subtracts from the spell handler instead)
                            if(spellID != SpellID::HEAL)
                                src.DecreaseMana(price);
                        }
                    }
                    
                    if(obj != nullptr) {
                        //spawn an utility object (projectile/spell effect/buff)
                        glm::vec2 target_pos = glm::vec2(action.data.target_pos) + (action.data.k * 0.5f);
                        level.objects.EmplaceUtilityObj(level, obj, target_pos, action.data.target, src, spellID != SpellID::HEAL);
                    }
                    else {
                        if(oom)
                            ENG_LOG_TRACE("ActionAction - action payload not delivered - out of mana (spellID={}).", spellID);
                        else
                            ENG_LOG_ERROR("ActionAction - action payload not delivered - invalid object.");
                    }
                }
                    break;
            }
        }

        //logic for transition from the action to idling (action cooldown)
        if(!anim_ended && src.AnimationFinished()) {
            anim_ended = true;
            //cooldown influenced by haste/slow buff
            action_stop_time = input.GameTimeDelay((2.f - src.SpeedBuffValue()) * src.Cooldown());
        }

        //animation values update
        src.act() = anim_ended ? ActionType::IDLE : action.logic.type;
        src.ori() = orientation;

        //terminate action only once the cooldown is over
        return (anim_ended && action_stop_time < (float)input.CurrentTime()) ? action_result : ACTION_INPROGRESS;
    }

    void ActionAction_Signal(Unit& src, Action& action, int signal, int cmdType, int cmdType_prev) {
        //mark payload as already delivered - action won't happen but the animation continues
        action.data.b = true;
        //TODO: could also start counting cooldown from now (or make cooldown shorter to switch action faster)
    }

    //=======================================

    //===== Command =====

    Command::Command() : type(CommandType::IDLE), handler(CommandHandler_Idle), flag(0) {}

    Command Command::Idle() {
        return Command();
    }

    Command Command::Move(const glm::ivec2& target_pos) {
        Command cmd = {};

        cmd.type = CommandType::MOVE;
        cmd.handler = CommandHandler_Move;
        cmd.target_pos = target_pos;
        cmd.flag = 1;       //marks first command update

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

    Command Command::AttackGround(const glm::ivec2& target_pos) {
        Command cmd = {};

        cmd.type = CommandType::ATTACK;
        cmd.handler = CommandHandler_Attack;
        cmd.target_pos = target_pos;
        cmd.target_id = ObjectID(ObjectType::MAP_OBJECT, 0, 0);
        cmd.flag = 1;       //marks as attack ground command

        return cmd;
    }

    Command Command::StandGround() {
        Command cmd = {};

        cmd.type = CommandType::STAND_GROUND;
        cmd.handler = CommandHandler_StandGround;

        return cmd;
    }

    Command Command::Patrol(const glm::ivec2& target_pos) {
        Command cmd = {};

        cmd.type = CommandType::PATROL;
        cmd.handler = CommandHandler_Patrol;
        cmd.target_pos = target_pos;
        cmd.v2 = glm::ivec2(-1);

        return cmd;
    }

    Command Command::Cast(int payload_id, const ObjectID& target_id, const glm::ivec2& target_pos) {
        Command cmd = {};

        cmd.type = CommandType::CAST;
        cmd.handler = CommandHandler_Cast;
        cmd.target_id = target_id;
        cmd.target_pos = target_pos;
        cmd.flag = payload_id;

        return cmd;
    }

    Command Command::EnterTransport(const ObjectID& target_id) {
        Command cmd = {};

        cmd.type = CommandType::ENTER_TRANSPORT;
        cmd.handler = CommandHandler_EnterTransport;
        cmd.target_id = target_id;
        cmd.target_pos = glm::ivec2(-1);
        cmd.flag = 1;

        return cmd;
    }

    Command Command::TransportUnload() {
        Command cmd = {};

        cmd.type = CommandType::TRANSPORT_UNLOAD;
        cmd.handler = CommandHandler_TransportUnload;
        cmd.target_id = ObjectID();
        cmd.flag = 0;       //distinguishes between targeted & untargeted unload

        return cmd;
    }

    Command Command::TransportUnload(const ObjectID& target_id) {
        Command cmd = {};

        cmd.type = CommandType::TRANSPORT_UNLOAD;
        cmd.handler = CommandHandler_TransportUnload;
        cmd.target_id = target_id;
        cmd.flag = 1;       //distinguishes between targeted & untargeted unload

        return cmd;
    }

    Command Command::Harvest(const glm::ivec2& target_pos) {
        Command cmd = {};

        cmd.type = CommandType::HARVEST_WOOD;
        cmd.handler = CommandHandler_Harvest;
        cmd.target_id = ObjectID();
        cmd.target_pos = target_pos;

        return cmd;
    }

    Command Command::Gather(const ObjectID& target_id) {
        Command cmd = {};

        cmd.type = CommandType::GATHER_RESOURCES;
        cmd.handler = CommandHandler_Gather;
        cmd.target_id = target_id;

        return cmd;
    }

    Command Command::ReturnGoods() {
        return ReturnGoods(glm::ivec2(-1));
    }

    Command Command::ReturnGoods(const glm::ivec2& prev_target) {
        Command cmd = {};

        cmd.type = CommandType::RETURN_GOODS;
        cmd.handler = CommandHandler_ReturnGoods;
        cmd.target_id = ObjectID();
        cmd.target_pos = prev_target;

        return cmd;
    }

    Command Command::Build(int buildingID, const glm::ivec2& target_pos) {
        Command cmd = {};

        cmd.type = CommandType::BUILD;
        cmd.handler = CommandHandler_Build;
        cmd.target_id = ObjectID(ObjectType::INVALID, 0, buildingID);
        cmd.target_pos = target_pos;

        return cmd;
    }

    Command Command::Repair(const ObjectID& target_id) {
        Command cmd = {};

        cmd.type = CommandType::REPAIR;
        cmd.handler = CommandHandler_Repair;
        cmd.target_id = target_id;

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
        case CommandType::CAST:
            snprintf(buf, sizeof(buf), "Command: Cast (spell: %d, target: %s/(%d, %d))", flag, target_id.to_string().c_str(), target_pos.x, target_pos.y);
            break;
        case CommandType::HARVEST_WOOD:
            snprintf(buf, sizeof(buf), "Command: Harvest wood at (%d, %d)", target_pos.x, target_pos.y);
            break;
        case CommandType::GATHER_RESOURCES:
            snprintf(buf, sizeof(buf), "Command: Gather resources from %s", target_id.to_string().c_str());
            break;
        case CommandType::RETURN_GOODS:
            snprintf(buf, sizeof(buf), "Command: Return goods to %s", target_id.to_string().c_str());
            break;
        case CommandType::BUILD:
            snprintf(buf, sizeof(buf), "Command: Build '%zd' at (%d, %d)", target_id.id, target_pos.x, target_pos.y);
            break;
        case CommandType::REPAIR:
            snprintf(buf, sizeof(buf), "Command: Repair %s", target_id.to_string().c_str());
            break;
        case CommandType::ENTER_TRANSPORT:
            snprintf(buf, sizeof(buf), "Command: Enter Transport %s", target_id.to_string().c_str());
            break;
        case CommandType::TRANSPORT_UNLOAD:
            snprintf(buf, sizeof(buf), "Command: Transport Unload %s", target_id.to_string().c_str());
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

    void CommandHandler_StandGround(Unit& src, Level& level, Command& cmd, Action& action) {
        int res = action.Update(src, level);

        //TODO: implement once Idle action is implemented (or auto command)
        //do the same stuff as in Idle, but nothing that requires movement
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
                    action = Action::Move(src.Position(), target_pos, src.IsTransport());
                }
            }
        }

        cmd.flag = 0;
    }

    void CommandHandler_Attack(Unit& src, Level& level, Command& cmd, Action& action) {
        int res = action.Update(src, level);

        if(res != ACTION_INPROGRESS) {

            //target existence check & value retrieval
            glm::vec2 pos_min, pos_max, target_pos;
            if(cmd.target_id.type != ObjectType::MAP_OBJECT) {
                //target is regular object

                FactionObject* target;
                if(!level.objects.GetObject(cmd.target_id, target) || level.map.IsUntouchable(target->Position(), target->NavigationType() == NavigationBit::AIR)) {
                    //existence check failed - target no longer exists
                    cmd = Command::Idle();
                    return;
                }

                pos_min = target->MinPos();
                pos_max = target->MaxPos();
                target_pos = target->PositionCentered();
            }
            else {
                //target is an attackable map object (wall)

                //check for wall existence (or skip if attack ground was issued)
                const TileData& tile = level.map(cmd.target_pos);
                bool attacking_ground = cmd.flag == 1 && src.IsSiege();
                if(!IsWallTile(tile.tileType) && !attacking_ground) {
                    //existence check failed - target wall no longer exists
                    cmd = Command::Idle();
                    return;
                }

                pos_min = glm::vec2(cmd.target_pos);
                pos_max = cmd.target_pos+1;
                target_pos = (pos_max + pos_min) * 0.5f;
            }

            //range check & action issuing
            if(src.AttackRange() < get_range(src.Position(), pos_min, pos_max)) {
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
                glm::ivec2 itarget_pos = glm::ivec2(target_pos);
                bool leftover = ((target_pos.x - itarget_pos.x) + (target_pos.y - itarget_pos.y)) > 0.9f;
                // ENG_LOG_INFO("TP: ({}, {}), MIN: ({}, {}), {}", target_pos.x, target_pos.y, pos_min.x, pos_min.y, leftover);
                action = Action::Attack(cmd.target_id, itarget_pos, glm::ivec2(pos_min) - src.Position(), src.AttackRange() > 1, leftover);
            }
        }
    }

    void CommandHandler_Cast(Unit& src, Level& level, Command& cmd, Action& action) {
        int res = action.Update(src, level);
        if(res == ACTION_INPROGRESS)
            return;

        int spellID = cmd.flag;

        //techtree and resources check
        if(!src.Faction()->CastConditionCheck(src, spellID)) {
            ENG_LOG_TRACE("Command::Cast - resource check failed.");
            cmd = Command::Idle();
            return;
        }

        //target verification & retrieval
        glm::vec2 pos_min, pos_max, target_pos;
        if(SpellID::RequiresTarget(spellID)) {
            FactionObject* target;
            //spells with target requirement only work on units
            if((cmd.target_id.type != ObjectType::UNIT)|| !level.objects.GetObject(cmd.target_id, target) || level.map.IsUntouchable(target->Position(), target->NavigationType() == NavigationBit::AIR)) {
                //existence check failed - target no longer exists
                cmd = Command::Idle();
                return;
            }

            pos_min = target->MinPos();
            pos_max = target->MaxPos();
            target_pos = target->PositionCentered();
        }
        else {
            pos_min = glm::vec2(cmd.target_pos);
            pos_max = pos_min + 1.f;
            target_pos = pos_min + 0.5f;
        }

        //range check
        int spell_range = SpellID::CastingRange(spellID);
        if(spell_range < get_range(src.Position(), pos_min, pos_max)) {
            //range check failed -> lookup new possible location for casting & start moving there
            glm::ivec2 move_pos = level.map.Pathfinding_NextPosition_Range(src, pos_min, pos_max, spell_range);
            if(move_pos == src.Position()) {
                //no reachable destination found
                cmd = Command::Idle();
            }
            else {
                //initiate new move action
                ASSERT_MSG(has_valid_direction(move_pos - src.Position()), "Command::Cast - target position for next action doesn't respect directions.");
                action = Action::Move(src.Position(), move_pos);
            }
            return;
        }

        //perform the cast action
        glm::ivec2 itarget_pos = glm::ivec2(target_pos);
        bool leftover = ((target_pos.x - itarget_pos.x) + (target_pos.y - itarget_pos.y)) > 0.9f;
        action = Action::Cast(spellID, cmd.target_id, itarget_pos, glm::ivec2(pos_min) - src.Position(), leftover);

        //terminate the command (unless it's a continuous cast)
        if(!SpellID::IsContinuous(spellID))
            cmd = Command::Idle();
    }

    void CommandHandler_Patrol(Unit& src, Level& level, Command& cmd, Action& action) {
        int res = action.Update(src, level);
        if(res == ACTION_INPROGRESS) {
            //periodically break the movement command - allows switching to attack command even when moving on a long straight line
            if(action.data.count >= 3 && action.data.target_pos != src.Position()) {
                action.Signal(src, ActionSignal::INTERRUPT, cmd.Type(), cmd.Type());
            }
            return;
        }

        //store the 2nd patroling waypoint if not already set
        if(cmd.v2 == glm::ivec2(-1))
            cmd.v2 = src.Position();
        
        //scan the unit's vision range for enemy units
        ObjectID targetID = ObjectID();
        glm::ivec2 targetPos = glm::ivec2(-1);
        if(level.map.SearchForTarget(src, level.factions.Diplomacy(), src.VisionRange(), targetID, &targetPos)) {
            //switch to attack command if enemy detected
            ENG_LOG_TRACE("Patrol Command - Enemy detected ({}), switching to attack.", targetID.to_string());
            cmd = Command::Attack(targetID, targetPos);
            return;
        }

        //otherwise, continue moving to the target_pos
        if(cmd.target_pos == src.Position()) {
            //if target_pos reached, switch target_pos with v2
            glm::ivec2 tmp = cmd.target_pos;
            cmd.target_pos = cmd.v2;
            cmd.v2 = tmp;
        }
        
        //consult navmesh - fetch next target position
        glm::ivec2 target_pos = level.map.Pathfinding_NextPosition(src, cmd.target_pos);
        if(target_pos == src.Position()) {
            //target destination unreachable
            cmd = Command::Idle();
        }
        else {
            //initiate new move action
            ASSERT_MSG(has_valid_direction(target_pos - src.Position()), "Command::Patrol - target position for next action doesn't respect directions.");
            action = Action::Move(src.Position(), target_pos);
        }        
    }

    void CommandHandler_Harvest(Unit& src, Level& level, Command& cmd, Action& action) {
        int res = action.Update(src, level);
        if(res == ACTION_INPROGRESS)
            return;

        //validate that it's issued on a worker unit
        if(!src.IsWorker()) {
            ENG_LOG_WARN("Harvest Command - attempting to invoke on a non worker unit.");
            cmd = Command::Idle();
            return;
        }

        //check worker's carry status
        if(src.CarryStatus() != WorkerCarryState::NONE) {
            //transition to return goods command if already carrying
            ENG_LOG_TRACE("Harvest Command - return with goods issued.");
            cmd = Command::ReturnGoods(cmd.target_pos);
            return;
        }

        //lookup new neighboring tile if selected tile doesn't contain trees
        glm::ivec2 tree_pos = {};
        if(!level.map(cmd.target_pos).IsTreeTile()) {
            if(level.map.FindTrees(cmd.target_pos, src.Position(), tree_pos, HARVEST_WOOD_SCAN_RADIUS)) {
                cmd.target_pos = tree_pos;
            }
            else {
                //terminate command if no tree tiles were found within the radius
                cmd = Command::Idle();
                return;
            }
        }

        //check distance to the target tile & move closer if possible
        if(chessboard_distance(src.Position(), cmd.target_pos) > 1) {
            //move until the worker is neighboring the tile or there's nowhere else to move
            glm::ivec2 target_pos = level.map.Pathfinding_NextPosition_Forrest(src, cmd.target_pos);
            if(target_pos != src.Position()) {
                ASSERT_MSG(has_valid_direction(target_pos - src.Position()), "Command::Move - target position for next action doesn't respect directions.");
                action = Action::Move(src.Position(), target_pos);
            }
            //nowhere else to move -> rescan the neighborhood & pick new tree tile
            else if(!level.map.FindTrees(src.Position(), cmd.target_pos, tree_pos, 1)) {
                //no neighboring trees found -> terminate the command
                cmd = Command::Idle();
            }
            else {
                cmd.target_pos = tree_pos;
                action = Action::Harvest(cmd.target_pos, cmd.target_pos - src.Position());
            }
        }
        else {
            //tree tile is in range -> issue a harvest action
            action = Action::Harvest(cmd.target_pos, cmd.target_pos - src.Position());
        }
    }

    void CommandHandler_Gather(Unit& src, Level& level, Command& cmd, Action& action) {
        int res = action.Update(src, level);
        if(res == ACTION_INPROGRESS)
            return;

        //validate that it's issued on a worker unit
        if(!src.IsWorker()) {
            ENG_LOG_WARN("Gather Command - attempting to invoke on a non worker unit.");
            cmd = Command::Idle();
            return;
        }

        glm::ivec2 target_info = glm::ivec2(cmd.target_id.idx, cmd.target_id.id);
        if(src.CarryStatus() != WorkerCarryState::NONE) {
            ENG_LOG_TRACE("Gather Command - return with goods issued.");
            cmd = Command::ReturnGoods(target_info);
            return;
        }
        
        //retrieve the nearest dropoff point for given resource type
        Building* resource = nullptr;
        if(!ObjectID::IsValid(cmd.target_id) || cmd.target_id.type != ObjectType::BUILDING || !level.objects.GetBuilding(cmd.target_id, resource)) {
            //target resource point not found, terminate the command
            ENG_LOG_TRACE("Gather command - target not found.");
            cmd = Command::Idle();
            return;
        }
        ASSERT_MSG(resource != nullptr, "Gather command - resource point should always be set here.");

        if(!resource->IsGatherable(src.NavigationType()) || !(resource->Faction()->ControllerID() == FactionControllerID::NATURE || resource->Faction()->ID() == src.Faction()->ID())) {
            ENG_LOG_TRACE("Gather Command - target not a gatherable resource or faction mismatch ({}).", *resource);
            cmd = Command::Idle();
            return;
        }

        //distance check & movement
        int entrance_range = 1 + int(src.NavigationType() != NavigationBit::GROUND);
        if(get_range(src.Position(), resource->MinPos(), resource->MaxPos()) > entrance_range) {
            //move until the worker stands next to the building
            glm::ivec2 target_pos = level.map.Pathfinding_NextPosition_Range(src, resource->MinPos(), resource->MaxPos(), 1);
            if(target_pos != src.Position()) {
                ASSERT_MSG(has_valid_direction(target_pos - src.Position()), "Command::Move - target position for next action doesn't respect directions.");
                action = Action::Move(src.Position(), target_pos);
            }
            else {
                //destination unreachable
                cmd = Command::Idle();
            }
        }
        else {
            //worker is next to the resource -> inform entrance controller and terminate the command
            level.objects.IssueEntrance_Work(cmd.target_id, src.OID(), target_info, src.CarryStatus());
            cmd = Command::Idle();
        }
    }

    void CommandHandler_ReturnGoods(Unit& src, Level& level, Command& cmd, Action& action) {
        int res = action.Update(src, level);
        if(res == ACTION_INPROGRESS)
            return;
        
        //validate that it's issued on a worker unit
        if(!src.IsWorker()) {
            ENG_LOG_WARN("ReturnGoods Command - attempting to invoke on a non worker unit.");
            cmd = Command::Idle();
            return;
        }

        if(src.CarryStatus() == WorkerCarryState::NONE) {
            ENG_LOG_WARN("ReturnGoods Command - selected worker doesn't carry any resources.");
            cmd = Command::Idle();
            return;
        }

        //retrieve the nearest dropoff point for given resource type
        Building* dropoff = nullptr;
        if(!ObjectID::IsValid(cmd.target_id) || cmd.target_id.type != ObjectType::BUILDING || !level.objects.GetBuilding(cmd.target_id, dropoff)) {
            glm::ivec2 next_pos = glm::ivec2();

            //dropoff not set or is invalid -> lookup new one
            if(level.map.Pathfinding_NextPosition_NearestBuilding(src, src.Faction()->DropoffPoints(), cmd.target_id, next_pos)) {
                dropoff = &level.objects.GetBuilding(cmd.target_id);
                ASSERT_MSG(dropoff->IsActive(), "DropoffPoints() list should always contain only active buildings.");
                ENG_LOG_TRACE("ReturnGoods - using '{}' as dropoff point for '{}'", *dropoff, src);

                //path found, issue movement action
                if(next_pos != src.Position()) {
                    ASSERT_MSG(has_valid_direction(next_pos - src.Position()), "Command::Move - target position for next action doesn't respect directions.");
                    action = Action::Move(src.Position(), next_pos);
                    return;
                }
            }
            else {
                //terminate the command if there's no reachable dropoff point
                cmd = Command::Idle();
                return;
            }
        }
        ASSERT_MSG(dropoff != nullptr, "ReturnGoods command - dropoff building should always be set here.");

        //distance check & movement
        int entrance_range = 1 + int(src.NavigationType() != NavigationBit::GROUND);
        if(get_range(src.Position(), dropoff->MinPos(), dropoff->MaxPos()) > entrance_range) {
            //move until the worker stands next to the building
            glm::ivec2 target_pos = level.map.Pathfinding_NextPosition_Range(src, dropoff->MinPos(), dropoff->MaxPos(), 1);
            if(target_pos != src.Position()) {
                ASSERT_MSG(has_valid_direction(target_pos - src.Position()), "Command::Move - target position for next action doesn't respect directions.");
                action = Action::Move(src.Position(), target_pos);
            }
            else {
                //destination unreachable
                cmd = Command::Idle();
            }
        }
        else {
            //worker is next to the building -> inform entrance controller and terminate the command
            src.Faction()->ResourcesUptick(src.CarryStatus());
            level.objects.IssueEntrance_Work(cmd.target_id, src.OID(), cmd.target_pos, src.CarryStatus());
            cmd = Command::Idle();
        }
    }

    void CommandHandler_Build(Unit& src, Level& level, Command& cmd, Action& action) {
        int res = action.Update(src, level);
        if(res == ACTION_INPROGRESS)
            return;

        //validate that it's issued on a worker unit
        if(!src.IsWorker()) {
            ENG_LOG_WARN("Build Command - attempting to invoke on a non worker unit.");
            cmd = Command::Idle();
            return;
        }

        //retrieve data of the building to be built (also serves as conditions check)
        BuildingDataRef buildingData = src.Faction()->FetchBuildingData(cmd.target_id.id, (bool)src.Race());
        if(buildingData == nullptr) {
            //building cannot be built
            cmd = Command::Idle();
            return;
        }

        glm::ivec2 building_size = buildingData->size;
        int buildable_distance = int(buildingData->coastal);

        if(get_range(src.Position(), cmd.target_pos, cmd.target_pos + building_size) >= (1 + buildable_distance)) {
            //move until the worker stands within the rectangle (where the building is to be built)
            glm::ivec2 target_pos = level.map.Pathfinding_NextPosition_Range(src, cmd.target_pos, cmd.target_pos + building_size, buildable_distance);
            if(target_pos != src.Position()) {
                ASSERT_MSG(has_valid_direction(target_pos - src.Position()), "Command::Move - target position for next action doesn't respect directions.");
                action = Action::Move(src.Position(), target_pos);
            }
            else {
                //destination unreachable
                ENG_LOG_TRACE("Command::Build - cannot reach the destination");
                cmd = Command::Idle();
            }
        }
        else {
            //validate that the area is clear & all the building conditions are met
            if(level.map.IsAreaBuildable(cmd.target_pos, building_size, buildingData->navigationType, src.Position(), buildingData->coastal, buildingData->requires_foundation) && src.Faction()->CanBeBuilt(cmd.target_id.id, (bool)src.Race())) {
                //start the construction
                src.WithdrawObject();
                if(!buildingData->requires_foundation) {
                    //regular building construction
                    ObjectID buildingID = level.objects.EmplaceBuilding(level, buildingData, src.Faction(), cmd.target_pos, false);
                    level.objects.IssueEntrance_Construction(buildingID, src.OID());
                }
                else {
                    //construction on existing foundation (oil patch)
                    ObjectID buildingID = level.map.GetFoundationObjectAt(cmd.target_pos);
                    ASSERT_MSG(ObjectID::IsValid(buildingID), "Command::Build - foundation not found.");
                    buildingID = level.objects.GetBuilding(buildingID).TransformFoundation(buildingData, src.Faction());
                    level.objects.IssueEntrance_Construction(buildingID, src.OID());
                }
            }
            else {
                ENG_LOG_TRACE("Command::Build - cannot build at ({}, {})", cmd.target_pos.x, cmd.target_pos.y);
            }
            cmd = Command::Idle();
        }
    }

    void CommandHandler_Repair(Unit& src, Level& level, Command& cmd, Action& action) {
        int res = action.Update(src, level);
        if(res == ACTION_INPROGRESS)
            return;

        //validate that it's issued on a worker unit
        if(!src.IsWorker()) {
            ENG_LOG_WARN("Repair Command - attempting to invoke on a non worker unit.");
            cmd = Command::Idle();
            return;
        }

        //validate that target still exists (& that it's a building)
        Building* target = nullptr;
        if(!level.objects.GetBuilding(cmd.target_id, target) || target->IsFullHealth()) {
            cmd = Command::Idle();
            return;
        }
        glm::ivec2 min_pos = target->MinPos();
        glm::ivec2 max_pos = target->MaxPos();

        //distance check & movement
        if(get_range(src.Position(), min_pos, max_pos) > 1) {
            //move until the worker stands next to the building
            glm::ivec2 target_pos = level.map.Pathfinding_NextPosition_Range(src, min_pos, max_pos, 1);
            if(target_pos != src.Position()) {
                ASSERT_MSG(has_valid_direction(target_pos - src.Position()), "Command::Move - target position for next action doesn't respect directions.");
                action = Action::Move(src.Position(), target_pos);
            }
            else {
                //destination unreachable
                cmd = Command::Idle();
            }
        }
        else {
            //worker within range -> start new repair action
            action = Action::Repair(cmd.target_id, target->Position() - src.Position());
        }
    }

    void CommandHandler_EnterTransport(Unit& src, Level& level, Command& cmd, Action& action) {
        int res = action.Update(src, level);
        if(res == ACTION_INPROGRESS)
            return;

        //validate that unit is ground unit
        if(src.NavigationType() != NavigationBit::GROUND) {
            ENG_LOG_TRACE("Enter Transport Command - Non-ground unit cannot enter a transport.");
            cmd = Command::Idle();
            return;
        }

        //retrieve the target & validate that it's a transport ship
        Unit* target = nullptr;
        if(!level.objects.GetUnit(cmd.target_id, target) || !target->IsTransport()) {
            ENG_LOG_TRACE("Enter Transport Command - target is invalid.");
            cmd = Command::Idle();
            return;
        }

        //check that the transport ship isn't full
        if(target->Transport_IsFull()) {
            ENG_LOG_TRACE("Enter Transport Command - target transport ship is full.");
            cmd = Command::Idle();
            return;
        }

        //first command tick -> do a docking request onto the transport
        if(cmd.flag) {
            target->Transport_DockingRequest(src.Position());
            cmd.flag = 0;
        }

        //if the ship is still moving, update target position
        bool still_in_motion = target->Transport_GoingDocking();
        if(still_in_motion) {
            cmd.target_pos = target->Transport_GetDockingLocation();
        }

        //when no target position was ever set (should at least make the unit move to the shore)
        if(cmd.target_pos == glm::ivec2(-1)) {
            ENG_LOG_TRACE("Command::EnterTransport - no docking was issued.");
            cmd.target_pos = target->Position();
        }

        //TODO: account for the fact that docking location will be water tile
        //TODO: docking on odd tile coordinates (when the coast is at odd coords)
        //          - both problems might be solved by distance to enter=2

        //check distance to the docking location (thresh=2, bcs boats move on even tiles)
        int fu = chessboard_distance(src.Position(), cmd.target_pos);
        if(fu > 2) {
            glm::ivec2 target_pos = level.map.Pathfinding_NextPosition(src, cmd.target_pos);
            if(target_pos != src.Position()) {
                ASSERT_MSG(has_valid_direction(target_pos - src.Position()), "Command::Move - target position for next action doesn't respect directions.");
                action = Action::Move(src.Position(), target_pos);
            }
            else {
                //destination unreachable
                if (still_in_motion)
                    action = Action::Idle();
                else
                    cmd = Command::Idle();
            }
        }
        else {
            //wait for both units to finish the movement action
            if(target->InMotion()) {
                action = Action::Idle();
            }
            else {
                //enter the ship & terminate the command
                ObjectID transport_id = cmd.target_id;
                cmd = Command::Idle();
                level.objects.IssueEntrance_Transport(transport_id, src.OID());
                level.factions.Player()->SignalGUIUpdate(*target);
            }
        }
    }

    void CommandHandler_TransportUnload(Unit& src, Level& level, Command& cmd, Action& action) {
        int res = action.Update(src, level);
        if(res == ACTION_INPROGRESS)
            return;

        //command target: could be issued to unload all or to unload a single (selected) unit -> gotta distinguish between those options

        //TODO: also need to check how the unload works ingame (ie. when unloading all - does it all happen in one tick or one-by-one?)
            //if they get unloaded one-by-one, then it might be easier to create Unload action as well & keep issuing it until there's nothing to unload
            //the action would handle notifying the EntranceController as well as cooldown between individual exits

        //validate that it's issued onto a transport ship
        if(!src.IsTransport()) {
            ENG_LOG_TRACE("TransportUnload Command - needs to be issued on a transport ship.");
            cmd = Command::Idle();
            return;
        }

        //validate that it isn't empty
        if(src.Transport_IsEmpty()) {
            ENG_LOG_TRACE("TransportUnload Command - transport ship is empty.");
            cmd = Command::Idle();
            return;
        }

        //validate that it's docked
        if(!level.map.IsDockingLocation(src.Position())) {
            ENG_LOG_TRACE("TransportUnload Command - transport ship needs to be docked in order to unload.");
            cmd = Command::Idle();
            return;
        }

        if(cmd.flag == 1) {
            //issue exit on single (handpicked) unit
            if(!level.objects.IssueExit_Transport(src.OID(), cmd.target_id)) {
                ENG_LOG_TRACE("TransportUnload Command - failed to unload unit {}.", cmd.target_id);
            }
        }
        else {
            //issue exits on all units in the transport
            auto [unloaded, total] = level.objects.IssueExit_Transport(src.OID());
            ENG_LOG_TRACE("TransportUnload Command - unloaded {}/{} units from the transport.", unloaded, total);
        }

        level.factions.Player()->SignalGUIUpdate(src);
        cmd = Command::Idle();
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

    BuildingAction BuildingAction::TrainOrResearch(bool training, int payload_id, float required_time) {
        BuildingAction act = {};
        act.logic = BuildingAction::Logic(BuildingActionType::TRAIN_OR_RESEARCH, BuildingAction_TrainOrResearch);
        act.data = {};
        act.data.flag = training;
        act.data.i = payload_id;
        act.data.t3 = required_time;
        return act;
    }

    BuildingAction BuildingAction::Upgrade(int payload_id) {
        BuildingAction act = BuildingAction();
        act.data.flag = true;       //mark as upgrade (instead of construction)
        act.data.i = payload_id;
        return act;
    }
    
    void BuildingAction::Update(Building& source, Level& level) {
        ASSERT_MSG(logic.update != nullptr, "BuildingAction::Update - handler should never be null.");
        logic.update(source, level, *this);
    }

    bool BuildingAction::IsConstructionAction() const {
        return (logic.type == BuildingActionType::CONSTRUCTION_OR_UPGRADE && !data.flag);
    }

    bool BuildingAction::CustomConstructionViz() const {
        float progress = data.t1;
        int target = data.t3;
        return int((progress / target) * 4.f) >= 2;
    }

    std::string BuildingAction::to_string() const {
        char buf[2048];

        switch(logic.type) {
        case BuildingActionType::IDLE:
        case BuildingActionType::IDLE_ATTACK:
            snprintf(buf, sizeof(buf), "Action: Idle");
            break;
        case BuildingActionType::CONSTRUCTION_OR_UPGRADE:
        {
            float progress = data.t3 != 0 ? int((data.t1/data.t3)*100.f) : 0.f;
            snprintf(buf, sizeof(buf), "Action: %s - %.0f/%.0f (%.0f%%)", data.flag ? "Upgrade" : "Construction", data.t1, data.t3, progress);
            break;
        }
        case BuildingActionType::TRAIN_OR_RESEARCH:
        {
            float progress = data.t3 != 0 ? int((data.t1/data.t3)*100.f) : 0.f;
            snprintf(buf, sizeof(buf), "Action: %s - %.0f/%.0f (%.0f%%)", data.flag ? "Train" : "Research", data.t1, data.t3, progress);
            break;
        }
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

        src.real_act() = BuildingAnimationType::IDLE;
    }

    void BuildingAction_Attack(Building& src, Level& level, BuildingAction& action) {
        if(!src.CanAttack()) {
            action = BuildingAction::Idle();
        }

        src.real_act() = BuildingAnimationType::IDLE;
        
        //=====
        Input& input = Input::Get();
        float& attack_tick = action.data.t1;
        bool& engaged = action.data.flag;
        int& scan_counter = action.data.i;
        ObjectID& targetID = action.data.target_id;
        //=====

        //attack disengaged, because of no target
        if(!engaged) {
            //periodically rescan the vincinity for possible targets
            if((++scan_counter) > BUILDING_ATTACK_SCAN_INTERVAL) {
                scan_counter = 0;
                if(level.map.SearchForTarget(src, level.factions.Diplomacy(), src.AttackRange(), targetID)) {
                    engaged = true;
                    attack_tick = (float)input.CurrentTime();
                    ENG_LOG_TRACE("BuildingAttack - Found target to attack.");
                }
            }
            return;
        }
        
        //attack timer tick
        float attack_gap = src.AttackSpeed() / Config::GameSpeed();
        if(attack_tick + attack_gap <= (float)Input::CurrentTime()) {
            //validate the target or search for new one within range
            FactionObject* target;
            if(!level.objects.GetObject(targetID, target) || !src.RangeCheck(*target)) {
                //previous target no longer exists or is out of range, so search for new one
                if(!level.map.SearchForTarget(src, level.factions.Diplomacy(), src.AttackRange(), targetID)) {
                    engaged = false;
                    scan_counter = 0;
                    ENG_LOG_TRACE("BuildingAttack - Target lost.");
                }
                else {
                    target = &level.objects.GetObject(targetID);
                }
            }
            ASSERT_MSG(target != nullptr, "Target should always be set here.");
            
            //target still valid (or found new one)
            if(engaged) {
                //spawn a projectile
                UtilityObjectDataRef projectile_data = src.FetchProjectileRef();
                if(projectile_data != nullptr) {
                    level.objects.EmplaceUtilityObj(level, projectile_data, target->Position(), targetID, src);
                    ENG_LOG_TRACE("BuildingAttack - Projectile spawned (target = {}).", *target);
                }
                else {
                    ENG_LOG_WARN("BuildingAttack - cannot spawn projectile, reference is missing.");
                }

                //reset the attack timer
                attack_tick = (float)input.CurrentTime();
            }
        }
    }

    void BuildingAction_TrainOrResearch(Building& src, Level& level, BuildingAction& action) {
        Input& input = Input::Get();
        float& progress = action.data.t1;
        float& prev_health = action.data.t2;
        bool is_training = action.data.flag;
        int payload_id = action.data.i;
        float target = action.data.t3;
        //=====

        //compute the uptick for this frame
        float uptick = input.deltaTime * CONSTRUCTION_SPEED;

        //increment construction tracking as well as building health
        progress += uptick;

        // src.real_act() = BuildingAnimationType::IDLE;

        //construction finished condition
        if(progress >= target) {
            src.TrainingOrResearchFinished(is_training, payload_id);
        }
    }

    void BuildingAction_ConstructOrUpgrade(Building& src, Level& level, BuildingAction& action) {
        Input& input = Input::Get();
        float& progress = action.data.t1;
        float& prev_health = action.data.t2;
        int& next_tick = action.data.i;         //only for construction, for upgrade, it carries payloadID
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
            src.real_act() = BuildingAnimationType::BUILD1 + state;
        }
        else {
            src.real_act() = BuildingAnimationType::UPGRADE;
        }

        //construction finished condition
        if(progress >= target) {
            if(is_construction) {
                src.OnConstructionFinished();
            }
            else {
                src.OnUpgradeFinished(action.data.i);
            }
        }
    }

}//namespace eng
