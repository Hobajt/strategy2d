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

        glm::vec2 Position() const { return position; }

        void DBG_GUI();
    protected:
        virtual void Inner_DBG_GUI();
    public:
        //TODO: probably make private again
        int orientation = 0;
        int action = 0;
    protected:
        Animator animator;
    private:
        GameObjectDataRef data = nullptr;

        glm::vec2 position = glm::vec2(0.f);

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
    public:
        Unit() = default;
        Unit(const UnitDataRef& data, const FactionControllerRef& faction, const glm::vec2& position = glm::vec2(0.f));

        virtual void Update(Level& level) override;
    private:
        UnitDataRef data = nullptr;
        Command command;
    };

    //===== Building =====

    class Building : public FactionObject {};

    //===== UtilityObject =====

    class UtilityObject : public GameObject {};

    // //===== ObjectPool =====

    class ObjectPool {};

}//namespace eng
