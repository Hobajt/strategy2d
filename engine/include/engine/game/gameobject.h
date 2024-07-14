#pragma once

#include "engine/utils/pool.hpp"
#include "engine/core/animator.h"

#include "engine/game/object_data.h"
#include "engine/game/command.h"
#include "engine/game/techtree.h"

#include <ostream>

#define TRANSPORT_MAX_LOAD 6

namespace eng {

    class Level;

    class FactionController;
    using FactionControllerRef = std::shared_ptr<FactionController>;

    using idMappingType = std::unordered_map<ObjectID, ObjectID>;

    //===== GameObject =====
    
    class GameObject {
        friend class ObjectPool;
    public:
        struct Entry;
    public:
        GameObject() = default;
        GameObject(Level& level, const GameObjectDataRef& data, const glm::vec2& position = glm::vec2(0.f));
        GameObject(Level& level, const GameObjectDataRef& data_, const GameObject::Entry& entry);

        //copy disabled
        GameObject(const GameObject&) = delete;
        GameObject& operator=(const GameObject&) = delete;

        //move enabled
        GameObject(GameObject&&) noexcept = default;
        GameObject& operator=(GameObject&&) noexcept = default;

        virtual void Render();
        void RenderAt(const glm::vec2& map_pos);
        void RenderAt(const glm::vec2& map_pos, const glm::vec2& size, float zOffset = 0.f);
        void RenderAt(const glm::vec2& map_pos, const glm::vec2& size, float scaling, bool centered, float zOffset = 0.f);
        void RenderAt(const glm::vec2& map_pos, const glm::vec2& size, int action, float frame, float zOffset = 0.f);

        //Return true when requesting object removal.
        virtual bool Update();

        void Export(GameObject::Entry& entry) const;

        void LevelPtrUpdate(Level& lvl);

        //Returns false to signal that the object is being reused & shouldn't be removed (for the use in ObjectPool).
        //silent = purely to remove the object (without sounds, corpse, death animation)
        virtual bool Kill(bool silent = false) { killed = true; return true; }

        //Check if this object covers given map coordinates.
        bool CheckPosition(const glm::ivec2& map_coords);

        //Checks nearby tiles & returns the first tile, where a unit can be spawned.
        //Coords are returned through parameter, function itself returns spawnpoint's chessboard distance from the building corner.
        int NearbySpawnCoords(int navType, int preferred_direction, glm::ivec2& out_coords, int max_range = -1);

        int Orientation() const { return orientation; }
        virtual int ActionIdx() const { return actionIdx; }
        glm::ivec2 Position() const { return position; }
        glm::vec2 Positionf() const { return glm::vec2(position); }
        virtual glm::vec2 RenderPosition() const { return glm::vec2(position); }
        virtual glm::vec2 RenderSize() const { return glm::vec2(data->size); }

        std::pair<glm::vec2, glm::vec2> AABB() const;
        glm::vec4 AABB_v4() const;

        glm::vec2 MinPos() const { return glm::vec2(position); }
        glm::vec2 MaxPos() const { return glm::vec2(position) + data->size - 1.f; }

        glm::ivec4 Price() const { return data->cost; }

        glm::ivec2& pos() { return position; }
        int& ori() { return orientation; }
        int& act() { return actionIdx; }
        Level* lvl();
        const Level* lvl() const;

        float AnimationProgress() const { return animator.GetCurrentFrame(); }
        bool AnimKeyframeSignal() const { return animator.KeyframeSignal(); }
        bool AnimKeyframeSignal(int actionIdx) const { return animator.KeyframeSignal(actionIdx); }
        void SwitchAnimatorAction(int actionIdx, bool forceReset = false) { animator.SwitchAction(actionIdx, forceReset); }

        int AnimationCount() const { return animator.ActionCount(); }
        float AnimationDuration(int actionIdx) const { return animator.GetAnimationDuration(actionIdx); }

        int ID() const { return id; }
        ObjectID OID() const { return oid; }
        glm::ivec3 NumID() const { return data->num_id; }
        std::string StrID() const { return data->str_id; }
        std::string Name() const { return data->name; }

        GameObjectDataRef Data() const { return data; }
        int NavigationType() const { return data->navigationType; }

        void Anim_DBG_Print(int actionIdx = -1) const { animator.DBG_Print(actionIdx); }

        void DBG_GUI();
        friend std::ostream& operator<<(std::ostream& os, const GameObject& go);
    protected:
        virtual void Inner_DBG_GUI();
        bool IsKilled() const { return killed; }

        void UpdateDataPointer(const GameObjectDataRef& data_) { data = data_; }
        void UnsetKilledFlag() { killed = false; }
    private:
        virtual std::ostream& DBG_Print(std::ostream& os) const;
        static int PeekNextID() { return idCounter; }
        static int GetNextID() { return idCounter++; }
        void IntegrateIntoLevel(const ObjectID& oid_) { oid = oid_; InnerIntegrate(); }
        virtual void InnerIntegrate() {}

        float ZIndex(const glm::vec2& pos, const glm::vec2& size, float zOffset);
    protected:
        Animator animator;
    private:
        GameObjectDataRef data = nullptr;
        Level* level = nullptr;

        glm::ivec2 position = glm::ivec2(0);
        int orientation = 0;
        int actionIdx = 0;
        bool killed = false;

        int id = -1;
        ObjectID oid = {};
        static int idCounter;
    };

    //===== FactionObject =====

    class FactionObject : public GameObject {
    public:
        struct Entry;
    public:
        FactionObject() = default;
        FactionObject(Level& level, const FactionObjectDataRef& data, const FactionControllerRef& faction, const glm::vec2& position, bool is_finished);
        FactionObject(Level& level, const FactionObjectDataRef& data, const FactionControllerRef& faction, const glm::vec2& position = glm::vec2(0.f), int colorIdx = -1, bool is_finished = true);
        FactionObject(Level& level, const FactionObjectDataRef& data, const FactionControllerRef& faction, float health_percentage, const glm::vec2& position = glm::vec2(0.f), int colorIdx = -1, bool is_finished = true);
        FactionObject(Level& level, const FactionObjectDataRef& data, const FactionObject::Entry& entry, bool is_finished = true);
        virtual ~FactionObject();

        //move enabled
        FactionObject(FactionObject&&) noexcept = default;
        FactionObject& operator=(FactionObject&&) noexcept = default;

        void ChangeColor(int colorIdx);
        void ResetColor();
        void ChangeFaction(const FactionControllerRef& new_faction, bool keep_color = false);

        void Export(FactionObject::Entry& entry) const;

        FactionObjectDataRef DataF() { return data_f; }

        virtual bool Kill(bool silent = false) override;

        //Return true if given object is within this object's attack range.
        bool RangeCheck(GameObject& target) const;
        bool RangeCheck(const glm::ivec2& min_pos, const glm::ivec2& max_pos) const;

        int GetRange(GameObject& target) const;
        int GetRange(const glm::ivec2& min_pos, const glm::ivec2& max_pos) const;

        //Accounting for upscaled visuals (ie. ships take 2x2 tiles).
        virtual glm::vec2 PositionCentered() const;
        virtual glm::vec2 RenderPositionCentered() const { return PositionCentered(); }

        bool CanAttack() const { return data_f->CanAttack(); }
        float Cooldown() const { return data_f->cooldown; }

        virtual bool DetectablyInvisible() const { return false; }

        //====== object stats getters - anything Base/Bonus prefix returns partial value (mostly for GUI), the rest returns final stat value ======

        virtual int BasicDamage() const { return data_f->basic_damage; }
        virtual int PierceDamage() const { return data_f->pierce_damage + BonusDamage(); }
        int BonusDamage() const { return IsUnit() ? Tech().BonusDamage(NumID()[1], IsOrc()) : 0; }
        int BaseMinDamage() const { return int(roundf(data_f->pierce_damage * 0.5f)); }
        int BaseMaxDamage() const { return data_f->basic_damage + data_f->pierce_damage; }

        int Armor() const { return BaseArmor() + BonusArmor(); }
        int BaseArmor() const { return data_f->armor; }
        int BonusArmor() const { return IsUnit() ? Tech().BonusArmor(NumID()[1], IsOrc()) : 0; }

        int AttackRange() const { return BaseAttackRange() + BonusAttackRange(); }
        int BaseAttackRange() const { return data_f->attack_range; }
        int BonusAttackRange() const { return IsUnit() ? Tech().BonusRange(NumID()[1], IsOrc()) : 0; }

        int VisionRange() const { return BaseVisionRange() + BonusVisionRange(); }
        int BaseVisionRange() const { return data_f->vision_range; }
        int BonusVisionRange() const { return IsUnit() ? Tech().BonusVision(NumID()[1], IsOrc()) : 0; }

        virtual bool IsInvulnerable() const {return data_f->invulnerable; }

        virtual bool IsTransport() const { return false; }

        //===============

        int BuildTime() const { return data_f->build_time; }
        int MaxHealth() const { return data_f->health; }
        int Health() const { return int(health); }
        int MissingHealth() const { return int(data_f->health - health); }
        float HealthPercentage() const { return float(health) / data_f->health; }
        glm::ivec2 Icon() const { return data_f->icon; }
        bool IsFullHealth() const { return int(health) == data_f->health; }

        //For melee damage application.
        int ApplyDirectDamage(const FactionObject& source);
        int ApplyDirectDamage(int src_basicDmg, int src_pierceDmg);
        int ApplyDamageFlat(int dmg);

        void SetHealth(int health);
        void AddHealth(float value);

        bool IsUnit() const { return NumID()[0] == ObjectType::UNIT; }

        virtual bool IsGatherable() const { return false; }
        virtual bool IsGatherable(int unitNavType) const { return false; }
        bool IsRepairable() const { return !IsUnit(); }
        virtual int DeathAnimIdx() const { return -1; }
        int Race() const { return data_f->race; }
        bool IsOrc() const { return bool(data_f->race); }
        const ButtonDescriptions& GetButtonDescriptions() const { return data_f->gui_btns; }

        int VariationIdx() const { return variationIdx; }
        void SetVariationIdx(int idx) { variationIdx = idx; }

        FactionControllerRef Faction() { return faction; }
        const FactionControllerRef Faction() const { return faction; }
        int FactionIdx() const { return factionIdx; }

        Techtree& Tech();
        const Techtree& Tech() const;

        bool IsActive() const { return active; }

        //Deactivates the object & removes it from pathfinding
        virtual void WithdrawObject(bool inform_faction = false);
        //Reactivates the object & adds it back to pathfinding.
        void ReinsertObject();
        void ReinsertObject(const glm::ivec2& position);

        void MoveTo(const glm::ivec2& position);

        UtilityObjectDataRef FetchProjectileRef() const { return data_f->projectile; }

        SoundEffect& Sound_Yes() const { return data_f->sound_yes; }
    protected:
        virtual void Inner_DBG_GUI() override;
        virtual void InnerIntegrate() override;
        void RemoveFromMap();

        //Resets object's state, used when reinserting object into the level.
        virtual void ClearState() { variationIdx = 0; }

        void UpdateDataPointer(const FactionObjectDataRef& data_);
        void Finalized(bool state) { finalized = state; }
    private:
        static bool ValidColor(int idx);
    private:
        FactionObjectDataRef data_f = nullptr;
        FactionControllerRef faction = nullptr;
        int factionIdx;
        int colorIdx;

        float health;

        int variationIdx = 0;       //to enable variations for certain animations (increment based on how many overridable animations there are)

        bool active = true;
        bool finalized = true;
        bool faction_informed = true;
    };

    namespace UnitEffectType {
        enum {
            SLOW, HASTE, INVISIBILITY, BLOODLUST, UNHOLY_ARMOR,
            COUNT
        };
    }//namespace UnitEffectType

    struct UnitEffects {
        std::array<bool, UnitEffectType::COUNT> flags = {};
    public:
        bool& operator[](int idx);
        const bool& operator[](int idx) const;

        std::string to_string() const;
        friend std::ostream& operator<<(std::ostream& os, const UnitEffects& eff);
    };

    //===== Unit =====

    class Unit : public FactionObject {
        friend class Command;
        friend class Action;
    public:
        struct Entry;
    public:
        Unit() = default;
        Unit(Level& level, const UnitDataRef& data, const FactionControllerRef& faction, const glm::vec2& position = glm::vec2(0.f), bool playReadySound = true);
        Unit(Level& level, const Unit::Entry& entry);
        virtual ~Unit();

        //move enabled
        Unit(Unit&&) noexcept = default;
        Unit& operator=(Unit&&) noexcept = default;

        virtual void Render() override;
        virtual bool Update() override;

        Unit::Entry Export() const;
        void RepairIDs(const idMappingType& ids);

        bool PassiveMindset() const;

        //Issue new command (by overriding the existing one).
        void IssueCommand(const Command& cmd);

        virtual int ActionIdx() const override;

        virtual glm::vec2 PositionCentered() const override;

        virtual bool IsInvulnerable() const override;
        virtual int BasicDamage() const override;
        virtual int PierceDamage() const override;

        bool UnitUpgrade(int factionID, int old_type, int new_type, bool isOrcUnit);
        ObjectID Transform(const UnitDataRef& new_data, const FactionControllerRef& new_faction, bool preserve_health = false);

        int MoveSpeed() const { return data->speed; }
        int UnitLevel() const { return Tech().UnitLevel(NumID()[1], IsOrc()); }
        bool IsCaster() const { return data->caster; }
        int Mana() const { return int(mana); }
        void Mana(int value) { mana = value; }
        float ManaPercentage() const { return mana / 255.f; }
        void DecreaseMana(int value);
        
        float MoveSpeed_Real() const;
        float SpeedBuffValue() const;

        int AttackUpgradeSource() const { return data->upgrade_src[0]; }
        int DefenseUpgradeSource() const { return data->upgrade_src[1]; }

        glm::vec2& m_offset() { return move_offset; }
        virtual glm::vec2 RenderPosition() const override;
        virtual glm::vec2 RenderSize() const override;
        virtual glm::vec2 RenderPositionCentered() const override;

        bool IsUndead() const;

        virtual bool DetectablyInvisible() const override;

        bool AnimationFinished() const { return animation_ended; }

        UnitDataRef UData() const { return data; }
        virtual int DeathAnimIdx() const override { return data->deathAnimIdx; }

        bool IsWorker() const { return data->worker; }
        int CarryStatus() const { return carry_state; }
        void ChangeCarryStatus(int carry_state);

        bool IsSiege() const { return data->siege; }
        bool RotateWhenAttacking() const { return data->attack_rotated; }
        virtual bool IsTransport() const override { return data->transport; }

        SoundEffect& Sound_Attack() const { return data->sound_attack; }

        bool Transport_IsFull() const { return carry_state >= TRANSPORT_MAX_LOAD; }
        bool Transport_IsEmpty() const { return carry_state == 0; }
        int Transport_CurrentLoad() const { return carry_state; }

        void Transport_UnitAdded();
        void Transport_UnitRemoved();

        //Conditionally issues a move command onto the ship to go to an accessible coast tile.
        //Movement is issued only if the ship already isn't on its way to dock (on any coast tile).
        void Transport_DockingRequest(const glm::ivec2& unitPosition);

        //Returns true if the transport is currently under move command to a coast tile.
        bool Transport_GoingDocking() const;
        glm::ivec2 Transport_GetDockingLocation() const;

        bool InMotion() const;

        void SetInvisible(bool state);
        void SetEffectFlag(int idx, bool state);
        bool GetEffectFlag(int idx);

        static bool IsMinion(const glm::ivec3& num_id);
        static bool IsRessurectable(const glm::ivec3& num_id, int navType);
    protected:
        virtual void Inner_DBG_GUI() override;
    private:
        virtual std::ostream& DBG_Print(std::ostream& os) const override;
        virtual void ClearState() override;

        void UpdateVariationIdx();
        void ManaIncrement();
        void TrollRegeneration();
    private:
        UnitDataRef data = nullptr;

        Command command = Command();
        Action action = Action();

        glm::vec2 move_offset = glm::vec2(0.f);
        bool animation_ended = false;

        int carry_state = WorkerCarryState::NONE;        //worker load indicator
        float mana = 0;

        UnitEffects effects = {};
    };

    //===== Building =====

    class Building : public FactionObject {
    public:
        struct Entry;
    public:
        Building() = default;
        Building(Level& level, const BuildingDataRef& data, const FactionControllerRef& faction, const glm::vec2& position = glm::vec2(0.f), bool constructed = false);
        Building(Level& level, const Building::Entry& entry);
        virtual ~Building();

        //move enabled
        Building(Building&&) noexcept = default;
        Building& operator=(Building&&) noexcept = default;

        virtual void Render() override;
        virtual bool Update() override;

        Building::Entry Export() const;
        void RepairIDs(const idMappingType& ids);

        void IssueAction(const BuildingAction& action);
        void CancelAction();

        virtual int ActionIdx() const override;
        virtual bool IsGatherable() const override { return data->gatherable; }
        virtual bool IsGatherable(int unitNavType) const override;
        bool IsResource() const { return data->resource || data->gatherable; }
        int AmountLeft() const { return IsResource() ? amount_left : 0; }
        void SetAmountLeft(int value) { amount_left = value; }
        void DecrementResource();

        bool IsCoastal() const { return data->coastal; }

        virtual void WithdrawObject(bool inform_faction = false) override;

        ObjectID TransformFoundation(const BuildingDataRef& new_data, const FactionControllerRef& new_faction, bool is_constructed = false);

        virtual bool Kill(bool silent = false) override;

        //==== Building properties ====

        //Defines the speed of upgrade operation (not relevant when building cannot upgrade).
        int UpgradeTime() const { return data->upgrade_time; }
        int DropoffMask() const { return data->dropoff_mask; }
        bool CanDropoffResource(int resourceType) const { return (data->dropoff_mask & resourceType) != 0; }

        //How much health should the building start with when constructing.
        int StartingHealth() const { return MaxHealth() > 100 ? 40 : 10; }

        float AttackSpeed() const { return data->attack_speed; }

        static int WinterSpritesOffset() { return BuildingAnimationType::COUNT; }

        bool Constructed() const { return constructed; }
        int BuildActionType() const { return action.logic.type; }
        bool IsTraining() const;
        bool ProgressableAction() const;
        float ActionProgress() const;
        glm::ivec3 ActionPrice() const;

        float TrainOrResearchTime(bool training, int payload_id) const;

        void OnConstructionFinished(bool registerDropoffPoint = true, bool kickoutWorkers = true);
        void OnUpgradeFinished(int ref_idx);
        void TrainingOrResearchFinished(bool training, int payload_id);

        glm::ivec2& LastBtnIcon() { return last_btn_icon; }
        const glm::ivec2& LastBtnIcon() const { return last_btn_icon; }

        int& real_act() { return real_actionIdx; }
    protected:
        virtual void Inner_DBG_GUI() override;
        virtual void InnerIntegrate() override;
        virtual void ClearState() override;
    private:
        virtual std::ostream& DBG_Print(std::ostream& os) const override;
        void UnregisterDropoffPoint();
    private:
        BuildingDataRef data = nullptr;
        BuildingAction action = BuildingAction();

        bool constructed = false;
        int amount_left = 0;
        glm::ivec2 last_btn_icon = glm::ivec2(-1);
        int real_actionIdx = 0;
    };

    //===== UtilityObject =====

    class UtilityObject : public GameObject {
    public:
        struct LiveData {
            glm::vec2 source_pos;
            glm::vec2 target_pos;
            ObjectID targetID = ObjectID();
            ObjectID sourceID = ObjectID();

            float f1 = 0.f;
            float f2 = 0.f;
            float f3 = 0.f;
            int i1 = 0;
            int i2 = 0;
            int i3 = 0;
            int i4 = 0;
            int i5 = 0;
            int i6 = 0;
            int i7 = 0;

            std::array<ObjectID,5> ids;
        public:
            glm::vec2 InterpolatePosition(float f);
        };
        struct Entry;
    public:
        UtilityObject() = default;
        UtilityObject(Level& level_, const UtilityObjectDataRef& data, const LiveData& init_data, bool play_sound = true);
        UtilityObject(Level& level_, const UtilityObjectDataRef& data, const glm::vec2& target_pos, const ObjectID& targetID, const LiveData& init_data, FactionObject& src, bool play_sound = true);
        UtilityObject(Level& level_, const UtilityObjectDataRef& data, const glm::vec2& target_pos, const ObjectID& targetID, FactionObject& src, bool play_sound = true);
        UtilityObject(Level& level_, const UtilityObjectDataRef& data, const glm::vec2& target_pos, const ObjectID& targetID = ObjectID(), bool play_sound = true);
        UtilityObject(Level& level_, const UtilityObject::Entry& entry);
        virtual ~UtilityObject();

        //move enabled
        UtilityObject(UtilityObject&&) noexcept = default;
        UtilityObject& operator=(UtilityObject&&) noexcept = default;

        virtual void Render() override;
        virtual bool Update() override;

        UtilityObject::Entry Export() const;
        void RepairIDs(const idMappingType& ids);

        glm::vec2& real_pos() { return real_position; }
        glm::vec2& real_size() { return m_real_size; }

        UtilityObjectDataRef UData() const { return data; }
        LiveData& LD() { return live_data; }

        glm::vec2 SizeScaled() const { return data->size * data->scale; }

        void ChangeType(const UtilityObjectDataRef& new_data, const glm::vec2 target_pos, const ObjectID targetID, bool play_sound = true, bool call_init = true);
        void ChangeType(const UtilityObjectDataRef& new_data, const glm::vec2 target_pos, const ObjectID targetID, FactionObject& src, bool play_sound = true, bool call_init = true);
        void ChangeType(const UtilityObjectDataRef& new_data, const glm::vec2 target_pos, const ObjectID targetID, const LiveData& init_data, FactionObject& src, bool play_sound = true, bool call_init = true);
        void ChangeType(const UtilityObjectDataRef& new_data, const LiveData& init_data, bool play_sound = true, bool call_init = true);
    protected:
        virtual void Inner_DBG_GUI() override;
    private:
        virtual std::ostream& DBG_Print(std::ostream& os) const override;
    private:
        UtilityObjectDataRef data = nullptr;
        LiveData live_data = {};

        glm::vec2 real_position;
        glm::vec2 m_real_size = glm::vec2(1.f);
    };

    struct GameObject::Entry {
        glm::ivec3 id;
        glm::ivec3 num_id;

        glm::ivec2 position;
        int anim_orientation;
        int anim_action;
        float anim_frame;
        bool killed;
    };

    struct FactionObject::Entry : public GameObject::Entry {
        float health;
        int factionIdx;
        int colorIdx;
        int variationIdx;
        bool active;
        bool finalized;
        bool faction_informed;
    };

    struct Unit::Entry : public FactionObject::Entry {
        glm::vec2 move_offset;
        int carry_state;
        float mana;
        bool anim_ended;
        Command::Entry command;
        Action::Entry action;
    };

    struct Building::Entry : public FactionObject::Entry {
        bool constructed;
        int amount_left;
        int real_actionIdx;
        BuildingAction::Entry action;
    };

    struct UtilityObject::Entry : public GameObject::Entry {
        LiveData live_data;
        glm::vec2 real_position;
        glm::vec2 real_size;
    };

    //=============================================

    //Apply damage to given target. Works for both regular & map objects. Returns true if the application went through.
    bool ApplyDamage(Level& level, Unit& src, const ObjectID& targetID, const glm::ivec2& target_pos);
    bool ApplyDamage(Level& level, int basicDamage, int pierceDamage, const ObjectID& targetID, const glm::ivec2& target_pos);
    bool ApplyDamageFlat(Level& level, int damage, const ObjectID& targetID, const glm::ivec2& target_pos);
    std::pair<int,int> ApplyDamage_Splash(Level& level, int basicDamage, int pierceDamage, const glm::ivec2& target_pos, int radius, const ObjectID& exceptID, bool dropoff = true);
    std::pair<int,int> ApplyDamageFlat_Splash(Level& level, int damage, const glm::ivec2& target_pos, int radius, const ObjectID& exceptID, bool dropoff = true);

}//namespace eng
