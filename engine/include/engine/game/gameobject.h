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
        GameObject(const GameObjectDataRef& data, const glm::vec2& position = glm::vec2(0.f));

        virtual void Render();
        virtual void Update(Level& level);

        glm::ivec2 Position() const { return position; }
        int Orientation() const { return orientation; }
        int ActionIdx() const { return actionIdx; }

        glm::ivec2& pos() { return position; }
        int& ori() { return orientation; }
        int& act() { return actionIdx; }

        int ID() const { return id; }
        std::string Name() const { return data->name; }

        void DBG_GUI();
    protected:
        virtual void Inner_DBG_GUI();
        void InnerRender(const glm::vec2& pos);
    protected:
        Animator animator;
    private:
        GameObjectDataRef data = nullptr;

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
        FactionObject(const GameObjectDataRef& data, const FactionControllerRef& faction, const glm::vec2& position = glm::vec2(0.f), int colorIdx = -1);
        virtual ~FactionObject();

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
        Unit(const UnitDataRef& data, const FactionControllerRef& faction, const glm::vec2& position = glm::vec2(0.f));

        virtual void Render() override;
        virtual void Update(Level& level) override;

        //Issue new command (by overriding the existing one).
        void IssueCommand(const Command& cmd);

        int NavigationType() const { return data->navigationType; }

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

    class Building : public FactionObject {};

    //===== UtilityObject =====

    class UtilityObject : public GameObject {};

    // //===== ObjectPool =====

    class ObjectPool {};

}//namespace eng
