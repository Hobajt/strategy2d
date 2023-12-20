#include "engine/game/utility_handlers.h"

#include "engine/core/input.h"
#include "engine/game/gameobject.h"
#include "engine/game/level.h"

#include <tuple>

#define Z_OFFSET -0.1f

namespace eng {

    void UtilityHandler_Default_Render(UtilityObject& obj);

    void UtilityHandler_Projectile_Init(UtilityObject& obj, FactionObject& src);
    bool UtilityHandler_Projectile_Update(UtilityObject& obj, Level& level);

    void UtilityHandler_Corpse_Init(UtilityObject& obj, FactionObject& src);
    bool UtilityHandler_Corpse_Update(UtilityObject& obj, Level& level);
    void UtilityHandler_Corpse_Render(UtilityObject& obj);

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
            default:
                ENG_LOG_ERROR("UtilityObject - invalid utility type ID ({})", id);
                throw std::runtime_error("");
        }
    }

    //===================================================================

    void UtilityHandler_Default_Render(UtilityObject& obj) {
        if(!obj.lvl()->map.IsTileVisible(obj.real_pos()))
            return;
        obj.RenderAt(obj.real_pos(), obj.real_size(), Z_OFFSET);
    }

    void UtilityHandler_Projectile_Init(UtilityObject& obj, FactionObject& src) {
        UtilityObject::LiveData& d = obj.LD();

        //set projectile orientation, start & end time, starting position and copy unit's damage values
        glm::vec2 target_dir = d.target_pos - d.source_pos;
        d.i1 = VectorOrientation(target_dir / glm::length(target_dir));
        d.f1 = d.f2 = (float)Input::CurrentTime();
        obj.real_size() = obj.SizeScaled();
        obj.real_pos() = glm::vec2(src.Position()) + 0.5f - obj.real_size() * 0.5f;
        d.i2 = src.BasicDamage();
        d.i3 = src.PierceDamage();
        // ENG_LOG_INFO("PROJECTILE PTS: {}, {}", d.source_pos, d.target_pos);
    }

    bool UtilityHandler_Projectile_Update(UtilityObject& obj, Level& level) {
        UtilityObject::LiveData& d = obj.LD();
        Input& input = Input::Get();

        obj.act() = 0;
        obj.ori() = d.i1;

        d.f2 += input.deltaTime;
        float t = (d.f2 - d.f1) / obj.UData()->duration;

        //adding sizes cuz rendering uses Quad::FromCorner
        obj.real_pos() = d.InterpolatePosition(t) + 0.5f - obj.real_size() * 0.5f;
        if(t >= 1.f) {
            bool attack_happened = ApplyDamage(level, d.i2, d.i3, d.targetID, d.target_pos);
            return true;
        }

        return false;
    }

    void UtilityHandler_Corpse_Init(UtilityObject& obj, FactionObject& src) {
        UtilityObject::LiveData& d = obj.LD();
        AnimatorDataRef anim = obj.Data()->animData;

        //store object's orientation
        d.i4 = src.Orientation();
        //TODO: maybe round up the orientation, as there are usually only 2 directions for dying animations

        obj.real_pos() = glm::vec2(src.Position());
        obj.real_size() = obj.Data()->size;

        //1st & 2nd anim indices
        d.i1 = src.DeathAnimIdx();
        d.i2 = -1;

        if(d.i1 < 0) {
            //1st anim has invalid idx -> play explosion animation
            d.i3 = 1;
            d.f2 = Input::GameTimeDelay(anim->GetGraphics(CorpseAnimID::EXPLOSION).Duration());

            if(!src.IsUnit()) {
                //src is a building -> display crater animation (along with explosion)
                int is_wasteland = int(obj.lvl()->map.GetTileset()->GetType() == TilesetType::WASTELAND);
                d.i1 = (src.NavigationType() == NavigationBit::GROUND) ? (CorpseAnimID::RUINS_GROUND + is_wasteland) : CorpseAnimID::RUINS_WATER;
                d.f1 = Input::GameTimeDelay(anim->GetGraphics(d.i1).Duration());

                obj.real_pos() = glm::vec2(src.Position()) + (src.Data()->size - glm::vec2(2.f)) * 0.5f;
                obj.real_size() = glm::vec2(2.f);
            }
        }
        else {
            d.f1 = Input::GameTimeDelay(anim->GetGraphics(d.i1).Duration() + 1.f);
            if(src.IsUnit()) {
                //2nd animation - transitioned to from the 1st one
                if(src.NavigationType() == NavigationBit::GROUND) {
                    d.i2 = CorpseAnimID::CORPSE1_HU + src.Race();
                    d.f2 =  anim->GetGraphics(d.i2).Duration();
                }
                else if(src.NavigationType() == NavigationBit::WATER) {
                    d.i2 = CorpseAnimID::CORPSE_WATER;
                    d.f2 = anim->GetGraphics(d.i2).Duration();
                }
            }
        }

        //visibility setup (copy object's original vision range)
        d.i5 = src.VisionRange();
        d.i6 = src.FactionIdx();
        src.lvl()->map.VisibilityIncrement(obj.real_pos(), obj.real_size(), d.i5, d.i6);

        // ENG_LOG_TRACE("CORPSE OBJECT - INIT: {}, {}, {}, {}, {}", d.i1, d.i2, d.i3, d.f1, d.f2);
    }

    bool UtilityHandler_Corpse_Update(UtilityObject& obj, Level& level) {
        UtilityObject::LiveData& d = obj.LD();
        Input& input = Input::Get();

        float t = (float)Input::CurrentTime();

        //switch from 1st to 2nd animation when the time runs out
        if(t >= d.f1) {
            d.i1 = d.i2;
            d.f1 = t + d.f2;
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

        //terminate explosion animation
        if(d.i3 && t >= d.f2) {
            d.i3 = 0;
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

}//namespace eng
