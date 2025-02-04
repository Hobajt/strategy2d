#include "engine/game/gameobject.h"

#include "engine/game/camera.h"
#include "engine/game/level.h"
#include "engine/game/resources.h"
#include "engine/game/faction.h"
#include "engine/core/palette.h"

#include "engine/utils/dbg_gui.h"
#include "engine/utils/randomness.h"
#include "engine/core/audio.h"
#include "engine/core/input.h"

#include "engine/game/player_controller.h"

namespace eng {

#define UNIT_MANA_REGEN_SPEED 1.f
#define TROLL_REGENERATION_SPEED 0.1f

    static constexpr std::array<const char*, 2> workdone_sound_name = { "peasant/pswrkdon", "orc/owrkdone" };

    //===== GameObject =====

    int GameObject::idCounter = 0;

    GameObject::GameObject(Level& level_, const GameObjectDataRef& data_, const glm::vec2& position_) : data(data_), id(idCounter++), position(position_), level(&level_) {
        ASSERT_MSG(data != nullptr, "Cannot create GameObject without any data!");
        ASSERT_MSG(level != nullptr, "GameObject needs a reference to the Level it's contained in!");

        if(data_->navigationType == NavigationBit::AIR)
            position = make_even(position);

        animator = Animator(data->animData, data->anim_speed);
    }

    GameObject::GameObject(Level& level_, const GameObjectDataRef& data_, const GameObject::Entry& entry) : level(&level_), data(data_) {
        ASSERT_MSG(data != nullptr, "Cannot create GameObject without any data!");
        ASSERT_MSG(level != nullptr, "GameObject needs a reference to the Level it's contained in!");

        position = entry.position;
        orientation = entry.anim_orientation;
        actionIdx = entry.anim_action;
        killed = entry.killed;
        oid = ObjectID(entry.id);
        id = entry.id.z;

        animator = Animator(data->animData, data->anim_speed);
        animator.Restore(entry.anim_action, entry.anim_frame);
    }

    void GameObject::Render() {
        RenderAt(glm::vec2(position));
    }

    void GameObject::RenderAt(const glm::vec2& pos) {
        RenderAt(pos, data->size);
    }

    void GameObject::RenderAt(const glm::vec2& pos, const glm::vec2& size, float zOffset) {
        ASSERT_MSG(data != nullptr && level != nullptr, "GameObject isn't properly initialized!");

        float zIdx = ZIndex(pos, size, zOffset);

        Camera& cam = Camera::Get();
        animator.Render(glm::vec3(cam.map2screen(pos), zIdx), size * cam.Mult(), ActionIdx(), orientation);
    }

    void GameObject::RenderAt(const glm::vec2& base_pos, const glm::vec2& base_size, float scaling, bool centered, float zOffset) {
        ASSERT_MSG(data != nullptr && level != nullptr, "GameObject isn't properly initialized!");

        glm::vec2 size = base_size * scaling;
        glm::vec2 pos = base_pos;
        if(centered) {
            pos = (pos + 0.5f) - size * 0.5f;
        }

        float zIdx = ZIndex(pos, size, zOffset);

        Camera& cam = Camera::Get();
        animator.Render(glm::vec3(cam.map2screen(pos), zIdx), size * cam.Mult(), ActionIdx(), orientation);
    }

    void GameObject::RenderAt(const glm::vec2& pos, const glm::vec2& size, int action, float frame, float zOffset) {
        ASSERT_MSG(data != nullptr && level != nullptr, "GameObject isn't properly initialized!");
        float zIdx = ZIndex(pos, size, zOffset);

        Camera& cam = Camera::Get();
        animator.Render(glm::vec3(cam.map2screen(pos), zIdx), size * cam.Mult(), action, orientation, frame);
    }

    bool GameObject::Update() {
        ASSERT_MSG(data != nullptr && level != nullptr, "GameObject isn't properly initialized!");
        animator.Update(ActionIdx());
        return killed;
    }

    void GameObject::Export(GameObject::Entry& entry) const {
        entry.id = glm::ivec3(oid.type, oid.idx, oid.id);
        entry.num_id = NumID();

        entry.position = position;
        entry.anim_action = actionIdx;
        entry.anim_orientation = orientation;
        entry.anim_frame = animator.GetCurrentFrame();
        entry.killed = killed;
    }

    void GameObject::LevelPtrUpdate(Level& lvl) {
        level = &lvl;
    }

    bool GameObject::CheckPosition(const glm::ivec2& map_coords) {
        return map_coords.x >= position.x && map_coords.x < (position.x + data->size.x) && map_coords.y >= position.y && map_coords.y < (position.y + data->size.y);
    }

    int GameObject::NearbySpawnCoords(int nav_type, int preferred_direction, glm::ivec2& out_coords, int max_range) {
        return lvl()->map.NearbySpawnCoords(position, data->size, preferred_direction, nav_type, out_coords, max_range);
    }

    std::pair<glm::vec2, glm::vec2> GameObject::AABB() const {
        glm::vec2 pos = RenderPosition();
        glm::vec2 size = RenderSize();
        return { glm::vec2(pos.x, pos.y), glm::vec2(pos.x+size.x, pos.y+size.y) };
    }

    glm::vec4 GameObject::AABB_v4() const {
        glm::vec2 pos = RenderPosition();
        glm::vec2 size = RenderSize();
        return glm::vec4(pos.x, pos.y, pos.x+size.x, pos.y+size.y);
    }

    Level* GameObject::lvl() {
        ASSERT_MSG(level != nullptr, "GameObject isn't properly initialized!");
        return level;
    }

    const Level* GameObject::lvl() const {
        ASSERT_MSG(level != nullptr, "GameObject isn't properly initialized!");
        return level;
    }

    void GameObject::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        ImGui::PushID(this);
        if(ImGui::CollapsingHeader(data->name.c_str())) {
            ImGui::Indent();
            Inner_DBG_GUI();
            if(ImGui::Button("Kill")) Kill();
            ImGui::Unindent();
        }
        ImGui::PopID();
#endif
    }

    std::ostream& operator<<(std::ostream& os, const GameObject& go) {
        return go.DBG_Print(os);
    }

    void GameObject::Inner_DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        ImGui::Text("ID: %d", id);
        ImGui::Text("AnimationIdx: %d (%d) | Orientation: %d", actionIdx, ActionIdx(), orientation);
        ImGui::Text("Position: [%d, %d]", position.x, position.y);
        ImGui::Text("Frame: %d/%d (%.0f%%)", animator.GetCurrentFrameIdx(), animator.GetCurrentFrameCount(), animator.GetCurrentFrame() * 100.f);
#endif
    }

    std::ostream& GameObject::DBG_Print(std::ostream& os) const {
        os << "GameObject - " << Name() << " (ID=" << ID() << ")";
        return os;
    }

    float GameObject::ZIndex(const glm::vec2& pos, const glm::vec2& size, float zOffset) {
        return -0.5f + 1e-4f * (pos.y + size.y * 0.5f) + zOffset;
    }

    //===== FactionObject =====

    FactionObject::FactionObject(Level& level_, const FactionObjectDataRef& data_, const FactionControllerRef& faction_, const glm::vec2& position_, bool is_finished_)
        : FactionObject(level_, data_, faction_, 100.f, position_, -1, is_finished_) {}
    
    FactionObject::FactionObject(Level& level_, const FactionObjectDataRef& data_, const FactionControllerRef& faction_, const glm::vec2& position_, int colorIdx_, bool is_finished_)
        : FactionObject(level_, data_, faction_, 100.f, position_, colorIdx_, is_finished_) {}

    FactionObject::FactionObject(Level& level_, const FactionObjectDataRef& data_, const FactionControllerRef& faction_, float health_p, const glm::vec2& position_, int colorIdx_, bool is_finished_)
        : GameObject(level_, std::static_pointer_cast<GameObjectData>(data_), position_), faction(faction_), data_f(data_) {

        health = (health_p * 1e-2f) * data_->MaxHealth();

        ASSERT_MSG((faction != nullptr) && (faction->ID() >= 0), "FactionObject must be assigned to a valid faction!");
        factionIdx = faction->ID();
        ChangeColor(colorIdx_);
        if(is_finished_) {
            faction->ObjectAdded(data_f);
        }
    }

    FactionObject::FactionObject(Level& level_, const FactionObjectDataRef& data_, const FactionObject::Entry& entry, bool is_finished_) 
        : GameObject(level_, std::static_pointer_cast<GameObjectData>(data_), entry), data_f(data_) {

        faction = level_.factions[entry.factionIdx];
        ASSERT_MSG((faction != nullptr) && (faction->ID() >= 0), "FactionObject must be assigned to a valid faction!");
        
        health              = entry.health;
        factionIdx          = entry.factionIdx;
        colorIdx            = entry.colorIdx;
        variationIdx        = entry.variationIdx;
        active              = entry.active;
        finalized           = entry.finalized;
        faction_informed    = entry.faction_informed;
        ChangeColor(entry.colorIdx);
        if(is_finished_) {
            faction->ObjectAdded(data_f);
        }
    }

    FactionObject::~FactionObject() {
        RemoveFromMap();
        if(lvl() != nullptr && Data() != nullptr) {
            lvl()->objects.KillObjectsInside(OID());
            ENG_LOG_TRACE("Removing {}", OID());
            if(finalized) {
                faction->ObjectRemoved(data_f);
            }
        }
    }

    void FactionObject::ChangeColor(int newColorIdx) {
        colorIdx = ValidColor(newColorIdx) ? newColorIdx : faction->GetColorIdx(Data()->num_id);
        animator.SetPaletteIdx((float)colorIdx);
    }

    void FactionObject::ResetColor() {
        colorIdx = faction->GetColorIdx(Data()->num_id);
        animator.SetPaletteIdx((float)colorIdx);
    }

    void FactionObject::ChangeFaction(const FactionControllerRef& new_faction, bool keep_color) {
        ASSERT_MSG((new_faction != nullptr) && (new_faction->ID() >= 0), "FactionObject must be assigned to a valid faction!");
        faction = new_faction;
        factionIdx = faction->ID();
        if(!keep_color)
            ChangeColor(faction->GetColorIdx(NumID()));
    }

    void FactionObject::Export(FactionObject::Entry& entry) const {
        GameObject::Export(entry);
        entry.health = health;
        entry.factionIdx = factionIdx;
        entry.colorIdx = colorIdx;
        entry.variationIdx = variationIdx;
        entry.active = active;
        entry.finalized = finalized;
        entry.faction_informed = faction_informed;
    }

    bool FactionObject::Kill(bool silent) {
        if(IsKilled())
            return true;
        GameObject::Kill(silent);
        
        if(!silent) {
            // UtilityObjectDataRef corpse_data = (NumID()[1] != UnitType::BALLISTA) ? Resources::LoadUtilityObj("corpse") : Resources::LoadUtilityObj("explosion_long_s2");
            // glm::vec2 pos = (NumID()[1] != UnitType::BALLISTA) ? RenderPosition() : RenderPositionCentered();
            UtilityObjectDataRef corpse_data = Resources::LoadUtilityObj("corpse");
            glm::vec2 pos = RenderPosition();
            //spawn a corpse utility object
            lvl()->objects.QueueUtilityObject(*lvl(), corpse_data, pos, ObjectID(), *this);
            
            //play the dying sound effect
            static constexpr std::array<const char*, 7> sound_name = { "misc/bldexpl1", "human/hdead", "orc/odead", "ships/shipsink", "misc/explode", "misc/firehit", "misc/Skeleton Death" };
            Audio::Play(SoundEffect::GetPath(sound_name[data_f->deathSoundIdx]), Position());
        }

        return true;
    }

    bool FactionObject::RangeCheck(GameObject& target) const {
        return RangeCheck(target.MinPos(), target.MaxPos());
    }

    bool FactionObject::RangeCheck(const glm::ivec2& min_pos, const glm::ivec2& max_pos) const {
        float D = get_range(MinPos(), MaxPos(), min_pos, max_pos);
        return (D <= data_f->attack_range);
    }

    int FactionObject::GetRange(GameObject& target) const {
        return GetRange(target.MinPos(), target.MaxPos());
    }

    int FactionObject::GetRange(const glm::ivec2& min_pos, const glm::ivec2& max_pos) const {
        return get_range(MinPos(), MaxPos(), min_pos, max_pos);
    }

    glm::vec2 FactionObject::PositionCentered() const {
        return glm::vec2(Position()) + (data_f->size * 0.5f);
    }

    int FactionObject::ApplyDirectDamage(const FactionObject& source) {
        if(IsInvulnerable()) {
            ENG_LOG_FINE("[DMG] attempting to damage invulnerable object ({}).", OID().to_string());
            return 0;
        }

        //formula from http://classic.battle.net/war2/basic/combat.shtml
        int dmg = std::max(source.BasicDamage() - this->Armor(), 0) + source.PierceDamage();
        dmg = int(dmg * (Random::Uniform() * 0.5f + 0.5f));

        if(dmg < 0) dmg = 0;

        //death condition is handled in object's Update() method
        health -= dmg;

        ENG_LOG_FINE("[DMG] {} dealt {} damage to {} (melee).", source.OID().to_string(), dmg, OID().to_string());
        return dmg;
    }

    int FactionObject::ApplyDirectDamage(int src_basicDmg, int src_pierceDmg) {
        if(IsInvulnerable()) {
            ENG_LOG_FINE("[DMG] attempting to damage invulnerable object ({}).", OID().to_string());
            return 0;
        }

        //formula from http://classic.battle.net/war2/basic/combat.shtml
        int dmg = std::max(src_basicDmg - this->Armor(), 0) + src_pierceDmg;
        dmg = int(dmg * (Random::Uniform() * 0.5f + 0.5f));

        if(dmg < 0) dmg = 0;

        //death condition is handled in object's Update() method
        health -= dmg;

        ENG_LOG_FINE("[DMG] <<no_source>> dealt {} damage to {} (melee).", dmg, OID().to_string());
        return dmg;
    }

    int FactionObject::ApplyDamageFlat(int dmg) {
        if(IsInvulnerable()) {
            ENG_LOG_FINE("[DMG] attempting to damage invulnerable object ({}).", OID().to_string());
            return 0;
        }

        if(dmg < 0) dmg = 0;

        //death condition is handled in object's Update() method
        health -= dmg;

        ENG_LOG_FINE("[DMG] <<no_source>> dealt {} damage to {} (melee).", dmg, OID().to_string());
        return dmg;
    }

    bool FactionObject::CanAttackTarget(int targetNavType) const {
        return (targetNavType != NavigationBit::AIR || !data_f->ground_attack_only);
    }

    void FactionObject::SetHealth(int value) {
        health = value;
        health = (health <= MaxHealth()) ? health : MaxHealth();
    }

    void FactionObject::AddHealth(float value) {
        health += value;
        health = (health <= MaxHealth()) ? health : MaxHealth();
    }

    Techtree& FactionObject::Tech() { return faction->Tech(); }
    const Techtree& FactionObject::Tech() const { return faction->Tech(); }

    void FactionObject::WithdrawObject(bool inform_faction) {
        if(active) {
            RemoveFromMap();
            active = false;

            if(finalized && inform_faction) {
                faction->ObjectRemoved(data_f);
            }
            faction_informed = inform_faction;
        }
    }

    void FactionObject::ReinsertObject() {
        ReinsertObject(Position());
    }

    void FactionObject::ReinsertObject(const glm::ivec2& position) {
        if(!active) {
            pos() = position;
            ClearState();
            InnerIntegrate();
            active = true;
            if(finalized && faction_informed) {
                faction->ObjectAdded(data_f);
            }
        }
        faction_informed = true;
    }

    void FactionObject::MoveTo(const glm::ivec2& position) {
        WithdrawObject(true);
        ReinsertObject(position);
    }

    void FactionObject::Inner_DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        GameObject::Inner_DBG_GUI();
        ImGui::Text("Active: %s", active ? "True" : "False");
        const char* btn_labels[2] { "Reinsert object", "Withdraw object" };
        if(ImGui::Button(btn_labels[(int)active])) {
            if(active)
                WithdrawObject(true);
            else
                ReinsertObject();
        }
        ImGui::Separator();

        ImGui::Text("Health: %d/%d", int(health), data_f->MaxHealth());
        ImGui::SliderFloat("health", &health, 0, data_f->MaxHealth());
        if(ImGui::SliderInt("color", &colorIdx, 0, ColorPalette::FactionColorCount()))
            ChangeColor(colorIdx);
        ImGui::SliderInt("variationIdx", &variationIdx, 0, 5);
#endif
    }

    void FactionObject::InnerIntegrate() {
        //register the object with the map
        lvl()->map.AddObject(NavigationType(), Position(), glm::ivec2(Data()->size), OID(), FactionIdx(), colorIdx, NumID(), Data()->objectType == ObjectType::BUILDING, VisionRange());
    }

    void FactionObject::RemoveFromMap() {
        if(active) {
            if(lvl() != nullptr && Data() != nullptr) {
                //remove from pathfinding (if it's a part of it to begin with)
                if(data_f->IntegrateInMap())
                    lvl()->map.RemoveObject(NavigationType(), Position(), glm::ivec2(Data()->size), Data()->objectType == ObjectType::BUILDING, FactionIdx(), VisionRange());
                else
                    lvl()->map.RemoveTraversableObject(OID());

                if(data_f->IsNotableObject())
                    lvl()->map.RemoveTraversableObject(OID());
            }
            active = false;
        }
    }

    void FactionObject::UpdateDataPointer(const FactionObjectDataRef& data_) {
        data_f = data_;
        GameObject::UpdateDataPointer(data_);
    }

    bool FactionObject::ValidColor(int idx) {
        return (unsigned int)idx < (unsigned int)ColorPalette::FactionColorCount();
    }

    //===== UnitEffects =====

    bool& UnitEffects::operator[](int idx) { return flags.at(idx); }

    const bool& UnitEffects::operator[](int idx) const { return flags.at(idx); }

    std::string UnitEffects::to_string() const {
        char buf[128];
        snprintf(buf, sizeof(buf), "[%d,%d,%d,%d,%d]", int(flags[0]), int(flags[1]), int(flags[2]), int(flags[3]), int(flags[4]));
        return std::string(buf);
    }

    std::ostream& operator<<(std::ostream& os, const UnitEffects& eff) {
        os << eff.to_string();
        return os;
    }

    //===== Unit =====

    Unit::Unit(Level& level_, const UnitDataRef& data_, const FactionControllerRef& faction_, const glm::vec2& position_, bool playReadySound)
        : FactionObject(level_, data_, faction_, position_), data(data_) {
        
        if(data_->navigationType != NavigationBit::GROUND) {
            if(int(position_.x) % 2 != 0 || int(position_.y) % 2 != 0) {
                ENG_LOG_ERROR("Unit - cannot spawn naval/air unit on odd coordinates (pathfinding only works on even coords for these).");
                throw std::runtime_error("");
            }
        }
        
        if(playReadySound && data->sound_ready.valid)
            Audio::Play(data->sound_ready.Random());
    }

    Unit::Unit(Level& level_, const Unit::Entry& entry)
        : FactionObject(level_, Resources::LoadUnit(entry.num_id[1], entry.num_id[2]), entry), data(Resources::LoadUnit(entry.num_id[1], entry.num_id[2])) {
        
        if(data->navigationType != NavigationBit::GROUND) {
            if(int(Position().x) % 2 != 0 || int(Position().y) % 2 != 0) {
                ENG_LOG_ERROR("Unit - cannot spawn naval/air unit on odd coordinates (pathfinding only works on even coords for these).");
                throw std::runtime_error("");
            }
        }

        move_offset = entry.move_offset;
        carry_state = entry.carry_state;
        mana = entry.mana;
        animation_ended = entry.anim_ended;
        command = Command(entry.command);
        action = Action(entry.action);
    }

    Unit::~Unit() {}

    void Unit::Render() {
        if(!IsActive() || !lvl()->map.IsTileVisible(Position()) || lvl()->map.IsUntouchable(Position(), NavigationType() == NavigationBit::AIR))
            return;
        bool render_centered = (NavigationType() == NavigationBit::GROUND);
        float zOffset        = (NavigationType() == NavigationBit::AIR) ? (-1e-3f) : 0.f;
        RenderAt(glm::vec2(Position()) + move_offset, data->size, data->scale, render_centered, zOffset);
    }

    bool Unit::Update() {
        ASSERT_MSG(data != nullptr, "Unit isn't properly initialized!");
        if(!IsActive())
            return (Health() <= 0) || IsKilled();
        command.Update(*this, *lvl());
        UpdateVariationIdx();
        ManaIncrement();
        TrollRegeneration();
        animation_ended = animator.Update(ActionIdx());
        return (Health() <= 0) || IsKilled();
    }

    Unit::Entry Unit::Export() const {
        Unit::Entry entry = {};

        FactionObject::Export(entry);

        entry.move_offset = move_offset;
        entry.carry_state = carry_state;
        entry.mana = mana;
        entry.anim_ended = animation_ended;

        entry.command = command.Export();
        entry.action = action.Export();

        return entry;
    }

    void Unit::RepairIDs(const idMappingType& ids) {
        if(ids.count(command.TargetID())) command.UpdateTargetID(ids.at(command.TargetID()));
        if(ids.count(action.data.target)) action.data.target = ids.at(action.data.target);
    }

    bool Unit::PassiveMindset() const {
        return 
            data->num_id[1] == UnitType::PEASANT || 
            data->num_id[1] == UnitType::MAGE || 
            data->num_id[1] == UnitType::TRANSPORT || 
            data->num_id[1] == UnitType::TANKER || 
            data->num_id[1] == UnitType::DEMOSQUAD || 
            data->num_id[1] == UnitType::ROFLCOPTER || 
            (data->num_id[1] >= UnitType::SEAL && data->num_id[1] <= UnitType::EYE);
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

    glm::vec2 Unit::PositionCentered() const {
        //apply scale only for non grounded units, since those are rendered centered
        float scale_modifier = (NavigationType() == NavigationBit::GROUND) ? 1.f : data->scale;
        return glm::vec2(Position()) + (data->size * scale_modifier * 0.5f);
    }

    bool Unit::IsInvulnerable() const {
        return FactionObject::IsInvulnerable() || effects[UnitEffectType::UNHOLY_ARMOR];
    }

    int Unit::BasicDamage() const {
        return FactionObject::BasicDamage() * (1 + effects[UnitEffectType::BLOODLUST]);
    }

    int Unit::PierceDamage() const { 
        return FactionObject::PierceDamage() * (1 + effects[UnitEffectType::BLOODLUST]);
    }

    bool Unit::UnitUpgrade(int factionID, int old_type, int new_type, bool isOrcUnit) {
        if(factionID != FactionIdx() || old_type != NumID()[1] || isOrcUnit != IsOrc()) {
            return false;
        }

        if(((unsigned)new_type) >= ((unsigned)UnitType::COUNT)) {
            ENG_LOG_ERROR("Unit::UnitUpgrade - invalid unit type provided ({}).", new_type);
            throw std::out_of_range("");
        }
        UnitDataRef new_data = Resources::LoadUnit(new_type, IsOrc());
        if(new_data == nullptr) {
            ENG_LOG_ERROR("Unit::UnitUpgrade - invalid data type for the upgrade.");
            throw std::runtime_error("");
        }

        //health update
        int new_health = HealthPercentage() * new_data->MaxHealth();

        //signal to faction controller, so that it updates its stats
        Faction()->ObjectUpgraded(data, new_data);

        //update vision range
        lvl()->map.VisionRangeUpdate(Position(), data->size, data->vision_range, new_data->vision_range);

        //data pointer & animator updates
        data = new_data;
        UpdateDataPointer(new_data);
        animator = Animator(new_data->animData);

        SetHealth(new_health);

        return true;
    }

    ObjectID Unit::Transform(const UnitDataRef& new_data, const FactionControllerRef& new_faction, bool preserve_health) {
        ENG_LOG_FINE("Unit::Transform - {} -> {} (faction: {} -> {})", data->name, new_data->name, FactionIdx(), new_faction->ID());

        //health update
        int new_health = preserve_health ? (HealthPercentage() * new_data->MaxHealth()) : new_data->MaxHealth();

        //withdraw object from the map, so that I don't have to modify values in the map struct (will update on reinsert)
        WithdrawObject(true);

        // lvl()->map.VisibilityDecrement(Position(), data->size, data->vision_range, FactionIdx());
        // lvl()->map.VisibilityIncrement(Position(), new_data->size, new_data->vision_range, new_faction->ID());

        ChangeFaction(new_faction);
        
        //data pointer & animator updates
        data = new_data;
        UpdateDataPointer(new_data);
        animator = Animator(new_data->animData);

        UnsetKilledFlag();
        Finalized(true);

        SetHealth(new_health);

        //update unit's ID - in order to cancel any commands issued on the unit pre-transformation
        lvl()->objects.UpdateUnitID(OID());

        ReinsertObject();

        return OID();
    }

    void Unit::DecreaseMana(int value) {
        ENG_LOG_FINER("Unit::DecreaseMana - {}-{} ({})", mana, value, *this);
        mana = std::max(0.f, mana - value);
    }

    float Unit::MoveSpeed_Real() const {
        return data->speed * 0.1f * SpeedBuffValue();
    }

    float Unit::SpeedBuffValue() const {
        return (1 + 0.5f * (int(effects[UnitEffectType::HASTE]) - int(effects[UnitEffectType::SLOW])));
    }

    glm::vec2 Unit::RenderPosition() const {
        glm::vec2 pos = glm::vec2(Position()) + move_offset;
        if(NavigationType() == NavigationBit::GROUND) {
            pos = (pos + 0.5f) - RenderSize() * 0.5f;
        }
        return pos;
    }

    glm::vec2 Unit::RenderPositionCentered() const {
        return PositionCentered() + move_offset;
    }

    bool Unit::IsUndead() const {
        return (data->num_id[2] == 1 && data->num_id[1] == UnitType::MAGE) || data->num_id[1] == UnitType::SKELETON;
    }

    glm::vec2 Unit::RenderSize() const {
        return glm::vec2(data->size) * data->scale;
    }

    bool Unit::DetectablyInvisible() const {
        return (NumID()[1] == UnitType::SUBMARINE) && !effects.flags[UnitEffectType::INVISIBILITY];
    }

    void Unit::RandomizeIdleRotation() {
        command.RandomizeIdleRotation();
    }

    void Unit::ChangeCarryStatus(int new_state) {
        if(IsWorker()) {
            ASSERT_MSG(new_state == WorkerCarryState::NONE || carry_state == WorkerCarryState::NONE, "Unit::ChangeCarryStatus - resource override, worker is already carrying something.");
            carry_state = new_state;
        }
        else {
            ENG_LOG_WARN("Attempting to set carry state on a non-worker unit ({})", *this);
            carry_state = WorkerCarryState::NONE;
        }
    }

    void Unit::Transport_UnitAdded() {
        carry_state++;
        if(carry_state > TRANSPORT_MAX_LOAD) {
            ENG_LOG_WARN("Unit::Transport_UnitAdded - transport ship overloaded ({}/{})", carry_state, TRANSPORT_MAX_LOAD);
        }
    }
    
    void Unit::Transport_UnitRemoved() {
        carry_state--;
        if(carry_state < 0) {
            ENG_LOG_WARN("Unit::Transport_UnitRemoved - unit removal from empty transport!");
            carry_state = 0;
        }
    }

    void Unit::Transport_DockingRequest(const glm::ivec2& unitPosition) {
        ASSERT_MSG(data->transport, "Unit::Transport_DockingLocation - can only invoke on transport ships!");

        //move command issued in this frame or the ship is moving to dock
        if(command.Type() == CommandType::MOVE && (command.Flag() != 0 || lvl()->map(command.TargetPos()).IsCoastTile())) {
            if(command.Flag() == 0) {
                ENG_LOG_TRACE("Unit::Transport_DockingRequest - docking refused as ship is already handling different docking request");
            }
            return;
        }

        glm::ivec2 docking_pos = lvl()->map.Pathfinding_DockingLocation(Position(), unitPosition);
        IssueCommand(Command::Move(docking_pos));
        ENG_LOG_TRACE("Unit::Transport_DockingRequest - docking issued at ({}, {})", docking_pos.x, docking_pos.y);
    }

    bool Unit::Transport_GoingDocking() const {
        return command.Type() == CommandType::MOVE && lvl()->map(command.TargetPos()).IsCoastTile();
    }

    glm::ivec2 Unit::Transport_GetDockingLocation() const {
        ASSERT_MSG(Transport_GoingDocking(), "Docking location is valid only if the transport is going docking.");
        return command.TargetPos();
    }

    bool Unit::InMotion() const {
        return command.Type() == CommandType::MOVE;
    }

    void Unit::SetInvisible(bool state) {
        effects.flags[UnitEffectType::INVISIBILITY] = state;
        lvl()->map.SetObjectInvisibility(Position(), state, NavigationType() == NavigationBit::AIR);
    }

    void Unit::SetEffectFlag(int idx, bool state) {
        ASSERT_MSG((((unsigned int)idx) < UnitEffectType::COUNT), "Unit::SetEffectFlag - invalid effect index ({}, max {})", idx, UnitEffectType::COUNT);
        effects.flags[idx] = state;
        if(idx == UnitEffectType::INVISIBILITY) {
            lvl()->map.SetObjectInvisibility(Position(), state, NavigationType() == NavigationBit::AIR);
        }

        animator.SetAnimSpeed(SpeedBuffValue());
    }

    bool Unit::GetEffectFlag(int idx) {
        ASSERT_MSG((((unsigned int)idx) < UnitEffectType::COUNT), "Unit::SetEffectFlag - invalid effect index ({}, max {})", idx, UnitEffectType::COUNT);
        return effects[idx];
    }

    bool Unit::IsMinion(const glm::ivec3& num_id) {
        return num_id[0] == ObjectType::UNIT && (num_id[1] == UnitType::SKELETON || num_id[1] == UnitType::EYE);
    }

    bool Unit::IsRessurectable(const glm::ivec3& num_id, int navType) {
        return navType == NavigationBit::GROUND && num_id[0] == ObjectType::UNIT && !(num_id[1] == UnitType::SKELETON || num_id[1] == UnitType::EYE || num_id[1] == UnitType::BALLISTA || num_id[1] == UnitType::DEMOSQUAD);
    }

    void Unit::Inner_DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        FactionObject::Inner_DBG_GUI();
        ImGui::Text("move_offset: %s", to_string(move_offset).c_str());
        ImGui::Text("%s", command.to_string().c_str());
        ImGui::Text("Action type: %d", action.logic.type);
        ImGui::Text("Carry state: %d", carry_state);
        ImGui::Text("Effects: %s", effects.to_string().c_str());
#endif
    }

    std::ostream& Unit::DBG_Print(std::ostream& os) const {
        os << "Unit - " << Name() << " (ID=" << ID() << ")";
        return os;
    }

    void Unit::ClearState() {
        move_offset = glm::vec2(0.f);
        command = Command();
        action = Action();
        carry_state = WorkerCarryState::NONE;
        animation_ended = false;
    }

    void Unit::UpdateVariationIdx() {
        SetVariationIdx((carry_state != WorkerCarryState::NONE) ? (1+int(carry_state == WorkerCarryState::WOOD)*2) : 0);
    }

    void Unit::ManaIncrement() {
        if(IsCaster() || NumID()[1] == UnitType::KNIGHT) {
            mana += Input::Get().deltaTime * UNIT_MANA_REGEN_SPEED;
            if(mana > 255.f)
                mana = 255.f;
        }
    }

    void Unit::TrollRegeneration() {
        if(IsOrc() && NumID()[1] == UnitType::RANGER && Tech().GetResearch(ResearchType::LM_UNIQUE, true)) {
            AddHealth(Input::Get().deltaTime * TROLL_REGENERATION_SPEED);
        }
    }

    void Unit::PanicMovement(bool hostile_attack, bool source_unreachable) {
        if(command.Type() == CommandType::IDLE && (source_unreachable || (hostile_attack && PassiveMindset()))) {
            glm::ivec2 target_pos = lvl()->map.GetPanicMovementLocation(*this);
            ENG_LOG_TRACE("Unit::PanicMovement - {} to {}", *this, target_pos);
            IssueCommand(Command::Move(target_pos));
        }
    }

    //===== Building =====

    Building::Building(Level& level_, const BuildingDataRef& data_, const FactionControllerRef& faction_, const glm::vec2& position_, bool constructed)
        : FactionObject(level_, data_, faction_, position_, false), data(data_) {

        if(data_->navigationType == NavigationBit::WATER && data_->num_id[1] == BuildingType::OIL_PLATFORM) {
            if(int(position_.x) % 2 != 0 || int(position_.y) % 2 != 0) {
                ENG_LOG_ERROR("Building - cannot spawn oil platform on odd coordinates (pathfinding only works on even coords for these).");
                throw std::runtime_error("");
            }
        }
        
        if(constructed) {
            //can't register dropoff point from constructor - ID isn't setup properly yet (called from IntegrateIntoLevel() instead)
            OnConstructionFinished(false, false);
        }
        else {
            //starting health uptick
            SetHealth(StartingHealth());
        }
    }

    Building::Building(Level& level_, const Building::Entry& entry)
        : FactionObject(level_, Resources::LoadBuilding(entry.num_id[1], entry.num_id[2]), entry, entry.constructed), data(Resources::LoadBuilding(entry.num_id[1], entry.num_id[2])) {
        
        if(data->navigationType == NavigationBit::WATER && data->num_id[1] == BuildingType::OIL_PLATFORM) {
            if(int(Position().x) % 2 != 0 || int(Position().y) % 2 != 0) {
                ENG_LOG_ERROR("Building - cannot spawn oil platform on odd coordinates (pathfinding only works on even coords for these).");
                throw std::runtime_error("");
            }
        }

        constructed = entry.constructed;
        amount_left = entry.amount_left;
        real_actionIdx = entry.real_actionIdx;
        action = BuildingAction(entry.action);
    }

    Building::~Building() {
        Finalized(constructed);
        if(lvl() != nullptr && Data() != nullptr) {
            if(constructed) {
                UnregisterDropoffPoint();
            }
        }
    }

    void Building::Render() {
        if(!IsActive())
            return;
        glm::vec2 rendering_size, rendering_offset;
        if(!action.UseConstructionSize()) {
            rendering_offset = glm::vec2(0.f);
            rendering_size = data->size;
        }
        else {
            rendering_offset = (data->size - glm::vec2(2.f)) * 0.5f;
            rendering_size = glm::vec2(2.f);
        }

        //only update displayed animation if the building is visible
        if(lvl()->map.IsTileVisible(Position(), data->size))
            act() = real_act();
        
        RenderAt(glm::vec2(Position()) + rendering_offset, rendering_size, 1e-3f);


        //render flame visuals on damaged buildings
        if(constructed) {
            glm::vec2 pos = glm::vec2(Position()) + rendering_offset + rendering_size * 0.5f;
            if(lvl()->map.IsTileVisible(pos)) {
                float hp = HealthPercentage();
                float frame = Input::Get().CustomAnimationFrame(OID().idx);
                if(hp <= 0.5f) {
                    RenderAt(pos - glm::vec2(1.f, 0.f), glm::vec2(2.f), BuildingAnimationType::FIRE_BIG, frame, -2e-3f);
                }
                else if(hp <= 0.75f) {
                    RenderAt(pos - glm::vec2(0.5f, 0.f), glm::vec2(1.f), BuildingAnimationType::FIRE_SMALL, frame, -2e-3f);
                }
            }
        }
    }

    bool Building::Update() {
        if(!IsActive())
            return (Health() <= 0) || IsKilled();
        action.Update(*this, *lvl());
        return (Health() <= 0) || IsKilled();
    }

    Building::Entry Building::Export() const {
        Building::Entry entry = {};

        FactionObject::Export(entry);
        entry.constructed = constructed;
        entry.amount_left = amount_left;
        entry.real_actionIdx = real_actionIdx;
        entry.action = action.Export();
        
        return entry;
    }

    void Building::RepairIDs(const idMappingType& ids) {
        if(ids.count(action.data.target_id)) action.data.target_id = ids.at(action.data.target_id);
    }

    void Building::IssueAction(const BuildingAction& new_action) {
        action = new_action;
        ENG_LOG_FINE("IssueAction: Building: {} ({}) ... {}", ID(), Name(), action.to_string());
    }

    void Building::CancelAction() {
        ENG_LOG_FINE("CancelAction: Building: {} ({}) ... {}", ID(), Name(), action.to_string());

        glm::ivec3 refund;

        //construction action -> destroy the site, respawn worker & return only portion of the building price
        if(!constructed) {
            refund = 3 * Price() / 4;
            lvl()->objects.IssueExit_Construction(OID());
            Kill();
        }
        else {
            refund = ActionPrice();
        }

        Faction()->RefundResources(refund);
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

    void Building::DecrementResource() {
        ASSERT_MSG(IsResource(), "Building::DecrementResource - invoked on a non resource building.");

        amount_left -= 100;
        if(amount_left <= 0) {
            ENG_LOG_TRACE("Building::DecrementResource - resource depleted ({})", OID());
            Kill();
        }
        lvl()->factions.Player()->SignalGUIUpdate(*this);
    }

    void Building::WithdrawObject(bool inform_faction) {
        if(IsActive()) {
            UnregisterDropoffPoint();
            FactionObject::WithdrawObject(inform_faction);
        }
    }

    ObjectID Building::TransformFoundation(const BuildingDataRef& new_data, const FactionControllerRef& new_faction, bool is_constructed) {
        ENG_LOG_FINE("Building::TransformFoundation - {} -> {} (pos={})", data->name, new_data->name, Position());

        //withdraw object from the map, so that I don't have to modify values in the map struct (will update on reinsert)
        WithdrawObject(true);

        lvl()->map.VisibilityDecrement(Position(), data->size, data->vision_range, FactionIdx());
        lvl()->map.VisibilityIncrement(Position(), new_data->size, new_data->vision_range, new_faction->ID());

        ChangeFaction(new_faction);

        //data pointer & animator updates
        data = new_data;
        UpdateDataPointer(new_data);
        animator = Animator(new_data->animData);

        UnsetKilledFlag();
        Finalized(is_constructed);
        constructed = false;

        if(is_constructed) {
            OnConstructionFinished(true, false);
            SetHealth(MaxHealth());
        }
        else {
            SetHealth(StartingHealth());

        }

        //update buildings's ID - in order to cancel any commands issued on the previous building type
        lvl()->objects.UpdateBuildingID(OID());

        ReinsertObject();


        //calling after reinsert, cuz it resets the action
        if(is_constructed) {
            action = BuildingAction::Idle();
        }
        else {
            action = BuildingAction();
        }

        return OID();
    }

    bool Building::Kill(bool silent) {
        if(IsKilled())
            return true;

        bool res = FactionObject::Kill(silent);

        if(!silent) {
            //non-depleted oil platform -> respawn oil patch in
            if(NumID()[1] == BuildingType::OIL_PLATFORM && amount_left > 0) {
                BuildingDataRef new_data = Resources::LoadBuilding("misc/oil");
                TransformFoundation(new_data, lvl()->factions.Nature(), true);
                res = false;

                //trigger this, as it's normally triggered from destructor (which doesn't get called here)
                lvl()->objects.KillObjectsInside(OID());
                lvl()->factions.Player()->SignalGUIUpdate(*this);
            }
        }

        return res;
    }

    bool Building::IsTraining() const {
        return action.logic.type == BuildingActionType::TRAIN_OR_RESEARCH && action.data.flag;
    }

    bool Building::ProgressableAction() const {
        return (action.logic.type == BuildingActionType::TRAIN_OR_RESEARCH || action.logic.type == BuildingActionType::CONSTRUCTION_OR_UPGRADE);
    }

    float Building::ActionProgress() const {
        switch(action.logic.type) {
            case BuildingActionType::CONSTRUCTION_OR_UPGRADE:
            case BuildingActionType::TRAIN_OR_RESEARCH:
                return action.data.t1 / action.data.t3;
            default:
                return 0.f;
        }
    }

    glm::ivec3 Building::ActionPrice() const {
        switch(action.logic.type) {
            case BuildingActionType::CONSTRUCTION_OR_UPGRADE:
                ASSERT_MSG(((unsigned)action.data.i) < BuildingType::COUNT, "Building::ActionPrice - payload_id doesn't correspond with building type ({}, max {})", action.data.i, BuildingType::COUNT-1);
                return Resources::LoadBuilding(action.data.i, IsOrc())->cost;
            case BuildingActionType::TRAIN_OR_RESEARCH:
                if(action.data.flag) {
                    //training
                    ASSERT_MSG(((unsigned)action.data.i) < UnitType::COUNT, "Building::ActionPrice - payload_id doesn't correspond with unit type ({}, max {})", action.data.i, UnitType::COUNT-1);
                    return Resources::LoadUnit(action.data.i, IsOrc())->cost;
                }
                else {
                    //reseach
                    return Tech().ResearchPrice(action.data.i, IsOrc(), true);
                }
            default:
                ENG_LOG_WARN("Building::ActionPrice - possibly invalid invokation");
                return glm::ivec3(0);
        }
    }

    glm::ivec3 Building::ActionPrice(int command_id, int payload_id) const {
        switch(command_id) {
            case GUI::ActionButton_CommandType::UPGRADE:
                ASSERT_MSG(((unsigned)payload_id) < BuildingType::COUNT, "Building::ActionPrice - payload_id doesn't correspond with building type ({}, max {})", payload_id, BuildingType::COUNT-1);
                return Resources::LoadBuilding(payload_id, IsOrc())->cost;
            case GUI::ActionButton_CommandType::RESEARCH:
                return Tech().ResearchPrice(payload_id, IsOrc(), true);
            case GUI::ActionButton_CommandType::TRAIN:
                ASSERT_MSG(((unsigned)payload_id) < UnitType::COUNT, "Building::ActionPrice - payload_id doesn't correspond with unit type ({}, max {})", payload_id, UnitType::COUNT-1);
                return Resources::LoadUnit(payload_id, IsOrc())->cost;
            default:
                ENG_LOG_WARN("Building::ActionPrice - possibly invalid invokation");
                return glm::ivec3(0);
        }
    }

    float Building::TrainOrResearchTime(bool training, int payload_id) const {
        if(training) {
            return Resources::LoadUnit(payload_id, IsOrc())->build_time;
        }
        else {
            return Tech().ResearchTime(payload_id, IsOrc());
        }
    }

    void Building::OnConstructionFinished(bool registerDropoffPoint, bool kickoutWorkers) {
        ASSERT_MSG(!constructed, "Building::OnConstructionFinished - multiple invokations (building already constructed, {})", OID());
        constructed = true;

        action = BuildingAction::Idle(CanAttack());

        if(registerDropoffPoint && data->dropoff_mask != 0) {
            Faction()->AddDropoffPoint(*this);
        }

        Faction()->ObjectAdded(data);
        
        if(kickoutWorkers) {
            Audio::Play(SoundEffect::GetPath(workdone_sound_name[data->race]), Position());
            lvl()->objects.IssueExit_Construction(OID());
        }
    }

    void Building::OnUpgradeFinished(int payload_id) {
        ASSERT_MSG(constructed, "Building::OnUpgradeFinished - upgraded building should already be constructed (id={})", OID());

        if(payload_id < 0 || payload_id >= BuildingType::COUNT) {
            ENG_LOG_ERROR("Building::OnUpgradeFinished - payload_id is out of range.");
            throw std::out_of_range("");
        }
        BuildingDataRef new_data = Resources::LoadBuilding(payload_id, IsOrc());
        if(new_data == nullptr) {
            ENG_LOG_ERROR("Building::OnUpgradeFinished - invalid data type for the upgrade.");
            throw std::runtime_error("");
        }

        //health update
        int new_health = HealthPercentage() * new_data->MaxHealth();

        //signal to faction controller, so that it updates its stats
        Faction()->ObjectUpgraded(data, new_data);

        //update vision range
        lvl()->map.VisionRangeUpdate(Position(), data->size, data->vision_range, new_data->vision_range);

        //data pointer & animator updates
        data = new_data;
        UpdateDataPointer(new_data);
        animator = Animator(new_data->animData);

        SetHealth(new_health);

        Audio::Play(SoundEffect::GetPath((Faction()->Race() == 0) ? "peasant/pswrkdon" : "orc/owrkdone"), Position());

        Faction()->SignalGUIUpdate();

        action = BuildingAction::Idle(CanAttack());
    }

    void Building::TrainingOrResearchFinished(bool training, int payload_id) {
        if(training) {
            Tech().ApplyUnitUpgrade(payload_id, IsOrc());
            UnitDataRef unit_data = Resources::LoadUnit(payload_id, IsOrc());
            glm::ivec2 spawn_pos;
            lvl()->map.NearbySpawnCoords(Position(), Data()->size, 3, unit_data->navigationType, spawn_pos);
            lvl()->objects.EmplaceUnit(*lvl(), unit_data, Faction(), spawn_pos);
            ENG_LOG_TRACE("Building::TrainingOrResearchFinished - new unit (type={}, name={}) spawned at ({}, {})", payload_id, unit_data->name, spawn_pos.x, spawn_pos.y);
        }
        else {
            int new_level = -1;
            if(Tech().IncrementResearch(payload_id, IsOrc(), &new_level)) {
                ENG_LOG_TRACE("Building::TrainingOrResearchFinished - research (type={}) improved to level {}", payload_id, new_level);
                Audio::Play(SoundEffect::GetPath(workdone_sound_name[data->race]), Position());

                //issue unit upgrade
                if(payload_id == ResearchType::LM_RANGER_UPGRADE) {
                    lvl()->objects.UnitUpgrade(FactionIdx(), UnitType::ARCHER, UnitType::RANGER, IsOrc());
                }
                else if(payload_id == ResearchType::PALA_UPGRADE) {
                    lvl()->objects.UnitUpgrade(FactionIdx(), UnitType::KNIGHT, UnitType::PALADIN, IsOrc());
                }

                Faction()->SignalGUIUpdate();
            }
            else {
                //could maybe not throw & refund the money instead (... but this shouldn't really happen so I guess it doesn't matter that much)
                ENG_LOG_WARN("Building::TrainingOrResearchFinished - research couldn't be upgraded any further (type={}, level={})", payload_id, new_level);
                throw std::runtime_error("");
            }
        }

        action = BuildingAction::Idle(CanAttack());
    }

    void Building::Inner_DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        FactionObject::Inner_DBG_GUI();
        ImGui::Text("%s", action.to_string().c_str());

        static int dir = 0;
        ImGui::DragInt("preferred_dir", &dir);
        if(ImGui::Button("NearbySpawnCoords")) {
            glm::ivec2 coords;
            NearbySpawnCoords(NavigationBit::GROUND, dir, coords);
            ENG_LOG_INFO("RESULT: ({}, {})", coords.x, coords.y);
        }
        ImGui::SliderInt("Amount left", &amount_left, 0, 200000);
#endif
    }

    void Building::InnerIntegrate() {
        if(data->IntegrateInMap())
            FactionObject::InnerIntegrate();
        else
            lvl()->map.AddTraversableObject(OID(), Position(), NumID()[1]);
        
        if(data->IsNotableObject())
            lvl()->map.AddTraversableObject(OID(), Position(), NumID()[1]);
        
        if(constructed) {
            if(data->dropoff_mask != 0) {
                Faction()->AddDropoffPoint(*this);
            }
        }
    }

    void Building::ClearState() {
        action = BuildingAction::Idle(CanAttack());
    }

    std::ostream& Building::DBG_Print(std::ostream& os) const {
        os << "Building - " << Name() << " (ID=" << ID() << ")";
        return os;
    }

    void Building::UnregisterDropoffPoint() {
        if(data->dropoff_mask != 0) {
            Faction()->RemoveDropoffPoint(*this);
        }
    }

    //===== UtilityObject =====

    glm::vec2 UtilityObject::LiveData::InterpolatePosition(float f) {
        return source_pos + f * (target_pos - source_pos);
    }

    UtilityObject::UtilityObject(Level& level_, const UtilityObjectDataRef& data_, const LiveData& init_data_, bool play_sound)
        : GameObject(level_, data_), data(data_) {
        ChangeType(data_, init_data_, play_sound, true);
    }

    UtilityObject::UtilityObject(Level& level_, const UtilityObjectDataRef& data_, const glm::vec2& target_pos_, const ObjectID& targetID_, const LiveData& init_data_, FactionObject& src_, bool play_sound)
        : GameObject(level_, data_), data(data_) {
        ChangeType(data_, target_pos_, targetID_, init_data_, src_, play_sound, true);
    }

    UtilityObject::UtilityObject(Level& level_, const UtilityObjectDataRef& data_, const glm::vec2& target_pos_, const ObjectID& targetID_, FactionObject& src_, bool play_sound)
        : GameObject(level_, data_), data(data_) {
        ChangeType(data_, target_pos_, targetID_, src_, play_sound, true);
    }

    UtilityObject::UtilityObject(Level& level_, const UtilityObjectDataRef& data_, const glm::vec2& target_pos_, const ObjectID& targetID_, bool play_sound)
        : GameObject(level_, data_), data(data_) {
        ChangeType(data_, target_pos_, targetID_, play_sound, true);
    }

    UtilityObject::UtilityObject(Level& level_, const UtilityObject::Entry& entry)
        : GameObject(level_, Resources::LoadUtilityObj(entry.num_id[1]), entry), data(Resources::LoadUtilityObj(entry.num_id[1])) {
        
        live_data = entry.live_data;
        real_position = entry.real_position;
        m_real_size = entry.real_size;
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
        if(res && data->on_done.valid) {
            Audio::Play(data->on_done.Random(), real_pos());
        }
        animator.Update(ActionIdx());
        return res || IsKilled();
    }

    UtilityObject::Entry UtilityObject::Export() const {
        UtilityObject::Entry entry = {};

        GameObject::Export(entry);
        entry.live_data = live_data;
        entry.real_position = real_position;
        entry.real_size = m_real_size;

        return entry;
    }

    void UtilityObject::RepairIDs(const idMappingType& ids) {
        if(ids.count(live_data.sourceID)) live_data.sourceID = ids.at(live_data.sourceID);
        if(ids.count(live_data.targetID)) live_data.targetID = ids.at(live_data.targetID);
        if(ids.count(live_data.ids[0]  )) live_data.ids[0]   = ids.at(live_data.ids[0]);
        if(ids.count(live_data.ids[1]  )) live_data.ids[1]   = ids.at(live_data.ids[1]);
        if(ids.count(live_data.ids[2]  )) live_data.ids[2]   = ids.at(live_data.ids[2]);
        if(ids.count(live_data.ids[3]  )) live_data.ids[3]   = ids.at(live_data.ids[3]);
        if(ids.count(live_data.ids[4]  )) live_data.ids[4]   = ids.at(live_data.ids[4]);
    }

    void UtilityObject::ChangeType(const UtilityObjectDataRef& new_data, glm::vec2 target_pos_, ObjectID targetID_, const LiveData& init_data_, FactionObject& src_, bool play_sound, bool call_init) {
        data = new_data;
        UpdateDataPointer(new_data);

        live_data = init_data_;
        live_data.target_pos = target_pos_;
        live_data.source_pos = src_.PositionCentered(); 
        live_data.targetID = targetID_;
        live_data.sourceID = src_.OID();

        if(call_init) {
            //this might throw if invoked with Init() handler, that requires src to not be null
            ASSERT_MSG(data->Init != nullptr, "UtilityObject - Init() handler in prefab '{}' isn't setup properly!", data->name);
            data->Init(*this, &src_);
        }

        if(play_sound && data->on_spawn.valid) {
            Audio::Play(data->on_spawn.Random(), Position());
        }
    }

    void UtilityObject::ChangeType(const UtilityObjectDataRef& new_data, const LiveData& init_data_, bool play_sound, bool call_init) {
        data = new_data;
        UpdateDataPointer(new_data);

        live_data = init_data_;

        if(call_init) {
            //this might throw if invoked with Init() handler, that requires src to not be null
            ASSERT_MSG(data->Init != nullptr, "UtilityObject - Init() handler in prefab '{}' isn't setup properly!", data->name);
            data->Init(*this, nullptr);
        }

        if(play_sound && data->on_spawn.valid) {
            Audio::Play(data->on_spawn.Random(), Position());
        }
    }

    void UtilityObject::ChangeType(const UtilityObjectDataRef& new_data, glm::vec2 target_pos_, ObjectID targetID_, FactionObject& src_, bool play_sound, bool call_init) {
        data = new_data;
        UpdateDataPointer(new_data);

        live_data.target_pos = target_pos_;
        live_data.source_pos = src_.PositionCentered(); 
        live_data.targetID = targetID_;
        live_data.sourceID = src_.OID();

        if(call_init) {
            //this might throw if invoked with Init() handler, that requires src to not be null
            ASSERT_MSG(data->Init != nullptr, "UtilityObject - Init() handler in prefab '{}' isn't setup properly!", data->name);
            data->Init(*this, &src_);
        }

        if(play_sound && data->on_spawn.valid) {
            Audio::Play(data->on_spawn.Random(), Position());
        }
    }

    void UtilityObject::ChangeType(const UtilityObjectDataRef& new_data, glm::vec2 target_pos_, ObjectID targetID_, bool play_sound, bool call_init) {
        data = new_data;
        UpdateDataPointer(new_data);

        live_data.target_pos = target_pos_;
        live_data.sourceID = ObjectID();

        live_data.source_pos = glm::ivec2(-1);
        live_data.targetID = targetID_;

        if(call_init) {
            //this might throw if invoked with Init() handler, that requires src to not be null
            ASSERT_MSG(data->Init != nullptr, "UtilityObject - Init() handler in prefab '{}' isn't setup properly!", data->name);
            data->Init(*this, nullptr);
        }

        if(play_sound && data->on_spawn.valid) {
            Audio::Play(data->on_spawn.Random(), Position());
        }
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
                target->PanicMovement(level.factions.Diplomacy().AreHostile(src.FactionIdx(), target->FactionIdx()), src.NavigationType() != target->NavigationType());
                target->LastHitFaction(src.FactionIdx());
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

    bool ApplyDamage(Level& level, int basicDamage, int pierceDamage, const ObjectID& targetID, const glm::ivec2& target_pos, const ObjectID& srcID) {
        bool attack_happened = false;
        if(targetID.type != ObjectType::MAP_OBJECT) {
            //target is regular object

            FactionObject* src = nullptr;
            if(ObjectID::IsValid(srcID)) {
                level.objects.GetObject(srcID, src);
            }

            FactionObject* target = nullptr;
            if(level.objects.GetObject(targetID, target)) {
                target->ApplyDirectDamage(basicDamage, pierceDamage);
                attack_happened = true;
                if(src != nullptr) {
                    target->PanicMovement(level.factions.Diplomacy().AreHostile(src->FactionIdx(), target->FactionIdx()), src->NavigationType() != target->NavigationType());
                    target->LastHitFaction(src->FactionIdx());
                }
                else {
                    target->LastHitFaction(-1);
                }
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

    bool ApplyDamageFlat(Level& level, int damage, const ObjectID& targetID, const glm::ivec2& target_pos, const ObjectID& srcID) {
        bool attack_happened = false;
        if(targetID.type != ObjectType::MAP_OBJECT) {
            //target is regular object

            FactionObject* src = nullptr;
            if(ObjectID::IsValid(srcID)) {
                level.objects.GetObject(srcID, src);
            }

            FactionObject* target = nullptr;
            if(level.objects.GetObject(targetID, target)) {
                target->ApplyDamageFlat(damage);
                attack_happened = true;
                if(src != nullptr) {
                    target->PanicMovement(level.factions.Diplomacy().AreHostile(src->FactionIdx(), target->FactionIdx()), src->NavigationType() != target->NavigationType());
                    target->LastHitFaction(src->FactionIdx());
                }
                else {
                    target->LastHitFaction(-1);
                }
            }
        }
        else {
            //target is attackable map object
            const TileData& tile = level.map(target_pos);
            if(IsWallTile(tile.tileType)) {
                level.map.DamageTile(target_pos, damage);
                attack_happened = true;
            }
        }
        return attack_happened;
    }

    std::pair<int,int> ApplyDamage_Splash(Level& level, int basicDamage, int pierceDamage, const glm::ivec2& target_pos, int radius, const ObjectID& exceptID, const ObjectID& srcID, bool dropoff) {
        int hit_count = 0;
        int total_damage = 0;

        FactionObject* src = nullptr;
        if(ObjectID::IsValid(srcID)) {
            level.objects.GetObject(srcID, src);
        }

        //to avoid applying damage to the same object multiple times
        std::vector<ObjectID> hit_objects = {};

        glm::ivec2 corner = target_pos - (radius-1);
        int diameter = (radius-1)*2 + 1;
        for(int y = 0; y < diameter; y++) {
            for(int x = 0; x < diameter; x++) {
                glm::ivec2 pos = glm::ivec2(corner.x + x, corner.y + y);

                if(!level.map.IsWithinBounds(pos))
                    continue;

                const TileData& td = level.map(pos);
                float m =  dropoff ? ((pos == target_pos) ? 1.f : 0.5f) : 1.f;
                int B = int(basicDamage  * m);
                int P = int(pierceDamage * m);
                
                //apply damage to regular objects (ground & flying)
                for(int i = 0; i < 2; i++) {
                    ObjectID id = td.info[i].id;
                    if(id != exceptID && ObjectID::IsObject(id) && (std::find(hit_objects.begin(), hit_objects.end(), id) == hit_objects.end())) {
                        hit_objects.push_back(id);
                        FactionObject* target = nullptr;
                        if(level.objects.GetObject(id, target)) {
                            total_damage += target->ApplyDirectDamage(B, P);
                            hit_count++;
                            if(src != nullptr) {
                                target->PanicMovement(level.factions.Diplomacy().AreHostile(src->FactionIdx(), target->FactionIdx()), src->NavigationType() != target->NavigationType());
                                target->LastHitFaction(src->FactionIdx());
                            }
                            else {
                                target->LastHitFaction(-1);
                            }
                        }
                    }
                }

                //apply damage to map objects
                ObjectID wallID = ObjectID(ObjectType::MAP_OBJECT, pos.x, pos.y);
                if(wallID != exceptID && IsWallTile(td.tileType) && (std::find(hit_objects.begin(), hit_objects.end(), wallID) == hit_objects.end())) {
                    hit_objects.push_back(wallID);
                    total_damage += level.map.DamageTile(pos, B);
                    hit_count++;
                }

            }
        }

        return { hit_count, total_damage };
    }

    std::pair<int,int> ApplyDamageFlat_Splash(Level& level, int damage, const glm::ivec2& target_pos, int radius, const ObjectID& exceptID, const ObjectID& srcID, bool dropoff) {
        int hit_count = 0;
        int total_damage = 0;

        FactionObject* src = nullptr;
        if(ObjectID::IsValid(srcID)) {
            level.objects.GetObject(srcID, src);
        }

        //to avoid applying damage to the same object multiple times
        std::vector<ObjectID> hit_objects = {};

        glm::ivec2 corner = target_pos - (radius-1);
        int diameter = (radius-1)*2 + 1;
        for(int y = 0; y < diameter; y++) {
            for(int x = 0; x < diameter; x++) {
                glm::ivec2 pos = glm::ivec2(corner.x + x, corner.y + y);

                if(!level.map.IsWithinBounds(pos))
                    continue;

                const TileData& td = level.map(pos);
                float m =  dropoff ? ((pos == target_pos) ? 1.f : 0.5f) : 1.f;
                int D = int(damage  * m);
                
                //apply damage to regular objects (ground & flying)
                for(int i = 0; i < 2; i++) {
                    ObjectID id = td.info[i].id;
                    if(id != exceptID && ObjectID::IsObject(id) && (std::find(hit_objects.begin(), hit_objects.end(), id) == hit_objects.end())) {
                        hit_objects.push_back(id);
                        FactionObject* target = nullptr;
                        if(level.objects.GetObject(id, target)) {
                            total_damage += target->ApplyDamageFlat(D);
                            hit_count++;
                            if(src != nullptr) {
                                target->PanicMovement(level.factions.Diplomacy().AreHostile(src->FactionIdx(), target->FactionIdx()), src->NavigationType() != target->NavigationType());
                                target->LastHitFaction(src->FactionIdx());
                            }
                            else {
                                target->LastHitFaction(-1);
                            }
                        }
                    }
                }

                //apply damage to map objects
                ObjectID wallID = ObjectID(ObjectType::MAP_OBJECT, pos.x, pos.y);
                if(wallID != exceptID && IsWallTile(td.tileType) && (std::find(hit_objects.begin(), hit_objects.end(), wallID) == hit_objects.end())) {
                    hit_objects.push_back(wallID);
                    total_damage += level.map.DamageTile(pos, D);
                    hit_count++;
                }

            }
        }

        return { hit_count, total_damage };
    }

}//namespace eng
