#pragma once

#include "engine/utils/pool.hpp"
#include "engine/core/animator.h"

#include "engine/game/object_data.h"
#include "engine/game/command.h"
#include "engine/game/faction.h"

#include <ostream>

namespace eng {

    class Level;

    namespace BuildingType {
        enum { 
            TOWN_HALL, BARRACKS, FARM, LUMBER_MILL, BLACKSMITH, TOWER, 
            SHIPYARD, FOUNDRY, OIL_REFINERY, INVENTOR, STABLES, CHURCH, WIZARD_TOWER, DRAGON_ROOST,
            COUNT
        };
    }

    //===== GameObject =====
    
    class GameObject {
        friend class ObjectPool;
    public:
        GameObject() = default;
        GameObject(Level& level, const GameObjectDataRef& data, const glm::vec2& position = glm::vec2(0.f));

        //copy disabled
        GameObject(const GameObject&) = delete;
        GameObject& operator=(const GameObject&) = delete;

        //move enabled
        GameObject(GameObject&&) noexcept = default;
        GameObject& operator=(GameObject&&) noexcept = default;

        virtual void Render();
        void RenderAt(const glm::vec2& map_pos);
        void RenderAt(const glm::vec2& map_pos, const glm::vec2& size, float zOffset = 0.f);

        //Return true when requesting object removal.
        virtual bool Update();

        virtual void Kill() { killed = true; }

        //Check if this object covers given map coordinates.
        bool CheckPosition(const glm::ivec2& map_coords);

        //Checks nearby tiles & returns the first tile, where a unit can be spawned.
        //Coords are returned through parameter, function itself returns spawnpoint's chessboard distance from the building corner.
        int NearbySpawnCoords(int navType, int preferred_direction, glm::ivec2& out_coords, int max_range = -1);

        int Orientation() const { return orientation; }
        virtual int ActionIdx() const { return actionIdx; }
        glm::ivec2 Position() const { return position; }
        glm::vec2 Positionf() const { return glm::vec2(position); }

        glm::vec2 MinPos() const { return glm::vec2(position); }
        glm::vec2 MaxPos() const { return glm::vec2(position) + data->size - 1.f; }

        glm::ivec2& pos() { return position; }
        int& ori() { return orientation; }
        int& act() { return actionIdx; }
        Level* lvl();

        float AnimationProgress() const { return animator.GetCurrentFrame(); }
        bool AnimKeyframeSignal() const { return animator.KeyframeSignal(); }

        int ID() const { return id; }
        ObjectID OID() const { return oid; }
        std::string Name() const { return data->name; }

        GameObjectDataRef Data() const { return data; }
        int NavigationType() const { return data->navigationType; }

        void DBG_GUI();
        friend std::ostream& operator<<(std::ostream& os, const GameObject& go);
    protected:
        virtual void Inner_DBG_GUI();
        bool IsKilled() const { return killed; }
    private:
        virtual std::ostream& DBG_Print(std::ostream& os) const;
        static int PeekNextID() { return idCounter; }
        void IntegrateIntoLevel(const ObjectID& oid_) { oid = oid_; InnerIntegrate(); }
        virtual void InnerIntegrate() {}
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
        FactionObject() = default;
        FactionObject(Level& level, const FactionObjectDataRef& data, const FactionControllerRef& faction, const glm::vec2& position = glm::vec2(0.f), int colorIdx = -1);
        FactionObject(Level& level, const FactionObjectDataRef& data, const FactionControllerRef& faction, float health_percentage, const glm::vec2& position = glm::vec2(0.f), int colorIdx = -1);
        virtual ~FactionObject();

        //move enabled
        FactionObject(FactionObject&&) noexcept = default;
        FactionObject& operator=(FactionObject&&) noexcept = default;

        void ChangeColor(int colorIdx);

        virtual void Kill() override;

        //Return true if given object is within this object's attack range.
        bool RangeCheck(GameObject& target) const;
        bool RangeCheck(const glm::ivec2& min_pos, const glm::ivec2& max_pos) const;

        bool CanAttack() const { return data_f->CanAttack(); }
        int AttackRange() const { return data_f->attack_range; }
        int BasicDamage() const { return data_f->basic_damage; }
        int PierceDamage() const { return data_f->pierce_damage; }
        float Cooldown() const { return data_f->cooldown; }
        int Armor() const { return data_f->armor; }

        int BuildTime() const { return data_f->build_time; }
        int MaxHealth() const { return data_f->health; }
        int Health() const { return health; }

        //For melee damage application.
        void ApplyDirectDamage(const FactionObject& source);
        void ApplyDirectDamage(int src_basicDmg, int src_pierceDmg);

        void SetHealth(int health);
        void AddHealth(int value);

        virtual bool IsUnit() const { return false; }
        virtual bool IsGatherable(int unitNavType)  const { return false; }
        virtual int DeathAnimIdx() const { return -1; }
        int Race() const { return data_f->race; }

        int VariationIdx() const { return variationIdx; }
        void SetVariationIdx(int idx) { variationIdx = idx; }

        FactionControllerRef Faction() { return faction; }

        bool IsActive() const { return active; }

        //Deactivates the object & removes it from pathfinding
        virtual void WithdrawObject();
        //Reactivates the object & adds it back to pathfinding.
        void ReinsertObject();
        void ReinsertObject(const glm::ivec2& position);
    protected:
        virtual void Inner_DBG_GUI() override;
        virtual void InnerIntegrate() override;
        void RemoveFromMap();

        //Resets object's state, used when reinserting object into the level.
        virtual void ClearState() { variationIdx = 0; }
    private:
        static bool ValidColor(int idx);
    private:
        FactionObjectDataRef data_f = nullptr;
        FactionControllerRef faction = nullptr;
        int colorIdx;

        int health;

        int variationIdx = 0;       //to enable variations for certain animations (increment based on how many overridable animations there are)

        bool active = true;
    };

    //===== Unit =====

    class Unit : public FactionObject {
        friend class Command;
        friend class Action;
    public:
        Unit() = default;
        Unit(Level& level, const UnitDataRef& data, const FactionControllerRef& faction, const glm::vec2& position = glm::vec2(0.f), bool playReadySound = true);
        virtual ~Unit();

        //move enabled
        Unit(Unit&&) noexcept = default;
        Unit& operator=(Unit&&) noexcept = default;

        virtual void Render() override;
        virtual bool Update() override;

        //Issue new command (by overriding the existing one).
        void IssueCommand(const Command& cmd);

        virtual int ActionIdx() const override;

        float MoveSpeed() const { return 1.f; }

        glm::vec2& m_offset() { return move_offset; }

        bool AnimationFinished() const { return animation_ended; }

        UnitDataRef UData() const { return data; }
        virtual bool IsUnit() const override { return true; }
        virtual int DeathAnimIdx() const override { return data->deathAnimIdx; }

        bool IsWorker() const { return data->worker; }
        int CarryStatus() const { return carry_state; }
        void ChangeCarryStatus(int carry_state);
    protected:
        virtual void Inner_DBG_GUI() override;
    private:
        virtual std::ostream& DBG_Print(std::ostream& os) const override;
        virtual void ClearState() override;

        void UpdateVariationIdx();
    private:
        UnitDataRef data = nullptr;

        Command command = Command();
        Action action = Action();

        glm::vec2 move_offset = glm::vec2(0.f);
        bool animation_ended = false;

        int carry_state = WorkerCarryState::NONE;        //worker load indicator
    };

    //===== Building =====

    class Building : public FactionObject {
    public:
        Building() = default;
        Building(Level& level, const BuildingDataRef& data, const FactionControllerRef& faction, const glm::vec2& position = glm::vec2(0.f), bool constructed = false);
        virtual ~Building();

        //move enabled
        Building(Building&&) noexcept = default;
        Building& operator=(Building&&) noexcept = default;

        virtual void Render() override;
        virtual bool Update() override;

        void IssueAction(const BuildingAction& action);
        void CancelAction();

        virtual int ActionIdx() const override;
        virtual bool IsGatherable(int unitNavType) const override;

        virtual void WithdrawObject() override;

        //==== Building properties ====

        //Defines the speed of upgrade operation (not relevant when building cannot upgrade).
        int UpgradeTime() const { return data->upgrade_time; }
        int DropoffMask() const { return data->dropoff_mask; }
        bool CanDropoffResource(int resourceType) const { return (data->dropoff_mask & resourceType) != 0; }

        //How much health should the building start with when constructing.
        int StartingHealth() const { return MaxHealth() > 100 ? 40 : 10; }

        static int WinterSpritesOffset() { return BuildingAnimationType::COUNT; }

        void OnConstructionFinished(bool registerDropoffPoint = true, bool kickoutWorkers = true);
        void OnUpgradeFinished();
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
    };

    //===== UtilityObject =====

    class UtilityObject : public GameObject {
    public:
        struct LiveData {
            glm::ivec2 source_pos;
            glm::ivec2 target_pos;
            ObjectID targetID;
            ObjectID sourceID;

            float f1 = 0.f;
            float f2 = 0.f;
            int i1 = 0;
            int i2 = 0;
            int i3 = 0;
            int i4 = 0;
        public:
            glm::vec2 InterpolatePosition(float f);
        };
    public:
        UtilityObject() = default;
        UtilityObject(Level& level_, const UtilityObjectDataRef& data, const glm::ivec2& target_pos, const ObjectID& targetID, FactionObject& src);
        virtual ~UtilityObject();

        //move enabled
        UtilityObject(UtilityObject&&) noexcept = default;
        UtilityObject& operator=(UtilityObject&&) noexcept = default;

        virtual void Render() override;
        virtual bool Update() override;

        glm::vec2& real_pos() { return real_position; }
        glm::vec2& real_size() { return m_real_size; }

        UtilityObjectDataRef UData() const { return data; }
        LiveData& LD() { return live_data; }
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

    //=============================================

    //Apply damage to given target. Works for both regular & map objects. Returns true if the application went through.
    bool ApplyDamage(Level& level, Unit& src, const ObjectID& targetID, const glm::ivec2& target_pos);
    bool ApplyDamage(Level& level, int basicDamage, int pierceDamage, const ObjectID& targetID, const glm::ivec2& target_pos);

}//namespace eng
