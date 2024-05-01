#include "engine/game/utility_handlers.h"

#include "engine/core/input.h"
#include "engine/game/gameobject.h"
#include "engine/game/level.h"
#include "engine/game/resources.h"
#include "engine/core/audio.h"

#include <tuple>

#define Z_OFFSET -0.1f

static constexpr glm::vec2 BUFF_ICON_SIZE = glm::vec2(4.f / 9.f);
static constexpr int BUFF_ROW_LENGTH = 4;

namespace eng {

    void UtilityHandler_Default_Render(UtilityObject& obj);

    void UtilityHandler_Projectile_Init(UtilityObject& obj, FactionObject* src);
    bool UtilityHandler_Projectile_Update(UtilityObject& obj, Level& level);

    void UtilityHandler_Corpse_Init(UtilityObject& obj, FactionObject* src);
    bool UtilityHandler_Corpse_Update(UtilityObject& obj, Level& level);
    void UtilityHandler_Corpse_Render(UtilityObject& obj);

    void UtilityHandler_Visuals_Init(UtilityObject& obj, FactionObject* src);
    bool UtilityHandler_Visuals_Update(UtilityObject& obj, Level& level);

    void UtilityHandler_Buff_Init(UtilityObject& obj, FactionObject* src);
    bool UtilityHandler_Buff_Update(UtilityObject& obj, Level& level);
    void UtilityHandler_Buff_Render(UtilityObject& obj);

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
                //TODO: gonna have to further distinguish among the various spells
                std::tie(data->Init, data->Update, data->Render) = handlerRefs{ UtilityHandler_Visuals_Init, UtilityHandler_Visuals_Update, UtilityHandler_Default_Render };
                break;
            case UtilityObjectType::BUFF:
                //TODO: gonna have to further distinguish among the various spells
                std::tie(data->Init, data->Update, data->Render) = handlerRefs{ UtilityHandler_Buff_Init, UtilityHandler_Buff_Update, UtilityHandler_Buff_Render };
                break;
            default:
                ENG_LOG_ERROR("UtilityObject - ResolveUtilityHandlers - invalid utility type ID ({})", id);
                throw std::runtime_error("");
        }
    }

    //===================================================================

    void UtilityHandler_Default_Render(UtilityObject& obj) {
        if(!obj.lvl()->map.IsTileVisible(obj.real_pos()))
            return;
        obj.RenderAt(obj.real_pos() - obj.real_size() * 0.5f, obj.real_size(), Z_OFFSET);
    }

    void UtilityHandler_Projectile_Init(UtilityObject& obj, FactionObject* src) {
        UtilityObject::LiveData& d = obj.LD();
        UtilityObjectData& ud = *obj.UData();

        if(src == nullptr) {
            ENG_LOG_ERROR("UtilityHandler::Init - handler requires a valid pointer to the source object.");
            throw std::runtime_error("");
        }

        //set projectile orientation, start & end time, starting position and copy unit's damage values
        glm::vec2 target_dir = d.target_pos - d.source_pos;
        d.i1 = VectorOrientation(target_dir / glm::length(target_dir));
        d.f1 = d.f2 = (float)Input::CurrentTime();
        obj.real_size() = obj.SizeScaled();
        obj.real_pos() = src->PositionCentered();

        //projectile damage setup (use caster's stats if not set in object data)
        if(ud.i3 + ud.i4 == 0) {
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
        d.f3 = 1.f / obj.UData()->duration;

        ENG_LOG_FINE("UtilityObject - Projectile spawned (guided={}, from {} to {})", obj.UData()->Projectile_IsAutoGuided(), d.source_pos, d.target_pos);
    }

    glm::vec2 NextBouncePos(const glm::vec2& source, const glm::vec2& target);

    bool UtilityHandler_Projectile_Update(UtilityObject& obj, Level& level) {
        UtilityObject::LiveData& d = obj.LD();
        Input& input = Input::Get();

        obj.act() = 0;
        obj.ori() = d.i1;

        d.f2 += input.deltaTime;
        float t = (d.f2 - d.f1) * d.f3;

        //adding sizes cuz rendering uses Quad::FromCorner
        obj.real_pos() = d.InterpolatePosition(t);
        if(t >= 1.f) {
            //damage application from the projectile
            if(obj.UData()->Projectile_IsAutoGuided())
                ApplyDamage(level, d.i2, d.i3, d.targetID, d.target_pos);
            else
                ApplyDamage_Splash(level, d.i2, d.i3, d.target_pos, obj.UData()->Projectile_SplashRadius());

            //optional spawning of followup object
            if(obj.UData()->spawn_followup) {
                UtilityObjectDataRef data =  Resources::LoadUtilityObj(obj.UData()->followup_name);
                if(data != nullptr) {
                    level.objects.QueueUtilityObject(level, data, d.target_pos);
                }
                else {
                    ENG_LOG_WARN("UtilityHandler_Projectile_Update - invalid followup object ('{}')", obj.UData()->followup_name);
                }
            }

            //projectile "bouncing"
            if(obj.UData()->b1 && d.i4 > 0) {
                //start next bounce instead of terminating
                d.i4--;                             //decrease the bounce counter
                glm::vec2 dir = glm::normalize(d.target_pos - d.source_pos);
                d.source_pos = d.target_pos;        //setup new target position
                d.target_pos = d.target_pos + dir;
                d.f1 = d.f2 = (float)Input::CurrentTime();
                d.f3 = 1.f / 0.25f;    //update duration for the bounces

                //manually play the on done sound, since it normally only spawns when object dies
                if(obj.UData()->on_done.valid)
                    Audio::Play(obj.UData()->on_done.Random(), d.source_pos);

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

        obj.real_pos() = glm::vec2(src->Position());
        obj.real_size() = obj.Data()->size;

        //1st & 2nd anim indices
        d.i1 = src->DeathAnimIdx();
        d.i2 = -1;

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
            if(src->IsUnit()) {
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
        obj.real_size() = obj.Data()->size;

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

}//namespace eng
