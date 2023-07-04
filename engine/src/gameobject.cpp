#include "engine/game/gameobject.h"

#include "engine/game/camera.h"
#include "engine/game/level.h"
#include "engine/core/palette.h"

#include "engine/utils/dbg_gui.h"

namespace eng {

    //===== GameObject =====

    int GameObject::idCounter = 0;

    GameObject::GameObject(const GameObjectDataRef& data_, const glm::vec2& position_) : data(data_), id(idCounter++), position(position_) {
        ASSERT_MSG(data != nullptr, "Cannot create GameObject without any data!");

        animator = Animator(data->animData);
    }

    void GameObject::Render() {
        ASSERT_MSG(data != nullptr, "GameObject isn't properly initialized!");

        //TODO: figure out zIdx from the y-coord
        float zIdx = -0.5f;

        Camera& cam = Camera::Get();
        animator.Render(glm::vec3(cam.w2s(position, data->size), zIdx), data->size * cam.Mult(), action, orientation);
    }

    void GameObject::Update(Level& level) {
        ASSERT_MSG(data != nullptr, "GameObject isn't properly initialized!");
        animator.Update(action);
    }

    void GameObject::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        if(ImGui::CollapsingHeader(data->name.c_str())) {
            ImGui::PushID(this);

            Inner_DBG_GUI();
            
            ImGui::PopID();
        }
#endif
    }

    void GameObject::Inner_DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        ImGui::Text("ID: %d", id);
        ImGui::DragInt("action", &action);
        ImGui::SliderInt("orientation", &orientation, 0, 8);
        ImGui::DragFloat2("position", (float*)&position);
#endif
    }

    //===== FactionObject =====

    FactionObject::FactionObject(const GameObjectDataRef& data_, const FactionControllerRef& faction_, const glm::vec2& position_, int colorIdx_)
        : GameObject(data_, position_), faction(faction_) {
        ASSERT_MSG(faction != nullptr, "FactionObject must be assigned to a Faction!");
        ChangeColor(colorIdx_);
    }

    FactionObject::~FactionObject() {
        //TODO: maybe establish an observer relationship with faction object (to track numbers of various types of units, etc.)
    }

    void FactionObject::ChangeColor(int newColorIdx) {
        colorIdx = ValidColor(newColorIdx) ? newColorIdx : faction->GetColorIdx();
        animator.SetPaletteIdx((float)colorIdx);
    }

    void FactionObject::Inner_DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        GameObject::Inner_DBG_GUI();
        ImGui::Separator();
        if(ImGui::SliderInt("color", &colorIdx, 0, ColorPalette::FactionColorCount()))
            ChangeColor(colorIdx);
#endif
    }

    bool FactionObject::ValidColor(int idx) {
        return (unsigned int)idx < (unsigned int)ColorPalette::FactionColorCount();
    }

    //===== Unit =====

    Unit::Unit(const UnitDataRef& data_, const FactionControllerRef& faction_, const glm::vec2& position_)
        : FactionObject(std::static_pointer_cast<GameObjectData>(data_), faction_, position_), data(data_) {}

    void Unit::Update(Level& level) {
        ASSERT_MSG(data != nullptr, "Unit isn't properly initialized!");
        command.Update(*this, level);
        animator.Update(action);
    }

}//namespace eng
