#pragma once

#include "engine/utils/pool.hpp"
#include "engine/core/animator.h"

#include "engine/game/object_data.h"
#include "engine/game/command.h"
#include "engine/game/faction.h"

#include <ostream>

namespace eng {

    class Level;

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

        //Return true when requesting object removal.
        virtual bool Update();

        //Check if this object covers given map coordinates.
        bool CheckPosition(const glm::ivec2& map_coords);

        int Orientation() const { return orientation; }
        int ActionIdx() const { return actionIdx; }
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
        void InnerRender(const glm::vec2& pos);
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

        void SetHealth(int health);
        void AddHealth(int value);
    protected:
        virtual void Inner_DBG_GUI() override;
        virtual void InnerIntegrate() override;
    private:
        static bool ValidColor(int idx);
    private:
        FactionObjectDataRef data_f = nullptr;
        FactionControllerRef faction = nullptr;
        int colorIdx;

        int health;
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

        float MoveSpeed() const { return 1.f; }

        glm::vec2& m_offset() { return move_offset; }

        bool AnimationFinished() const { return animation_ended; }

        UnitDataRef UData() const { return data; }
    protected:
        virtual void Inner_DBG_GUI() override;
    private:
        virtual std::ostream& DBG_Print(std::ostream& os) const override;
    private:
        UnitDataRef data = nullptr;

        Command command = Command();
        Action action = Action();

        glm::vec2 move_offset = glm::vec2(0.f);
        bool animation_ended = false;
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

        virtual bool Update() override;

        void IssueAction(const BuildingAction& action);
        void CancelAction();

        //==== Building properties ====

        //Defines the speed of upgrade operation (not relevant when building cannot upgrade).
        int UpgradeTime() const { return data->upgrade_time; }

        //How much health should the building start with when constructing.
        int StartingHealth() const { return MaxHealth() > 100 ? 40 : 10; }

        static int WinterSpritesOffset() { return BuildingAnimationType::COUNT; }
    protected:
        virtual void Inner_DBG_GUI() override;
    private:
        virtual std::ostream& DBG_Print(std::ostream& os) const override;
    private:
        BuildingDataRef data = nullptr;
        BuildingAction action = BuildingAction();
    };

    //===== UtilityObject =====

    class UtilityObject : public GameObject {
    public:
        UtilityObject() = default;
        UtilityObject(const UtilityObjectDataRef& data);
        virtual ~UtilityObject();

        virtual void Render() override;
        virtual bool Update() override;
    protected:
        virtual void Inner_DBG_GUI() override;
    private:
        virtual std::ostream& DBG_Print(std::ostream& os) const override;
    private:
        UtilityObjectDataRef data = nullptr;
    };

}//namespace eng
