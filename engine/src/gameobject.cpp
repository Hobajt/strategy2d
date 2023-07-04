#include "engine/game/gameobject.h"

namespace eng {

    //===== GameObject =====

    int GameObject::idCounter = 0;

    GameObject::GameObject(const GameObjectDataRef& data_, const glm::vec2& position_) : data(data_), id(idCounter++), position(position_) {
        ENG_LOG_TRACE("[C] GameObject {} ({})", id, data->name);
    }

    GameObject::~GameObject() {
        ENG_LOG_TRACE("[D] GameObject {} ({})", id, data->name);
    }

    GameObject GameObject::Clone() const noexcept {
        GameObject go = GameObject(*this);
        go.id = idCounter++;
        ENG_LOG_TRACE("[C] GameObject {} ({}) (cloned from {})", go.id, go.data->name, id);
        return go;
    }

    void GameObject::Render() {
        // animator.Render();
    }

    //===== Unit =====

    // void Unit::Update() {
    //     command.Update();
    // }

}//namespace eng
