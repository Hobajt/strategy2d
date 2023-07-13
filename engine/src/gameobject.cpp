#include "engine/game/gameobject.h"

#include "engine/game/camera.h"
#include "engine/game/level.h"
#include "engine/core/palette.h"

#include "engine/utils/dbg_gui.h"

namespace eng {

    //===== GameObject =====

    int GameObject::idCounter = 0;

    GameObject::GameObject(Level& level_, const GameObjectDataRef& data_, const glm::vec2& position_) : data(data_), id(idCounter++), position(position_), level(&level_) {
        ASSERT_MSG(data != nullptr, "Cannot create GameObject without any data!");
        ASSERT_MSG(level != nullptr, "GameObject needs a reference to the Level it's contained in!");

        animator = Animator(data->animData);
    }

    void GameObject::Render() {
        InnerRender(glm::vec2(position));
    }

    void GameObject::Update() {
        ASSERT_MSG(data != nullptr && level != nullptr, "GameObject isn't properly initialized!");
        animator.Update(actionIdx);
    }

    Level* GameObject::lvl() {
        ASSERT_MSG(level != nullptr, "GameObject isn't properly initialized!");
        return level;
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
        ImGui::DragInt("action", &actionIdx);
        ImGui::SliderInt("orientation", &orientation, 0, 8);
        ImGui::DragInt2("position", (int*)&position);
#endif
    }

    void GameObject::InnerRender(const glm::vec2& pos) {
        ASSERT_MSG(data != nullptr && level != nullptr, "GameObject isn't properly initialized!");

        //TODO: figure out zIdx from the y-coord
        float zIdx = -0.5f;

        Camera& cam = Camera::Get();
        animator.Render(glm::vec3(cam.w2s(pos, data->size), zIdx), data->size * cam.Mult(), actionIdx, orientation);
    }

    //===== FactionObject =====

    FactionObject::FactionObject(Level& level_, const GameObjectDataRef& data_, const FactionControllerRef& faction_, const glm::vec2& position_, int colorIdx_)
        : GameObject(level_, data_, position_), faction(faction_) {

        ASSERT_MSG(faction != nullptr, "FactionObject must be assigned to a Faction!");
        ChangeColor(colorIdx_);

        //register the object with the map
        lvl()->map.AddObject(NavigationType(), Position(), glm::ivec2(Data()->size), Data()->objectType == ObjectType::BUILDING);
    }

    FactionObject::~FactionObject() {
        if(lvl() != nullptr && Data() != nullptr) {
            lvl()->map.RemoveObject(NavigationType(), Position(), glm::ivec2(Data()->size), Data()->objectType == ObjectType::BUILDING);
        }

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

    Unit::Unit(Level& level_, const UnitDataRef& data_, const FactionControllerRef& faction_, const glm::vec2& position_)
        : FactionObject(level_, std::static_pointer_cast<GameObjectData>(data_), faction_, position_), data(data_) {}

    Unit::~Unit() {}

    void Unit::Render() {
        InnerRender(glm::vec2(Position()) + move_offset);
    }

    void Unit::Update() {
        ASSERT_MSG(data != nullptr, "Unit isn't properly initialized!");
        command.Update(*this, *lvl());
        animator.Update(ActionIdx());
    }

    void Unit::IssueCommand(const Command& cmd) {
        int prevType = command.Type();
        command = cmd;
        action.Signal(*this, ActionSignal::COMMAND_SWITCH, command.Type(), prevType);
        ENG_LOG_FINE("IssueCommand: Unit: {} ({}) ... {}", ID(), Name(), command.to_string());
    }

    void Unit::Inner_DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        FactionObject::Inner_DBG_GUI();
        ImGui::Separator();
        ImGui::Text("move_offset: %s", to_string(move_offset).c_str());
        ImGui::Text("%s", command.to_string().c_str());
        ImGui::Text("Action type: %d", action.logic.type);
#endif
    }

    //===== Building =====

    Building::Building(Level& level_, const BuildingDataRef& data_, const FactionControllerRef& faction_, const glm::vec2& position_, bool constructed)
        : FactionObject(level_, data_, faction_, position_), data(data_) {
        
        if(constructed)
            action = BuildingAction::Idle(CanAttack());
    }

    Building::~Building() {}

    void Building::Update() {
        action.Update(*this, *lvl());
    }

    void Building::IssueAction(const BuildingAction& new_action) {
        action = new_action;
        ENG_LOG_FINE("IssueAction: Building: {} ({}) ... {}", ID(), Name(), action.to_string());
    }

    void Building::CancelAction() {
        ENG_LOG_FINE("CancelAction: Building: {} ({}) ... {}", ID(), Name(), action.to_string());
        action = BuildingAction::Idle(CanAttack());
    }

    void Building::Inner_DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        FactionObject::Inner_DBG_GUI();
        ImGui::Separator();
        ImGui::Text("%s", action.to_string().c_str());
#endif
    }

}//namespace eng
