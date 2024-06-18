#include "engine/game/utility_handlers.h"

#include "engine/core/input.h"
#include "engine/game/gameobject.h"
#include "engine/game/level.h"
#include "engine/game/resources.h"
#include "engine/core/audio.h"

#include "engine/utils/randomness.h"

#include <tuple>
#include <sstream>

#define Z_OFFSET -0.1f

static constexpr glm::vec2 BUFF_ICON_SIZE = glm::vec2(4.f / 9.f);
static constexpr int BUFF_ROW_LENGTH = 4;

static constexpr int FLAME_SHIELD_NUM_FLAMES = 5;
static constexpr float TORNADO_MOTION_PROBABILITY = 0.3f;

namespace eng {

    void UtilityHandler_Default_Render(UtilityObject& obj);
    void UtilityHandler_No_Render(UtilityObject& obj);

    bool UtilityHandler_No_Update(UtilityObject& obj, Level& level);

    void UtilityHandler_Projectile_Init(UtilityObject& obj, FactionObject* src);
    bool UtilityHandler_Projectile_Update(UtilityObject& obj, Level& level);

    void UtilityHandler_Corpse_Init(UtilityObject& obj, FactionObject* src);
    bool UtilityHandler_Corpse_Update(UtilityObject& obj, Level& level);
    void UtilityHandler_Corpse_Render(UtilityObject& obj);

    void UtilityHandler_Visuals_Init(UtilityObject& obj, FactionObject* src);
    bool UtilityHandler_Visuals_Update(UtilityObject& obj, Level& level);
    bool UtilityHandler_Visuals_Update2(UtilityObject& obj, Level& level);

    void UtilityHandler_Buff_Init(UtilityObject& obj, FactionObject* src);
    bool UtilityHandler_Buff_Update(UtilityObject& obj, Level& level);
    void UtilityHandler_Buff_Render(UtilityObject& obj);

    void UtilityHandler_Heal_Init(UtilityObject& obj, FactionObject* src);

    void UtilityHandler_Vision_Init(UtilityObject& obj, FactionObject* src);
    bool UtilityHandler_Vision_Update(UtilityObject& obj, Level& level);

    void UtilityHandler_Minion_Init(UtilityObject& obj, FactionObject* src);
    bool UtilityHandler_Minion_Update(UtilityObject& obj, Level& level);

    void UtilityHandler_Polymorph_Init(UtilityObject& obj, FactionObject* src);

    void UtilityHandler_FlameShield_Init(UtilityObject& obj, FactionObject* src);
    bool UtilityHandler_FlameShield_Update(UtilityObject& obj, Level& level);
    void UtilityHandler_FlameShield_Render(UtilityObject& obj);

    void UtilityHandler_Tornado_Init(UtilityObject& obj, FactionObject* src);
    bool UtilityHandler_Tornado_Update(UtilityObject& obj, Level& level);

    void UtilityHandler_Runes_Init(UtilityObject& obj, FactionObject* src);
    bool UtilityHandler_Runes_Update(UtilityObject& obj, Level& level);
    void UtilityHandler_Runes_Render(UtilityObject& obj);

    void UtilityHandler_ExoCoil_Init(UtilityObject& obj, FactionObject* src);

    void UtilityHandler_BlizzDnD_Init(UtilityObject& obj, FactionObject* src);
    bool UtilityHandler_BlizzDnD_Update(UtilityObject& obj, Level& level);

    void UtilityHandler_Demolish_Init(UtilityObject& obj, FactionObject* src);

    //===================================================================

    namespace CorpseAnimID { enum { CORPSE1_HU = 0, CORPSE1_OC, CORPSE2, CORPSE_WATER, RUINS_GROUND, RUINS_GROUND_WASTELAND, RUINS_WATER, EXPLOSION }; }

    //===================================================================

    void ResolveUtilityHandlers(UtilityObjectDataRef& data, int id) {
        using handlerRefs = std::tuple<UtilityObjectData::InitHandlerFn, UtilityObjectData::UpdateHandlerFn, UtilityObjectData::RenderHandlerFn>;

        switch(id) {
            case UtilityObjectType::PROJECTILE:
                std::tie(data->Init, data->Update, data->Render) = handlerRefs{ UtilityHandler_Projectile_Init, UtilityHandler_Projectile_Update, UtilityHandler_Default_Render };
                break;
            case UtilityObjectType::CORPSE:
                std::tie(data->Init, data->Update, data->Render) = handlerRefs{ UtilityHandler_Corpse_Init, UtilityHandler_Corpse_Update, UtilityHandler_Corpse_Render };
                break;
            case UtilityObjectType::VISUALS:
                std::tie(data->Init, data->Update, data->Render) = handlerRefs{ UtilityHandler_Visuals_Init, UtilityHandler_Visuals_Update, UtilityHandler_Default_Render };
                break;
            case UtilityObjectType::SPELL:
                //pick handler based on spellID
                switch(data->i1) {
                    case SpellID::HEAL:
                        std::tie(data->Init, data->Update, data->Render) = handlerRefs{ UtilityHandler_Heal_Init, UtilityHandler_Visuals_Update2, UtilityHandler_Default_Render };
                        break;
                    case SpellID::HOLY_VISION:
                        std::tie(data->Init, data->Update, data->Render) = handlerRefs{ UtilityHandler_Vision_Init, UtilityHandler_Vision_Update, UtilityHandler_Default_Render };
                        break;
                    case SpellID::EYE:
                        std::tie(data->Init, data->Update, data->Render) = handlerRefs{ UtilityHandler_Minion_Init, UtilityHandler_Minion_Update, UtilityHandler_No_Render };
                        data->b1 = false;
                        break;
                    case SpellID::RAISE_DEAD:
                        std::tie(data->Init, data->Update, data->Render) = handlerRefs{ UtilityHandler_Minion_Init, UtilityHandler_Minion_Update, UtilityHandler_No_Render };
                        data->b1 = true;
                        break;
                    case SpellID::POLYMORPH:
                        std::tie(data->Init, data->Update, data->Render) = handlerRefs{ UtilityHandler_Polymorph_Init, UtilityHandler_Visuals_Update, UtilityHandler_Default_Render };
                        break;
                    case SpellID::FLAME_SHIELD:
                        std::tie(data->Init, data->Update, data->Render) = handlerRefs{ UtilityHandler_FlameShield_Init, UtilityHandler_FlameShield_Update, UtilityHandler_FlameShield_Render };
                        break;
                    case SpellID::TORNADO:
                        std::tie(data->Init, data->Update, data->Render) = handlerRefs{ UtilityHandler_Tornado_Init, UtilityHandler_Tornado_Update, UtilityHandler_Default_Render };
                        break;
                    case SpellID::RUNES:
                        std::tie(data->Init, data->Update, data->Render) = handlerRefs{ UtilityHandler_Runes_Init, UtilityHandler_Runes_Update, UtilityHandler_Runes_Render };
                        break;
                    case SpellID::DEATH_COIL:
                    case SpellID::EXORCISM:
                        std::tie(data->Init, data->Update, data->Render) = handlerRefs{ UtilityHandler_ExoCoil_Init, UtilityHandler_No_Update, UtilityHandler_No_Render };
                        break;
                    case SpellID::BLIZZARD:
                    case SpellID::DEATH_AND_DECAY:
                        std::tie(data->Init, data->Update, data->Render) = handlerRefs{ UtilityHandler_BlizzDnD_Init, UtilityHandler_BlizzDnD_Update, UtilityHandler_No_Render };
                        break;
                    case SpellID::DEMOLISH:
                        std::tie(data->Init, data->Update, data->Render) = handlerRefs{ UtilityHandler_Demolish_Init, UtilityHandler_No_Update, UtilityHandler_No_Render };
                        break;
                    default:
                        ENG_LOG_ERROR("UtilityObject - ResolveUtilityHandlers - invalid spell ID ({})", data->i1);
                        throw std::runtime_error("");
                }
                break;
            case UtilityObjectType::BUFF:
                std::tie(data->Init, data->Update, data->Render) = handlerRefs{ UtilityHandler_Buff_Init, UtilityHandler_Buff_Update, UtilityHandler_Buff_Render };
                break;
            default:
                ENG_LOG_ERROR("UtilityObject - ResolveUtilityHandlers - invalid utility type ID ({})", id);
                throw std::runtime_error("");
        }
    }

    //===================================================================

    void UtilityHandler_Default_Render(UtilityObject& obj) {
        if(!obj.lvl()->map.IsWithinBounds(obj.real_pos()) || !obj.lvl()->map.IsTileVisible(obj.real_pos()))
            return;
        obj.RenderAt(obj.real_pos() - obj.real_size() * 0.5f, obj.real_size(), Z_OFFSET);
    }

    void UtilityHandler_No_Render(UtilityObject& obj) {}

    bool UtilityHandler_No_Update(UtilityObject& obj, Level& level) {
        return true;
    }

    void UtilityHandler_Projectile_Init(UtilityObject& obj, FactionObject* src) {
        UtilityObject::LiveData& d = obj.LD();
        UtilityObjectData& ud = *obj.UData();

        //damage possibly passed in runtime
        int live_damage = d.i1;
        int dmg_base = d.i2;
        int dmg_pierce = d.i3;

        //set projectile orientation, start & end time, starting position and copy unit's damage values
        glm::vec2 target_dir = d.target_pos - d.source_pos;
        d.i1 = VectorOrientation(target_dir / glm::length(target_dir));
        d.f1 = d.f2 = (float)Input::CurrentTime();
        obj.real_size() = obj.SizeScaled();

        if(src != nullptr) {
            obj.real_pos() = src->PositionCentered();
        }
        else {
            obj.real_pos() = d.source_pos;
        }

        //projectile damage setup (use caster's stats if not set in object data)
        if(live_damage != 0) {
            d.i2 = dmg_base;
            d.i3 = dmg_pierce;
        }
        else if(ud.b2) {
            d.i2 = d.i3 = 0;
        }
        else if(ud.i3 + ud.i4 == 0 && src != nullptr) {
            d.i2 = src->BasicDamage();
            d.i3 = src->PierceDamage();
        }
        else {
            d.i2 = ud.i3;
            d.i3 = ud.i4;
        }

        //bounce count for bouncing projectiles
        d.i4 = obj.UData()->b1 ? obj.UData()->i2 : 0;

        //store duration in separate variable, so that it can be changed for bouncing
        d.f3 = obj.UData()->duration;
        if(ud.b4) d.f3 -= Random::Uniform() * 1.f;
        d.f3 = 1.f / d.f3;

        ENG_LOG_FINE("UtilityObject - Projectile spawned (guided={}, from {} to {})", obj.UData()->Projectile_IsAutoGuided(), d.source_pos, d.target_pos);
    }

    bool UtilityHandler_Projectile_Update(UtilityObject& obj, Level& level) {
        UtilityObject::LiveData& d = obj.LD();
        UtilityObjectData ud = *obj.UData();
        Input& input = Input::Get();

        obj.act() = 0;
        obj.ori() = d.i1;

        d.f2 += input.deltaTime;
        float t = (d.f2 - d.f1) * d.f3;

        //adding sizes cuz rendering uses Quad::FromCorner
        obj.real_pos() = d.InterpolatePosition(t);
        if(t >= 1.f) {
            //damage application from the projectile
            if(ud.Projectile_IsAutoGuided()) {
                if(!ud.b3)
                    ApplyDamage(level, d.i2, d.i3, d.targetID, d.target_pos);
                else
                    ApplyDamageFlat(level, d.i3, d.targetID, d.target_pos);
            }
            else
                ApplyDamage_Splash(level, d.i2, d.i3, d.target_pos, ud.Projectile_SplashRadius(), ud.b3 ? d.sourceID : ObjectID());

            //optional spawning of followup object
            if(ud.spawn_followup) {
                UtilityObjectDataRef data =  Resources::LoadUtilityObj(ud.followup_name);
                if(data != nullptr) {
                    level.objects.QueueUtilityObject(level, data, d.target_pos, d.targetID);
                }
                else {
                    ENG_LOG_WARN("UtilityHandler_Projectile_Update - invalid followup object ('{}')", ud.followup_name);
                }
            }

            //projectile "bouncing"
            if(ud.b1 && d.i4 > 0) {
                //start next bounce instead of terminating
                d.i4--;                             //decrease the bounce counter
                glm::vec2 dir = glm::normalize(d.target_pos - d.source_pos);
                d.source_pos = d.target_pos;        //setup new target position
                d.target_pos = d.target_pos + dir;
                d.f1 = d.f2 = (float)Input::CurrentTime();
                d.f3 = 1.f / 0.25f;    //update duration for the bounces

                //manually play the on done sound, since it normally only spawns when object dies
                if(ud.on_done.valid)
                    Audio::Play(ud.on_done.Random(), d.source_pos);

                return false;
            }
            return true;
        }

        return false;
    }

    void UtilityHandler_Corpse_Init(UtilityObject& obj, FactionObject* src) {
        UtilityObject::LiveData& d = obj.LD();
        AnimatorDataRef anim = obj.Data()->animData;

        if(src == nullptr) {
            ENG_LOG_ERROR("UtilityHandler::Init - handler requires a valid pointer to the source object.");
            throw std::runtime_error("");
        }

        //store object's orientation
        d.i4 = src->Orientation();
        //TODO: maybe round up the orientation, as there are usually only 2 directions for dying animations

        obj.real_pos() = d.target_pos;
        obj.real_size() = (src->NavigationType() == NavigationBit::GROUND && src->NumID()[1] != UnitType::BALLISTA) ? obj.Data()->size : src->RenderSize();
        obj.pos() = src->Position();

        //1st & 2nd anim indices
        d.i1 = src->DeathAnimIdx();
        d.i2 = -1;

        //mark as ressurectable (for raise dead)
        // d.i7 = src->IsUnit() && src->NavigationType() == NavigationBit::GROUND;
        d.i7 = Unit::IsRessurectable(src->NumID(), src->NavigationType());

        if(d.i1 < 0) {
            //1st anim has invalid idx -> play explosion animation
            d.i3 = 1;
            d.f2 = anim->GetGraphics(CorpseAnimID::EXPLOSION).Duration();

            if(!src->IsUnit()) {
                //src is a building -> display crater animation (along with explosion)
                int is_wasteland = int(obj.lvl()->map.GetTileset()->GetType() == TilesetType::WASTELAND);
                d.i1 = (src->NavigationType() == NavigationBit::GROUND) ? (CorpseAnimID::RUINS_GROUND + is_wasteland) : CorpseAnimID::RUINS_WATER;
                d.f1 = anim->GetGraphics(d.i1).Duration();

                obj.real_pos() = glm::vec2(src->Position()) + (src->Data()->size - glm::vec2(2.f)) * 0.5f;
                obj.real_size() = glm::vec2(2.f);
            }
        }
        else {
            d.f1 = anim->GetGraphics(d.i1).Duration() + 1.f;
            if(src->IsUnit() && d.i7) {
                //2nd animation - transitioned to from the 1st one
                if(src->NavigationType() == NavigationBit::GROUND) {
                    d.i2 = CorpseAnimID::CORPSE1_HU + src->Race();
                    d.f2 =  anim->GetGraphics(d.i2).Duration();
                }
                else if(src->NavigationType() == NavigationBit::WATER) {
                    d.i2 = CorpseAnimID::CORPSE_WATER;
                    d.f2 = anim->GetGraphics(d.i2).Duration();
                }
            }
        }

        //visibility setup (copy object's original vision range)
        d.i5 = src->VisionRange();
        d.i6 = src->FactionIdx();
        src->lvl()->map.VisibilityIncrement(obj.real_pos(), obj.real_size(), d.i5, d.i6);

        // ENG_LOG_TRACE("CORPSE OBJECT - INIT: {}, {}, {}, {}, {}", d.i1, d.i2, d.i3, d.f1, d.f2);
        ENG_LOG_FINE("UtilityObject - Corpse spawned at {}", d.target_pos);
    }

    bool UtilityHandler_Corpse_Update(UtilityObject& obj, Level& level) {
        UtilityObject::LiveData& d = obj.LD();
        Input& input = Input::Get();

        d.f1 -= input.deltaTime;

        //switch from 1st to 2nd animation when the time runs out
        if(d.f1 <= 0.f) {
            d.i1 = d.i2;
            d.f1 += d.f2;
            d.i2 = -1;

            //visibility update - from object's original vision range to small circle around the corpse
            obj.lvl()->map.VisibilityDecrement(obj.real_pos(), obj.real_size(), d.i5, d.i6);
            d.i5 = (d.i5 != 2) ? 1 : 0;     //no vision for 3rd animation
            obj.lvl()->map.VisibilityIncrement(obj.real_pos(), obj.real_size(), d.i5, d.i6);

            //dying ground unit - queue 3rd animation (generic, decayed corpse)
            if(d.i1 == CorpseAnimID::CORPSE1_HU || d.i1 == CorpseAnimID::CORPSE1_OC) {
                d.i2 = CorpseAnimID::CORPSE2;
                d.f2 = obj.Data()->animData->GetGraphics(d.i2).Duration();
            }
        }

        //update the explosion animation
        if(d.i3) {
            d.f2 -= input.deltaTime;
            if(d.f2 <= 0.f) {
                d.i3 = 0;       //hides the explosion visuals
            }
        }
        
        obj.act() = d.i1;
        obj.ori() = d.i4;

        //terminate if ran out of animations to play
        if(d.i1 < 0) {
            //also remove the visibility
            obj.lvl()->map.VisibilityDecrement(obj.real_pos(), obj.real_size(), d.i5, d.i6);
            return true;
        }
        else
            return false;
    }

    void UtilityHandler_Corpse_Render(UtilityObject& obj) {
        if(!obj.lvl()->map.IsTileVisible(obj.real_pos()))
            return;

        //regular corpse animation
        if(obj.LD().i1 >= 0) {
            obj.RenderAt(obj.real_pos(), obj.real_size());
        }
        
        //explosion animation
        if(obj.LD().i3) {
            int prev_id = obj.act();
            obj.act() = CorpseAnimID::EXPLOSION;
            obj.RenderAt(obj.real_pos(), obj.real_size(), -1e-3f);
            obj.act() = prev_id;
        }
    }

    void UtilityHandler_Visuals_Init(UtilityObject& obj, FactionObject* src) {
        UtilityObject::LiveData& d = obj.LD();
        AnimatorDataRef anim = obj.Data()->animData;

        obj.real_pos() = d.target_pos;
        obj.real_size() = obj.SizeScaled();

        d.f1 = 0.f;

        ENG_LOG_FINE("UtilityObject - Visuals spawned at {}", d.target_pos);
    }

    bool UtilityHandler_Visuals_Update(UtilityObject& obj, Level& level) {
        UtilityObject::LiveData& d = obj.LD();
        Input& input = Input::Get();

        obj.act() = 0;
        obj.ori() = 0;

        d.f1 += input.deltaTime;
        float t = d.f1 / obj.UData()->duration;
        
        return (t >= 1.f);
    }

    bool UtilityHandler_Visuals_Update2(UtilityObject& obj, Level& level) {
        UtilityObject::LiveData& d = obj.LD();
        Input& input = Input::Get();

        obj.act() = 0;
        obj.ori() = 0;

        d.f1 += input.deltaTime;
        float t = d.f1 / d.f2;
        
        return (t >= 1.f);
    }

    void UtilityHandler_Buff_Init(UtilityObject& obj, FactionObject* src) {
        UtilityObject::LiveData& d = obj.LD();
        AnimatorDataRef anim = obj.Data()->animData;

        obj.real_pos() = d.target_pos;
        obj.real_size() = obj.Data()->size;

        d.f1 = 0.f;
        int spellID = d.i1 = obj.UData()->i1;
        int buffIdx = SpellID::Spell2Buff(spellID);

        //store info about the target being airborne or not (used for visibility check during rendering)
        d.i2 = obj.NavigationType() == NavigationBit::AIR;

        //duration of the initial (cast) animation
        d.f2 = (obj.AnimationCount() > 1) ? obj.AnimationDuration(1) : 0.f;

        //cast animation rendering pos
        d.source_pos = d.target_pos;
        
        //position offset for the icon rendering
        d.target_pos = glm::vec2(0.f, -1.f) + BUFF_ICON_SIZE * glm::vec2(-(buffIdx % BUFF_ROW_LENGTH), 1+(buffIdx / BUFF_ROW_LENGTH));

        //buff flag setup
        Unit* target = nullptr;
        if(d.targetID.type != ObjectType::UNIT || !obj.lvl()->objects.GetUnit(d.targetID, target)) {
            ENG_LOG_ERROR("UtilityObject::Buff - invalid target, buffs only work on units! ({})", d.targetID);
            throw std::runtime_error("UtilityObject::Buff - invalid target, buffs only work on units!");
        }
        target->SetEffectFlag(buffIdx, true);

        //unholy armor specific - health halving
        if(buffIdx == UnitEffectType::UNHOLY_ARMOR)
            target->SetHealth(target->Health() / 2);

        ENG_LOG_FINE("UtilityObject - Buff ({}) spawned at ({})", obj, *target);
    }

    bool UtilityHandler_Buff_Update(UtilityObject& obj, Level& level) {
        UtilityObject::LiveData& d = obj.LD();
        Input& input = Input::Get();

        obj.act() = 0;
        obj.ori() = 0;

        d.f1 += input.deltaTime;
        float t = d.f1 / obj.UData()->duration;
        int buffIdx = SpellID::Spell2Buff(d.i1);

        Unit* target = nullptr;
        if(d.targetID.type != ObjectType::UNIT || !obj.lvl()->objects.GetUnit(d.targetID, target)) {
            ENG_LOG_TRACE("UtilityObject::Buff - target lost ({}, {})", obj, d.targetID);
            return true;
        }

        obj.real_pos() = target->RenderPosition();
        obj.pos() = target->Position();

        //terminate prematurely if the flag was unset from some other source (ie. invisibility canceled by an action)
        if(!target->GetEffectFlag(buffIdx)) {
            ENG_LOG_FINE("UtilityObject - Buff ({}) forcefully removed from ({})", obj, *target);
            return true;
        }
        
        //remove the buff flag from the target once expired            
        if(t >= 1.f) {
            target->SetEffectFlag(buffIdx, false);
            return true;
        }
        return false;
    }

    void UtilityHandler_Buff_Render(UtilityObject& obj) {
        UtilityObject::LiveData& d = obj.LD();

        obj.act() = 0;
        obj.ori() = 0;

        bool target_airborne = bool(d.i2);
        if(!obj.lvl()->map.IsTileVisible(obj.Position()) || obj.lvl()->map.IsUntouchable(obj.Position(), target_airborne))
            return;

        //render the buff icon
        obj.RenderAt(obj.real_pos() - d.target_pos, BUFF_ICON_SIZE, Z_OFFSET);

        //render on spawn visuals (stop once the anim is over)
        if(d.f1 < d.f2) {
            obj.act() = 1;
            obj.RenderAt(d.source_pos - obj.real_size() * 0.5f, obj.real_size(), Z_OFFSET - 1e-3f);
            obj.act() = 0;
        }
    }

    void UtilityHandler_Heal_Init(UtilityObject& obj, FactionObject* src) {
        UtilityObject::LiveData& d = obj.LD();
        AnimatorDataRef anim = obj.Data()->animData;

        obj.real_pos() = d.target_pos;
        obj.real_size() = obj.Data()->size;

        d.f1 = d.f2 = 0.f;

        //target unit retrieval
        Unit* target = nullptr;
        Unit* source = dynamic_cast<Unit*>(src);
        if(d.targetID.type != ObjectType::UNIT || !obj.lvl()->objects.GetUnit(d.targetID, target) || source == nullptr) {
            ENG_LOG_ERROR("UtilityObject::Heal - invalid target, buffs only work on units! ({})", d.targetID);
            throw std::runtime_error("UtilityObject::Heal - invalid target, buffs only work on units!");
        }

        //do target check - terminate if unfriendly or orc
        if(src->lvl()->factions.Diplomacy().AreHostile(src->FactionIdx(), target->FactionIdx())) {
            ENG_LOG_TRACE("UtilityObject::Heal - attempting to heal an unfriendly unit ({})", *target);
            return;
        }

        //consumption computation
        int to_heal = target->MissingHealth();
        int max_heal = source->Mana() / obj.UData()->cost[3];
        to_heal = (to_heal > max_heal) ? max_heal : to_heal;

        if(to_heal == 0) {
            ENG_LOG_FINE("UtilityObject - nothing to heal (target={})", *target);
            return;
        }

        //subtract resources, increase target health
        int cost = to_heal * obj.UData()->cost[3];
        source->DecreaseMana(cost);
        target->AddHealth(to_heal);
        ENG_LOG_FINE("UtilityObject - Heal '{}' hitpoints for '{}' mana (target={})", to_heal, cost, *target);

        //lifespan timing (use animation duration)
        d.f2 = (obj.AnimationCount() > 1) ? obj.AnimationDuration(1) : 1.f;

        //play sound
        if(obj.UData()->on_spawn.valid)
            Audio::Play(obj.UData()->on_spawn.Random());
    }

    void UtilityHandler_Vision_Init(UtilityObject& obj, FactionObject* src) {
        UtilityObject::LiveData& d = obj.LD();
        AnimatorDataRef anim = obj.Data()->animData;

        obj.real_pos() = d.target_pos;
        obj.real_size() = obj.Data()->size;

        //lifespan timing
        d.f1 = 0.f;
        d.f2 = obj.UData()->duration;
        d.i1 = 0;

        //initialize the vision
        int radius = obj.UData()->i2;
        d.i3 = src->FactionIdx();
        obj.lvl()->map.VisibilityIncrement(glm::ivec2(d.target_pos), glm::ivec2(1), radius, d.i3);

        ENG_LOG_FINE("UtilityObject - Holy vision at {}", d.target_pos);
    }

    bool UtilityHandler_Vision_Update(UtilityObject& obj, Level& level) {
        UtilityObject::LiveData& d = obj.LD();

        if(UtilityHandler_Visuals_Update2(obj, level) && (d.i1 == 0)) {
            //disable the vision & terminate
            int radius = obj.UData()->i2;
            obj.lvl()->map.VisibilityDecrement(glm::ivec2(d.target_pos), glm::ivec2(1), radius, d.i3);
            d.i1 = 1;
            ENG_LOG_FINE("UtilityObject - Holy vision terminated at {}", d.target_pos);
        }

        return (d.i1 != 0);
    }

    void UtilityHandler_Minion_Init(UtilityObject& obj, FactionObject* src) {
        UtilityObject::LiveData& d = obj.LD();
        Level& level = *src->lvl();
        bool raise_dead = obj.UData()->b1;

        UnitDataRef minion_data = Resources::LoadUnit(raise_dead ? "misc/skeleton" : "misc/eye");

        if(raise_dead) {
            std::vector<UtilityObject*> corpses = {};
            int corpses_available = level.objects.GetCorpsesAround(d.target_pos, obj.UData()->i2, corpses);

            //cost computation & subtraction
            Unit* source = dynamic_cast<Unit*>(src);
            int max_count = source->Mana() / obj.UData()->cost[3];
            int raised = (corpses_available > max_count) ? max_count : corpses_available;

            if(raised == 0) {
                ENG_LOG_FINE("UtilityObject - Spawn a minion (skeleton) failed - no mana ({}) or no corpses", source->Mana());
                d.i1 = 0;
                d.f1 = (float)Input::CurrentTime();
                d.f2 = d.f1 + obj.UData()->duration;
                return;
            }

            source->DecreaseMana(raised * obj.UData()->cost[3]);

            UtilityObjectDataRef viz_data = obj.UData()->spawn_followup ? Resources::LoadUtilityObj(obj.UData()->followup_name) : nullptr;
            
            //unit spawning & corpse deletion
            for(int i = 0; i < raised; i++) {
                glm::ivec2 spawn_pos = corpses[i]->Position();
                d.ids[i] = level.objects.EmplaceUnit(level, minion_data, src->Faction(), spawn_pos);

                //alternative removal - could straightup mark for delete in the ObjectPool
                corpses[i]->Kill(true);

                //spawn spell visuals on each raised skeleton
                if(obj.UData()->spawn_followup)
                    level.objects.QueueUtilityObject(level, viz_data, glm::vec2(spawn_pos) + 0.5f);
            }
            d.i1 = raised;

            ENG_LOG_FINE("UtilityObject - Spawned a minion - skeletons, '{}/{}' around {} ({})", raised, corpses_available, d.target_pos, obj);
        }
        else {
            //pick a spawn position around the caster
            glm::ivec2 spawn_pos = d.target_pos;
            level.map.NearbySpawnCoords(src->Position(), glm::ivec2(1), 3, NavigationBit::AIR, spawn_pos);

            //spawn the unit
            d.ids[0] = level.objects.EmplaceUnit(level, minion_data, src->Faction(), spawn_pos);
            d.i1 = 1;

            ENG_LOG_FINE("UtilityObject - Spawned a minion - eye, at {} ({})", d.target_pos, obj);
        }

        //start the timer
        d.f1 = d.f2 = (float)Input::CurrentTime();
    }

    bool UtilityHandler_Minion_Update(UtilityObject& obj, Level& level) {
        UtilityObject::LiveData& d = obj.LD();
        Input& input = Input::Get();

        d.f2 += input.deltaTime;
        bool timer_expired = (d.f2 - d.f1) >= obj.UData()->duration;

        //kill spawned minions if timer expires
        if(timer_expired) {
            ENG_LOG_FINE("UtilityObject - Minions expired ({})", obj);
            for(int i = 0; i < d.i1; i++) {
                Unit* unit = nullptr;
                if(level.objects.GetUnit(d.ids[i], unit, false)) {
                    unit->Kill();
                }
            }
        }

        return timer_expired;
    }

    void UtilityHandler_Polymorph_Init(UtilityObject& obj, FactionObject* src) {
        UtilityObject::LiveData& d = obj.LD();
        
        //target verification
        Unit* target = nullptr;
        if(d.targetID.type != ObjectType::UNIT || !obj.lvl()->objects.GetUnit(d.targetID, target)) {
            ENG_LOG_ERROR("UtilityObject::Polymorph - invalid target, target must be an unit! ({})", d.targetID);
            throw std::runtime_error("UtilityObject::Polymorph - invalid target, target must be an unit!");
        }

        int nav = target->NavigationType();
        if((nav & (NavigationBit::GROUND | NavigationBit::AIR)) == 0) {
            ENG_LOG_ERROR("UtilityObject::Polymorph - invalid target, only works for AIR/GND units! ({})", nav);
            throw std::runtime_error("UtilityObject::Polymorph - invalid target, only works for AIR/GND units!");
        }

        bool sheepified = true;
        std::stringstream ss  = {};
        ss << *target;
        std::string target_id = ss.str();
        
        if(nav == NavigationBit::AIR && !src->lvl()->map(target->Position()).Traversable(NavigationBit::GROUND)) {
            //when there's nowhere to spawn the sheep - just silently remove the target unit
            target->Kill(true);
            sheepified = false;
        }
        else {
            //transform the target unit
            UnitDataRef sheep_data = Resources::LoadUnit("misc/sheep");
            target->Transform(sheep_data, src->lvl()->factions.Nature());
        }

        //call visuals initialization - to play the spell animation
        UtilityHandler_Visuals_Init(obj, src);
        ENG_LOG_FINE("UtilityObject - unit '{}' was polymorphed (sheep spawned: {}).", target_id, sheepified);
    }

    void UtilityHandler_FlameShield_Init(UtilityObject& obj, FactionObject* src) {
        UtilityObject::LiveData& d = obj.LD();

        obj.real_size() = obj.SizeScaled();

        d.f1 = d.f2 = (float)Input::CurrentTime();

        ENG_LOG_FINE("UtilityObject - Flame Shield effect applied to '{}'.", d.targetID);
    }

    bool UtilityHandler_FlameShield_Update(UtilityObject& obj, Level& level) {
        UtilityObject::LiveData& d = obj.LD();
        UtilityObjectData& ud = *obj.UData();

        //fetch the central unit
        Unit* target = nullptr;
        if(!level.objects.GetUnit(d.targetID, target)) {
            ENG_LOG_FINE("UtilityObject - Flame Shield effect expired from '{}' (central unit lost).", d.targetID);
            return true;
        }

        obj.real_pos() = target->RenderPosition() + target->RenderSize() * 0.5f;
        obj.pos() = target->Position();

        d.f2 += Input::Get().deltaTime;

        if((d.f2 - d.f1) >= obj.UData()->duration) {
            d.f1 += obj.UData()->duration;

            //apply damage to objects around
            auto [hit_count, total_damage] = ApplyDamage_Splash(level, ud.i3, ud.i4, obj.pos(), ud.i2, d.targetID, false);
            ENG_LOG_FINE("UtilityObject - Flame Shield tick - Damaged {} objects ({} damage in total).", hit_count, total_damage);
            
            //increment tick counter & possibly terminate
            if((++d.i1) >= ud.i5) {
                ENG_LOG_FINE("UtilityObject - Flame Shield effect expired from '{}'.", d.targetID);
                return true;
            }
        }
        
        return false;
    }

    void UtilityHandler_FlameShield_Render(UtilityObject& obj) {
        if(!obj.lvl()->map.IsWithinBounds(obj.real_pos()) || !obj.lvl()->map.IsTileVisible(obj.real_pos()))
            return;

        UtilityObject::LiveData& d = obj.LD();
        float speed = obj.UData()->f1;

        float f = 2.f*glm::pi<float>() / FLAME_SHIELD_NUM_FLAMES;
        for(int i = 0; i < FLAME_SHIELD_NUM_FLAMES; i++) {
            glm::vec2 offset = glm::vec2(std::cosf(d.f2*speed + i*f), std::sinf(d.f2*speed + i*f));
            obj.RenderAt(obj.real_pos() - obj.real_size()*0.5f + offset, obj.real_size(), Z_OFFSET);
        }
    }

    void UtilityHandler_Tornado_Init(UtilityObject& obj, FactionObject* src) {
        UtilityObject::LiveData& d = obj.LD();

        obj.real_pos() = d.target_pos;
        obj.real_size() = obj.SizeScaled();
        obj.pos() = d.target_pos;

        d.source_pos = d.target_pos;
        d.f1 = d.f2 = (float)Input::CurrentTime();
        d.f3 = -1.f;

        ENG_LOG_FINE("UtilityObject - Tornado spawned at {}.", d.target_pos);
    }

    bool UtilityHandler_Tornado_Update(UtilityObject& obj, Level& level) {
        UtilityObject::LiveData& d = obj.LD();
        UtilityObjectData& ud = *obj.UData();
        float deltaTime = Input::Get().deltaTime;

        //d.target_pos = last movement's direction vector
        //d.source_pos = movement interpolation position
        //obj.pos() = current position (map coords)

        if(d.f3 >= 1.f) {
            d.source_pos = glm::vec2(obj.pos());
            d.f3 = -1.f;
        }

        if(d.f3 >= 0.f) {
            //tornado in motion
            d.f3 += ud.f1 * deltaTime;
            d.source_pos = glm::vec2(obj.pos()) - d.target_pos * (1.f - d.f3);
        }
        else if(Random::Uniform() < TORNADO_MOTION_PROBABILITY) {
            //initiate new motion
            glm::ivec2 dir;
            do {
                dir = DirectionVector(int(9.f*Random::Uniform()));
            } while(!level.map.IsWithinBounds(obj.pos() + dir));

            d.target_pos = dir;
            obj.pos() += dir;
            d.f3 = 0.f;
        }
        // ENG_LOG_INFO("POS: {}, {}, {}", obj.pos(), d.source_pos, d.f3);

        obj.real_pos() = d.source_pos + 0.5f;

        d.f2 += deltaTime;
        if((d.f2 - d.f1) >= obj.UData()->duration) {
            d.f1 += obj.UData()->duration;

            //apply damage to objects around
            auto [hit_count, total_damage] = ApplyDamage_Splash(level, ud.i3, ud.i4, obj.pos(), ud.i2, ObjectID(), false);
            ENG_LOG_FINE("UtilityObject - Tornado tick - Damaged {} objects ({} damage in total).", hit_count, total_damage);
            
            //increment tick counter & possibly terminate
            if((++d.i1) >= ud.i5) {
                ENG_LOG_FINE("UtilityObject - Tornado terminated.");
                return true;
            }
        }
        
        return false;
    }

    void UtilityHandler_Runes_Init(UtilityObject& obj, FactionObject* src) {
        UtilityObject::LiveData& d = obj.LD();
        UtilityObjectData& ud = *obj.UData();

        obj.pos() = d.target_pos;
        obj.real_size() = obj.SizeScaled();

        d.f1 = d.f2 = (float)Input::CurrentTime();
        d.f3 = ud.f1;

        obj.lvl()->map.SpawnRunes(d.target_pos, obj.ID());

        ENG_LOG_FINE("UtilityObject - Runes spawned at {}.", d.target_pos);
    }

    bool UtilityHandler_Runes_Update(UtilityObject& obj, Level& level) {
        UtilityObject::LiveData& d = obj.LD();
        UtilityObjectData& ud = *obj.UData();

        //spell duration timer
        d.f2 += Input::Get().deltaTime;
        if(d.f2 - d.f1 >= ud.duration) {
            int leftovers = level.map.DespawnRunes(d.target_pos, obj.ID());
            ENG_LOG_FINE("UtilityObject - Runes despawned at {} ({} remained).", d.target_pos, leftovers);
            return true;
        }

        //animation timer
        d.f3 += Input::Get().deltaTime;
        if(d.f3 >= ud.f1) {
            d.i1 = 1;
            d.f3 = 0.f;
            //animation frame reset
            obj.SwitchAnimatorAction(0, true);
        }

        return false;
    }

    void RenderRune(UtilityObject& obj, const glm::ivec2& pos) {
        if(!obj.lvl()->map.IsWithinBounds(pos) || !obj.lvl()->map.IsTileVisible(pos) || obj.lvl()->map(pos).rune_id != obj.ID())
            return;
        
        obj.RenderAt(glm::vec2(pos) - obj.real_size() * 0.5f + 0.5f, obj.real_size(), Z_OFFSET);
    }

    void UtilityHandler_Runes_Render(UtilityObject& obj) {
        //only play if signaled from the Update() logic
        if(obj.LD().i1 == 0)
            return;
        
        Level& level = *obj.lvl();
        RenderRune(obj, obj.pos() + glm::ivec2(+0,+0));
        RenderRune(obj, obj.pos() + glm::ivec2(-1,+0));
        RenderRune(obj, obj.pos() + glm::ivec2(+1,+0));
        RenderRune(obj, obj.pos() + glm::ivec2(+0,-1));
        RenderRune(obj, obj.pos() + glm::ivec2(+0,+1));

        //stops the animation after playing once
        if(obj.AnimKeyframeSignal()) {
            obj.LD().i1 = 0;
        }
    }

    //Spreads provided damage among given targets with varying health.
    //Attempts to do it evenly. When health cap is reached, spreads the leftover damage among the remaining units.
    //Returns the total damage spread out.
    //Also, health values in the vector are overriden and now represent the damage for given unit.
    int spread_dmg_evenly(std::vector<std::pair<Unit*,int>>& targets, int max_damage) {
        if(targets.size() == 0)
            return 0;

        std::sort(targets.begin(), targets.end(), [](auto& lhs, auto& rhs) { return lhs.second < rhs.second; });

        int i = 0;

        //skip entries with no capacity
        while(targets[i].second == 0) { i++; }

        //fill slots from left to right without leftovers
        //meaning that when assigning v[i] = x, there's enough value to assign x to all the slots to the right as well
        int used = 0;
        int size, thresh;
        do {
            size = targets.size() - i;
            thresh = (max_damage-used) / size;

            while(i < targets.size() && targets[i].second <= thresh) {
                used += targets[i++].second;
            }
        } while(i < targets.size() && thresh >= targets[i].second);

        //spread out the remainder, when there's no longer enough value to spread it evenly among the slots
        int leftover = max_damage-used-thresh*size;
        while(i < targets.size() && leftover > 0) {
            targets[i++].second = thresh + 1;
            leftover--;
            used += thresh + 1;
        }

        //cap the remaining slots
        while(i < targets.size()) {
            targets[i++].second = thresh;
            used += thresh;
        }

        return used;
    }

    void UtilityHandler_ExoCoil_Init(UtilityObject& obj, FactionObject* src) {
        UtilityObject::LiveData& d = obj.LD();
        UtilityObjectData& ud = *obj.UData();
        Level& level = *obj.lvl();

        int spell_damage = ud.i4;

        Unit* source = static_cast<Unit*>(src);
        UtilityObjectDataRef followup = nullptr;
        if(ud.spawn_followup) {
            followup = Resources::LoadUtilityObj(ud.followup_name);
        }

        if(followup == nullptr) {
            ENG_LOG_ERROR("UtilityObject - Exo/Coil requires followup object!");
            return;
        }

        //mana & damage computation
        int max_damage = ud.b1 ? (spell_damage * (source->Mana() / ud.cost[3])) : spell_damage;

        if(max_damage <= 0) {
            ENG_LOG_FINE("UtilityObject - Exo/Coil - not enough resources (damage={})", max_damage);
            return;
        }

        //scan the area for targets
        std::vector<ObjectID> ids = level.map.EnemyUnitsInArea(level.factions.Diplomacy(), d.target_pos, ud.i2, src->FactionIdx());
        std::vector<std::pair<Unit*,int>> targets;
        for(ObjectID& id : ids) {
            Unit& target = level.objects.GetUnit(id);

            //exorcism can only target undeads
            if(!ud.b1 || target.IsUndead()) {
                targets.push_back({&target, target.Health()});
            }
        }

        if(targets.size() == 0) {
            ENG_LOG_FINE("UtilityObject - Exo/Coil - no targets.");
            return;
        }

        //divide damage among the targets
        int damage = spread_dmg_evenly(targets, max_damage);

        UtilityObject::LiveData init_data = {};
        init_data.i1 = 1;

        //spawn a followup for each (damaged passed through target_pos param)
        for(auto& [target, dmg] : targets) {
            //exorcism - also deal damage (followup is just visuals) ... coil spawns a guided projectile
            if(ud.b1) {
                target->ApplyDamageFlat(dmg);
            }
            init_data.i3 = dmg;
            level.objects.QueueUtilityObject(level, followup, target->Positionf() + 0.5f, target->OID(), init_data, *src);
        }

        if(ud.b1) {
            //subtract the mana for exorcism
            source->DecreaseMana((ud.cost[3] * damage) / spell_damage);
        }
        else {
            //heal for damage dealt if not flag1
            source->AddHealth(spell_damage);
        }

        //play sound
        if(ud.on_spawn.valid)
            Audio::Play(ud.on_spawn.Random());

        ENG_LOG_FINE("UtilityObject - Exo/Coil wrapper spawned ({} targets, {} total damage dealt).", targets.size(), damage);
    }

    std::string stringify_dnd_pattern(const std::array<ObjectID,5>& ids) {
        int n = 0;
        for(int i = 0; i < 5; i++) {
            n |= ids[i].type*5+ids[i].idx;
        }
        char buf[64];
        for(int i = 0; i < 25; i++) {
            buf[i] = ((n & (1 << i)) != 0) ? '1' : '0';
        }
        buf[26] = '\0';
        return std::string(buf);
    }

    void UtilityHandler_BlizzDnD_Init(UtilityObject& obj, FactionObject* src) {
        UtilityObject::LiveData& d = obj.LD();
        UtilityObjectData& ud = *obj.UData();

        //pattern setup
        int i = 0;
        while(i < 5) {
            bool duplicate = false;

            //storing the position as objectIDs in the live data
            ObjectID pos = ObjectID(Random::UniformInt(5), Random::UniformInt(5), 0);
            for(int j = 0; j < i; j++) {
                if(d.ids[i] == pos) {
                    duplicate = true;
                    break;
                }
            }

            if(!duplicate) {
                d.ids[i++] = pos;
            }
        }

        d.f1 = d.f2 = (float)Input::CurrentTime();

        ENG_LOG_FINE("UtilityObject - Blizzard/DnD spawned at {} (pattern: {}).", d.target_pos, stringify_dnd_pattern(d.ids));
        d.target_pos -= 2.f;
    }

    bool UtilityHandler_BlizzDnD_Update(UtilityObject& obj, Level& level) {
        UtilityObject::LiveData& d = obj.LD();
        UtilityObjectData& ud = *obj.UData();
        float deltaTime = Input::Get().deltaTime;

        d.f2 += deltaTime;
        if((d.f2 - d.f1) >= obj.UData()->duration) {
            d.f1 += obj.UData()->duration;

            UtilityObject::LiveData init_data = {};
            init_data.sourceID = d.sourceID;

            UtilityObjectDataRef followup = nullptr;
            if(ud.spawn_followup) {
                followup = Resources::LoadUtilityObj(ud.followup_name);
            }

            if(followup == nullptr) {
                ENG_LOG_ERROR("UtilityObject - Blizzard/DnD requires followup object!");
                return true;
            }

            glm::vec2 start_offset = glm::vec2(ud.i2, ud.i3);

            //spawn the next salvo
            for(int i = 0; i < 5; i++) {
                init_data.target_pos = d.target_pos + glm::vec2(d.ids[i].type, d.ids[i].idx);
                init_data.source_pos = init_data.target_pos - start_offset;

                level.objects.QueueUtilityObject(level, followup, init_data, nullptr);
            }
            ENG_LOG_FINE("UtilityObject - Blizzard/DnD tick {}.", obj);
            
            //increment tick counter & possibly terminate
            if((++d.i1) >= ud.i5) {
                ENG_LOG_FINE("UtilityObject - Blizzard/DnD spawner {} terminated.", obj);
                return true;
            }
        }
        
        return false;
    }

    void UtilityHandler_Demolish_Init(UtilityObject& obj, FactionObject* src) {
        UtilityObject::LiveData& d = obj.LD();
        UtilityObjectData& ud = *obj.UData();

        if(src->NumID()[1] != UnitType::DEMOSQUAD) {
            ENG_LOG_ERROR("UtilityObject - Demolish only works with demosquad!");
            return;
        }

        src->Kill();
        auto [hit_count, damage] = ApplyDamageFlat_Splash(*obj.lvl(), ud.i4, d.target_pos, ud.i2, src->OID(), false);

        ENG_LOG_FINE("UtilityObject - Demolish spawned at {} ({} objects hit, {} damage dealt).", d.target_pos, hit_count, damage);
    }

}//namespace eng
