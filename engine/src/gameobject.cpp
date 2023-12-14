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

    void GameObject::RenderAt(const glm::vec2& base_pos, const glm::vec2& base_size, float scaling, bool centered, float zOffset) {
        ASSERT_MSG(data != nullptr && level != nullptr, "GameObject isn't properly initialized!");

        glm::vec2 size = base_size * scaling;
        glm::vec2 pos = base_pos;
        if(centered) {
            pos = (pos + 0.5f) - size * 0.5f;
        }

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

    int GameObject::NearbySpawnCoords(int nav_type, int preferred_direction, glm::ivec2& out_coords, int max_range) {
        return lvl()->map.NearbySpawnCoords(position, data->size, preferred_direction, nav_type, out_coords, max_range);
    }

    Level* GameObject::lvl() {
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

    FactionObject::~FactionObject() {
        RemoveFromMap();
        lvl()->objects.KillObjectsInside(OID());
        if(lvl() != nullptr && Data() != nullptr) {
            ENG_LOG_TRACE("Removing {}", OID());
            if(finalized) {
                faction->ObjectRemoved(data_f);
            }
        }
        //TODO: maybe establish an observer relationship with faction object (to track numbers of various types of units, etc.)
    }

    void FactionObject::ChangeColor(int newColorIdx) {
        colorIdx = ValidColor(newColorIdx) ? newColorIdx : faction->GetColorIdx(Data()->num_id);
        animator.SetPaletteIdx((float)colorIdx);
    }

    void FactionObject::Kill(bool silent) {
        if(IsKilled())
            return;
        GameObject::Kill(silent);
        
        if(!silent) {
            UtilityObjectDataRef corpse_data = Resources::LoadUtilityObj("corpse");
            //spawn a corpse utility object
            lvl()->objects.EmplaceUtilityObj(*lvl(), corpse_data, Position(), ObjectID(), *this);
            
            //play the dying sound effect
            static constexpr std::array<const char*, 6> sound_name = { "misc/bldexpl1", "human/hdead", "orc/odead", "ships/shipsink", "misc/explode", "misc/firehit" };
            Audio::Play(SoundEffect::GetPath(sound_name[data_f->deathSoundIdx]), Position());
        }
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

    void FactionObject::AddHealth(float value) {
        health += value;
        health = (health <= MaxHealth()) ? health : MaxHealth();
    }

    Techtree& FactionObject::Tech() { return faction->Tech(); }
    const Techtree& FactionObject::Tech() const { return faction->Tech(); }

    void FactionObject::WithdrawObject() {
        if(active) {
            RemoveFromMap();
            active = false;
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
        }
    }

    void FactionObject::Inner_DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        GameObject::Inner_DBG_GUI();
        ImGui::Text("Active: %s", active ? "True" : "False");
        const char* btn_labels[2] { "Reinsert object", "Withdraw object" };
        if(ImGui::Button(btn_labels[(int)active])) {
            if(active)
                WithdrawObject();
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
        lvl()->map.AddObject(NavigationType(), Position(), glm::ivec2(Data()->size), OID(), FactionIdx(), colorIdx, Data()->objectType == ObjectType::BUILDING, VisionRange());
    }

    void FactionObject::RemoveFromMap() {
        if(active) {
            if(lvl() != nullptr && Data() != nullptr) {
                if(data_f->IntegrateInMap()) {
                    //remove from pathfinding (if it's a part of it to begin with)
                    lvl()->map.RemoveObject(NavigationType(), Position(), glm::ivec2(Data()->size), Data()->objectType == ObjectType::BUILDING);
                }
                else {
                    lvl()->map.RemoveTraversableObject(OID());
                }
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

    //===== Unit =====

    Unit::Unit(Level& level_, const UnitDataRef& data_, const FactionControllerRef& faction_, const glm::vec2& position_, bool playReadySound)
        : FactionObject(level_, data_, faction_, position_), data(data_) {
        
        if(playReadySound && data->sound_ready.valid)
            Audio::Play(data->sound_ready.Random());
    }

    Unit::~Unit() {}

    void Unit::Render() {
        if(!IsActive())
            return;
        bool airborne = (NavigationType() == NavigationBit::AIR);
        RenderAt(glm::vec2(Position()) + move_offset, data->size, data->scale, !airborne, airborne ? (-1e-3f) : 0.f);
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

        //data pointer & animator updates
        data = new_data;
        UpdateDataPointer(new_data);
        animator = Animator(new_data->animData);

        SetHealth(new_health);

        return true;
    }

    glm::vec2 Unit::RenderPosition() const {
        glm::vec2 pos = glm::vec2(Position()) + move_offset;
        if(NavigationType() != NavigationBit::AIR) {
            pos = (pos + 0.5f) - RenderSize() * 0.5f;
        }
        return pos;
    }

    glm::vec2 Unit::RenderSize() const {
        return glm::vec2(data->size) * data->scale;
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

    void Unit::Inner_DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        FactionObject::Inner_DBG_GUI();
        ImGui::Text("move_offset: %s", to_string(move_offset).c_str());
        ImGui::Text("%s", command.to_string().c_str());
        ImGui::Text("Action type: %d", action.logic.type);
        ImGui::Text("Carry state: %d", carry_state);
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
        SetVariationIdx((carry_state != WorkerCarryState::NONE) ? (1+(carry_state-1)*2) : 0);
    }

    void Unit::ManaIncrement() {
        if(IsCaster()) {
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

    //===== Building =====

    Building::Building(Level& level_, const BuildingDataRef& data_, const FactionControllerRef& faction_, const glm::vec2& position_, bool constructed)
        : FactionObject(level_, data_, faction_, position_, false), data(data_) {
        
        if(constructed) {
            //can't register dropoff point from constructor - ID isn't setup properly yet (called from IntegrateIntoLevel() instead)
            OnConstructionFinished(false, false);
        }
        else {
            //starting health uptick
            SetHealth(StartingHealth());
        }
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
        
        RenderAt(glm::vec2(Position()) + rendering_offset, rendering_size, 1e-3f);
    }

    bool Building::Update() {
        if(!IsActive())
            return (Health() <= 0) || IsKilled();
        action.Update(*this, *lvl());
        return (Health() <= 0) || IsKilled();
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

    void Building::WithdrawObject() {
        if(IsActive()) {
            UnregisterDropoffPoint();
            FactionObject::WithdrawObject();
        }
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

        //data pointer & animator updates
        data = new_data;
        UpdateDataPointer(new_data);
        animator = Animator(new_data->animData);

        SetHealth(new_health);

        Audio::Play(SoundEffect::GetPath((Faction()->Race() == 0) ? "peasant/pswrkdon" : "orc/owrkdone"), Position());

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

                }
                
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
#endif
    }

    void Building::InnerIntegrate() {
        if(data->IntegrateInMap())
            FactionObject::InnerIntegrate();
        else {
            lvl()->map.AddTraversableObject(OID());
        }
        
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
        return glm::vec2(source_pos) + f * glm::vec2(target_pos - source_pos);
    }

    UtilityObject::UtilityObject(Level& level_, const UtilityObjectDataRef& data_, const glm::ivec2& target_pos_, const ObjectID& targetID_, FactionObject& src_, bool play_sound)
        : GameObject(level_, data_), data(data_) {
        
        live_data.target_pos = target_pos_;
        live_data.source_pos = src_.Position();
        live_data.targetID = targetID_;
        live_data.sourceID = src_.OID();

        ASSERT_MSG(data->Init != nullptr, "UtilityObject - Init() handler in prefab '{}' isn't setup properly!", data->name);
        data->Init(*this, src_);

        if(play_sound && data->on_spawn.valid) {
            Audio::Play(data->on_spawn.Random(), Position());
        }
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
