#include "engine/game/utility_handlers.h"

#include "engine/core/input.h"
#include "engine/game/gameobject.h"
#include "engine/game/level.h"
#include "engine/game/resources.h"

#include <tuple>

#define Z_OFFSET -0.1f

namespace eng {

    void UtilityHandler_Default_Render(UtilityObject& obj);

    void UtilityHandler_Projectile_Init(UtilityObject& obj, FactionObject* src);
    bool UtilityHandler_Projectile_Update(UtilityObject& obj, Level& level);

    void UtilityHandler_Corpse_Init(UtilityObject& obj, FactionObject* src);
    bool UtilityHandler_Corpse_Update(UtilityObject& obj, Level& level);
    void UtilityHandler_Corpse_Render(UtilityObject& obj);

    void UtilityHandler_Visuals_Init(UtilityObject& obj, FactionObject* src);
    bool UtilityHandler_Visuals_Update(UtilityObject& obj, Level& level);

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
        d.i2 = src->BasicDamage();
        d.i3 = src->PierceDamage();

        ENG_LOG_FINE("UtilityObject - Projectile spawned (guided={}, from {} to {})", obj.UData()->Projectile_IsAutoGuided(), d.source_pos, d.target_pos);
    }

    bool UtilityHandler_Projectile_Update(UtilityObject& obj, Level& level) {
        UtilityObject::LiveData& d = obj.LD();
        Input& input = Input::Get();

        obj.act() = 0;
        obj.ori() = d.i1;

        d.f2 += input.deltaTime;
        float t = (d.f2 - d.f1) / obj.UData()->duration;

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

}//namespace eng
