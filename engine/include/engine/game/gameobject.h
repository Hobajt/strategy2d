#pragma once

#include "engine/utils/pool.hpp"
#include "engine/core/animator.h"

#include "engine/game/object_data.h"
#include "engine/game/command.h"

namespace eng {

    //===== GameObject =====
    
    class GameObject {
    public:
        GameObject() = default;
        GameObject(const GameObjectDataRef& data, const glm::vec2& position = glm::vec2(0.f));
        virtual ~GameObject();

        GameObject Clone() const noexcept;

        //move enabled
        GameObject(GameObject&&) noexcept = default;
        GameObject& operator=(GameObject&&) noexcept = default;

        virtual void Render();
        virtual void Update() {}
    private:
        //copy private
        GameObject(const GameObject&) noexcept = default;
        GameObject& operator=(const GameObject&) noexcept = default;
    private:
        GameObjectDataRef data = nullptr;
        Animator animator;

        glm::vec2 position = glm::vec2(0.f);
        int orientation = 0;
        int action = 0;

        int id = -1;
        static int idCounter;
    };

    //===== FactionObject =====

    class FactionObject : public GameObject {
    private:
        int colorIdx;
        //TODO: add faction identifier or pointer
    };

    //===== Unit =====

    class Unit : public FactionObject {
        virtual void Update() override;
    private:
        Command command;
    };

    //===== Building =====

    class Building : public FactionObject {};

    //===== UtilityObject =====

    class UtilityObject : public GameObject {};

    // //===== ObjectPool =====

    class ObjectPool {};

}//namespace eng
