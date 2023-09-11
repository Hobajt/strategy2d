#include "engine/game/gameobject.h"

#include "engine/game/camera.h"
#include "engine/game/level.h"
#include "engine/game/resources.h"
#include "engine/core/palette.h"

#include "engine/utils/dbg_gui.h"
#include "engine/utils/randomness.h"
#include "engine/core/audio.h"

namespace eng {

    //===== GameObject =====

    int GameObject::idCounter = 0;

    GameObject::GameObject(Level& level_, const GameObjectDataRef& data_, const glm::vec2& position_) : data(data_), id(idCounter++), position(position_), level(&level_) {
        ASSERT_MSG(data != nullptr, "Cannot create GameObject without any data!");
        ASSERT_MSG(level != nullptr, "GameObject needs a reference to the Level it's contained in!");

        animator = Animator(data->animData);
    }

    void GameObject::Render() {
        RenderAt(glm::vec2(position));
    }

    void GameObject::RenderAt(const glm::vec2& pos) {
        RenderAt(pos, data->size);
    }

    void GameObject::RenderAt(const glm::vec2& pos, const glm::vec2& size, float zOffset) {
        ASSERT_MSG(data != nullptr && level != nullptr, "GameObject isn't properly initialized!");

        //TODO: figure out zIdx from the y-coord
        float zIdx = -0.5f;

        Camera& cam = Camera::Get();
        animator.Render(glm::vec3(cam.map2screen(pos), zIdx + zOffset), size * cam.Mult(), ActionIdx(), orientation);
    }

    bool GameObject::Update() {
        ASSERT_MSG(data != nullptr && level != nullptr, "GameObject isn't properly initialized!");
        animator.Update(ActionIdx());
        return killed;
    }

    bool GameObject::CheckPosition(const glm::ivec2& map_coords) {
        return map_coords.x >= position.x && map_coords.x < (position.x + data->size.x) && map_coords.y >= position.y && map_coords.y < (position.y + data->size.y);
    }

    Level* GameObject::lvl() {
        ASSERT_MSG(level != nullptr, "GameObject isn't properly initialized!");
        return level;
    }

    void GameObject::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        if(ImGui::CollapsingHeader(data->name.c_str())) {
            ImGui::PushID(this);
            ImGui::Indent();
            Inner_DBG_GUI();
            if(ImGui::Button("Kill")) Kill();
            ImGui::Unindent();
            ImGui::PopID();
        }
#endif
    }

    std::ostream& operator<<(std::ostream& os, const GameObject& go) {
        return go.DBG_Print(os);
    }

    void GameObject::Inner_DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        ImGui::Text("ID: %d", id);
        ImGui::Text("Action: %d (%d) | Orientation: %d", actionIdx, ActionIdx(), orientation);
        ImGui::Text("Position: [%d, %d]", position.x, position.y);
        ImGui::Text("Frame: %d/%d (%.0f%%)", animator.GetCurrentFrameIdx(), animator.GetCurrentFrameCount(), animator.GetCurrentFrame() * 100.f);
#endif
    }

    std::ostream& GameObject::DBG_Print(std::ostream& os) const {
        os << "GameObject - " << Name() << " (ID=" << ID() << ")";
        return os;
    }

    //===== FactionObject =====

    FactionObject::FactionObject(Level& level_, const FactionObjectDataRef& data_, const FactionControllerRef& faction_, const glm::vec2& position_, int colorIdx_)
        : FactionObject(level_, data_, faction_, 100.f, position_, colorIdx_) {}

    FactionObject::FactionObject(Level& level_, const FactionObjectDataRef& data_, const FactionControllerRef& faction_, float health_p, const glm::vec2& position_, int colorIdx_)
        : GameObject(level_, std::static_pointer_cast<GameObjectData>(data_), position_), faction(faction_), data_f(data_) {

        health = (health_p * 1e-2f) * data_->MaxHealth();

        ASSERT_MSG(faction != nullptr, "FactionObject must be assigned to a Faction!");
        ChangeColor(colorIdx_);
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

    void FactionObject::Kill() {
        if(IsKilled())
            return;
        GameObject::Kill();
        
        UtilityObjectDataRef corpse_data = Resources::LoadUtilityObj("corpse");

        //spawn a corpse utility object
        lvl()->objects.EmplaceUtilityObj(*lvl(), corpse_data, Position(), ObjectID(), *this);
        
        //play the dying sound effect
        static constexpr std::array<const char*, 4> sound_name = { "misc/bldexpl1", "human/hdead", "orc/odead", "ships/shipsink", };
        Audio::Play(SoundEffect::GetPath(sound_name[data_f->deathSoundIdx]), Position());
    }

    bool FactionObject::RangeCheck(GameObject& target) const {
        return RangeCheck(target.MinPos(), target.MaxPos());
    }

    bool FactionObject::RangeCheck(const glm::ivec2& min_pos, const glm::ivec2& max_pos) const {
        float D = get_range(MinPos(), MaxPos(), min_pos, max_pos);
        return (D <= data_f->attack_range);
    }

    void FactionObject::ApplyDirectDamage(const FactionObject& source) {
        //formula from http://classic.battle.net/war2/basic/combat.shtml
        int dmg = std::max(source.BasicDamage() - this->Armor(), 0) + source.PierceDamage();
        dmg = int(dmg * (Random::Uniform() * 0.5f + 0.5f));

        if(dmg < 0) dmg = 0;

        //death condition is handled in object's Update() method
        health -= dmg;

        ENG_LOG_FINE("[DMG] {} dealt {} damage to {} (melee).", source.OID().to_string(), dmg, OID().to_string());
    }

    void FactionObject::ApplyDirectDamage(int src_basicDmg, int src_pierceDmg) {
        //formula from http://classic.battle.net/war2/basic/combat.shtml
        int dmg = std::max(src_basicDmg - this->Armor(), 0) + src_pierceDmg;
        dmg = int(dmg * (Random::Uniform() * 0.5f + 0.5f));

        if(dmg < 0) dmg = 0;

        //death condition is handled in object's Update() method
        health -= dmg;

        ENG_LOG_FINE("[DMG] <<no_source>> dealt {} damage to {} (melee).", dmg, OID().to_string());
    }

    void FactionObject::SetHealth(int value) {
        health = value;
        health = (health <= MaxHealth()) ? health : MaxHealth();
    }

    void FactionObject::AddHealth(int value) {
        health += value;
        health = (health <= MaxHealth()) ? health : MaxHealth();
    }

    void FactionObject::Inner_DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        GameObject::Inner_DBG_GUI();
        ImGui::Text("Health: %d/%d", health, data_f->MaxHealth());
        ImGui::SliderInt("health", &health, 0, data_f->MaxHealth());
        if(ImGui::SliderInt("color", &colorIdx, 0, ColorPalette::FactionColorCount()))
            ChangeColor(colorIdx);
        ImGui::SliderInt("variationIdx", &variationIdx, 0, 5);
#endif
    }

    void FactionObject::InnerIntegrate() {
        //register the object with the map
        lvl()->map.AddObject(NavigationType(), Position(), glm::ivec2(Data()->size), OID(), Data()->objectType == ObjectType::BUILDING);
    }

    bool FactionObject::ValidColor(int idx) {
        return (unsigned int)idx < (unsigned int)ColorPalette::FactionColorCount();
    }

    //===== Unit =====

    Unit::Unit(Level& level_, const UnitDataRef& data_, const FactionControllerRef& faction_, const glm::vec2& position_, bool playReadySound)
        : FactionObject(level_, data_, faction_, position_), data(data_) {
        
        if(playReadySound && data->sound_ready.valid)
            Audio::Play(data->sound_ready.Random());
    }

    Unit::~Unit() {}

    void Unit::Render() {
        RenderAt(glm::vec2(Position()) + move_offset);
    }

    bool Unit::Update() {
        ASSERT_MSG(data != nullptr, "Unit isn't properly initialized!");
        command.Update(*this, *lvl());
        animation_ended = animator.Update(ActionIdx());
        return (Health() <= 0) || IsKilled();
    }

    void Unit::IssueCommand(const Command& cmd) {
        int prevType = command.Type();
        command = cmd;
        action.Signal(*this, ActionSignal::COMMAND_SWITCH, command.Type(), prevType);
        ENG_LOG_FINE("IssueCommand: Unit: {} ({}) ... {}", ID(), Name(), command.to_string());
    }

    int Unit::ActionIdx() const {
        int idx = GameObject::ActionIdx();
        //animation variation switching (for idle & move animations)
        if(idx < ActionType::ACTION && VariationIdx() != 0)
            idx = idx + (UnitAnimationType::DEATH + VariationIdx());
        return idx;
    }

    void Unit::Inner_DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        FactionObject::Inner_DBG_GUI();
        ImGui::Text("move_offset: %s", to_string(move_offset).c_str());
        ImGui::Text("%s", command.to_string().c_str());
        ImGui::Text("Action type: %d", action.logic.type);
#endif
    }

    std::ostream& Unit::DBG_Print(std::ostream& os) const {
        os << "Unit - " << Name() << " (ID=" << ID() << ")";
        return os;
    }

    //===== Building =====

    Building::Building(Level& level_, const BuildingDataRef& data_, const FactionControllerRef& faction_, const glm::vec2& position_, bool constructed)
        : FactionObject(level_, data_, faction_, position_), data(data_) {
        
        if(constructed) {
            //change action to idle for constructed buildings
            action = BuildingAction::Idle(CanAttack());
        }
        else {
            //starting health uptick
            SetHealth(StartingHealth());
        }
    }

    Building::~Building() {}

    void Building::Render() {
        glm::vec2 rendering_size, rendering_offset;
        if(!action.UseConstructionSize()) {
            rendering_offset = glm::vec2(0.f);
            rendering_size = data->size;
        }
        else {
            rendering_offset = (data->size - glm::vec2(2.f)) * 0.5f;
            rendering_size = glm::vec2(2.f);
        }
        
        RenderAt(glm::vec2(Position()) + rendering_offset, rendering_size);
    }

    bool Building::Update() {
        action.Update(*this, *lvl());
        return (Health() <= 0) || IsKilled();
    }

    void Building::IssueAction(const BuildingAction& new_action) {
        action = new_action;
        ENG_LOG_FINE("IssueAction: Building: {} ({}) ... {}", ID(), Name(), action.to_string());
    }

    void Building::CancelAction() {
        ENG_LOG_FINE("CancelAction: Building: {} ({}) ... {}", ID(), Name(), action.to_string());
        action = BuildingAction::Idle(CanAttack());
    }

    int Building::ActionIdx() const {
        int idx = GameObject::ActionIdx();
        //animation variation switching (for idle animation)
        if(idx == BuildingAnimationType::IDLE && VariationIdx() != 0)
            idx = BuildingAnimationType::IDLE2;
        return idx;
    }

    bool Building::IsGatherable(int unitNavType) const {
        return data->gatherable && NavigationType() == unitNavType;
    }

    void Building::Inner_DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        FactionObject::Inner_DBG_GUI();
        ImGui::Text("%s", action.to_string().c_str());
#endif
    }

    void Building::InnerIntegrate() {
        if(!data->traversable)
            FactionObject::InnerIntegrate();
    }

    std::ostream& Building::DBG_Print(std::ostream& os) const {
        os << "Building - " << Name() << " (ID=" << ID() << ")";
        return os;
    }

    //===== UtilityObject =====

    glm::vec2 UtilityObject::LiveData::InterpolatePosition(float f) {
        return glm::vec2(source_pos) + f * glm::vec2(target_pos - source_pos);
    }

    UtilityObject::UtilityObject(Level& level_, const UtilityObjectDataRef& data_, const glm::ivec2& target_pos_, const ObjectID& targetID_, FactionObject& src_)
        : GameObject(level_, data_), data(data_) {
        
        live_data.target_pos = target_pos_;
        live_data.source_pos = src_.Position();
        live_data.targetID = targetID_;
        live_data.sourceID = src_.OID();

        ASSERT_MSG(data->Init != nullptr, "UtilityObject - Init() handler in prefab '{}' isn't setup properly!", data->name);
        data->Init(*this, src_);
    }

    UtilityObject::~UtilityObject() {}

    void UtilityObject::Render() {
        ASSERT_MSG(data->Render != nullptr, "UtilityObject - Render() handler in prefab '{}' isn't setup properly!", data->name);
        data->Render(*this);
    }

    bool UtilityObject::Update() {
        ASSERT_MSG(data != nullptr, "UtilityObject isn't properly initialized!");
        ASSERT_MSG(data->Update != nullptr, "UtilityObject - Update() handler in prefab '{}' isn't setup properly!", data->name);
        bool res = data->Update(*this, *lvl());
        animator.Update(ActionIdx());
        return res || IsKilled();
    }

    void UtilityObject::Inner_DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI

#endif
    }

    std::ostream& UtilityObject::DBG_Print(std::ostream& os) const {
        os << "UtilityObj - " << Name() << " (ID=" << ID() << ")";
        return os;
    }

    //=============================================

    bool ApplyDamage(Level& level, Unit& src, const ObjectID& targetID, const glm::ivec2& target_pos) {
        bool attack_happened = false;
        if(targetID.type != ObjectType::MAP_OBJECT) {
            //target is regular object
            FactionObject* target = nullptr;
            if(level.objects.GetObject(targetID, target)) {
                target->ApplyDirectDamage(src);
                attack_happened = true;
            }
        }
        else {
            //target is attackable map object
            const TileData& tile = level.map(target_pos);
            if(IsWallTile(tile.tileType)) {
                level.map.DamageTile(target_pos, src.BasicDamage(), src.OID());
                attack_happened = true;
            }
        }
        return attack_happened;
    }

    bool ApplyDamage(Level& level, int basicDamage, int pierceDamage, const ObjectID& targetID, const glm::ivec2& target_pos) {
        bool attack_happened = false;
        if(targetID.type != ObjectType::MAP_OBJECT) {
            //target is regular object
            FactionObject* target = nullptr;
            if(level.objects.GetObject(targetID, target)) {
                target->ApplyDirectDamage(basicDamage, pierceDamage);
                attack_happened = true;
            }
        }
        else {
            //target is attackable map object
            const TileData& tile = level.map(target_pos);
            if(IsWallTile(tile.tileType)) {
                level.map.DamageTile(target_pos, basicDamage);
                attack_happened = true;
            }
        }
        return attack_happened;
    }

}//namespace eng
