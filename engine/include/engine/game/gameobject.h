#pragma once

#include "engine/utils/pool.hpp"
#include "engine/core/animator.h"

#include "engine/game/object_data.h"
#include "engine/game/command.h"
#include "engine/game/faction.h"

namespace eng {

    class Level;

    //===== GameObject =====
    
    class GameObject {
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
        virtual void Update();

        glm::ivec2 Position() const { return position; }
        int Orientation() const { return orientation; }
        int ActionIdx() const { return actionIdx; }

        glm::ivec2& pos() { return position; }
        int& ori() { return orientation; }
        int& act() { return actionIdx; }
        Level* lvl();

        int ID() const { return id; }
        std::string Name() const { return data->name; }

        GameObjectDataRef Data() const { return data; }
        int NavigationType() const { return data->navigationType; }

        void DBG_GUI();
    protected:
        virtual void Inner_DBG_GUI();
        void InnerRender(const glm::vec2& pos);
    protected:
        Animator animator;
    private:
        GameObjectDataRef data = nullptr;
        Level* level = nullptr;

        glm::ivec2 position = glm::ivec2(0);
        int orientation = 0;
        int actionIdx = 0;

        int id = -1;
        static int idCounter;
    };

    //===== FactionObject =====

    class FactionObject : public GameObject {
    public:
        FactionObject() = default;
        FactionObject(Level& level, const GameObjectDataRef& data, const FactionControllerRef& faction, const glm::vec2& position = glm::vec2(0.f), int colorIdx = -1);
        virtual ~FactionObject();

        //move enabled
        FactionObject(FactionObject&&) noexcept = default;
        FactionObject& operator=(FactionObject&&) noexcept = default;

        void ChangeColor(int colorIdx);
    protected:
        virtual void Inner_DBG_GUI() override;
    private:
        static bool ValidColor(int idx);
    private:
        FactionControllerRef faction = nullptr;
        int colorIdx;
    };

    //===== Unit =====

    class Unit : public FactionObject {
        friend class Command;
        friend class Action;
    public:
        Unit() = default;
        Unit(Level& level, const UnitDataRef& data, const FactionControllerRef& faction, const glm::vec2& position = glm::vec2(0.f));
        virtual ~Unit();

        //move enabled
        Unit(Unit&&) noexcept = default;
        Unit& operator=(Unit&&) noexcept = default;

        virtual void Render() override;
        virtual void Update() override;

        //Issue new command (by overriding the existing one).
        void IssueCommand(const Command& cmd);

        float MoveSpeed() const { return 1.f; }

        glm::vec2& m_offset() { return move_offset; }
    protected:
        virtual void Inner_DBG_GUI();
    private:
        UnitDataRef data = nullptr;

        Command command = Command();
        Action action = Action();

        glm::vec2 move_offset = glm::vec2(0.f);
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

        virtual void Update() override;

        void IssueAction(const BuildingAction& action);
        void CancelAction();

        //==== Building properties ====

        bool CanAttack() const { return data->attack_damage > 0; } //TODO: return stuff from data
        int MaxHealth() const { return data->health; }

        //Defines the speed of upgrade operation (not relevant when building cannot upgrade).
        int UpgradeTargetHealth() const { return data->upgrade_target; }
    protected:
        virtual void Inner_DBG_GUI();
    private:
        BuildingDataRef data = nullptr;
        BuildingAction action = BuildingAction();
    };

    //===== UtilityObject =====

    class UtilityObject : public GameObject {};

    // //===== ObjectPool =====

    class ObjectPool {};

}//namespace eng
