#include "engine/core/animator.h"

#include "engine/core/input.h"
#include "engine/utils/randomness.h"

#define WOBBLING_OFFSET 5e-3f

namespace eng {

    //===== AnimatorData =====

    static SpriteGroup no_anim = SpriteGroup::CreateDefault();

    AnimatorData::AnimatorData(const std::string& name_, const std::map<int, SpriteGroup>& anims_) : name(name_), anims(anims_) {}

    SpriteGroup& AnimatorData::GetGraphics(int action) {
        if(anims.count(action)) {
            return anims.at(action);
        }
        else {
            //TODO: AnimatorData - invalid actionID - add some defaulting mechanism
            if(anims.size() != 0)
                return anims.begin()->second;
            else
                return no_anim;
        }
    }

    void AnimatorData::AddAction(int actionIdx, const SpriteGroup& graphics) {
        anims.insert({ actionIdx, graphics });
    }

    //===== Animator =====

    Animator::Animator(const AnimatorDataRef& data_, float anim_speed_) : data(data_), anim_speed(anim_speed_), frame(Random::Uniform()) {}

    void Animator::SwitchAction(int action, bool forceReset) {
        if(lastAction != action || forceReset) {
            frame = data->GetGraphics(action).FirstFrameF();
        }
        lastAction = action;
    }

    bool Animator::Update(int action) {
        SpriteGroup& graphics = data->GetGraphics(action);

        SwitchAction(action);

        frame += Input::Get().deltaTime * anim_speed;
        bool res = false;
        while(frame >= graphics.Duration() && graphics.Repeat()) {
            frame -= graphics.Duration();
            res = true;
        }
        return res || (frame >= graphics.Duration());
    }

    void Animator::Render(const glm::vec3& screen_pos, const glm::vec2& screen_size, int action, int orientation, const glm::uvec4& info) {
        SpriteGroup& graphics = data->GetGraphics(action);
        int frameIdx = graphics.FrameIdx(frame);
        glm::vec3 wobble = (graphics.Wobble() && (frame > graphics.Duration() * 0.5f)) ? glm::vec3(0.f, WOBBLING_OFFSET, 0.f) : glm::vec3(0.f);
        graphics.RenderAnim(screen_pos + wobble, screen_size, info, frameIdx, orientation, paletteIdx);
    }

    void Animator::Render(const glm::vec3& screen_pos, const glm::vec2& screen_size, int action, int orientation, float frame, const glm::uvec4& info) {
        SpriteGroup& graphics = data->GetGraphics(action);
        int frameIdx = graphics.FrameIdx(frame);
        glm::vec3 wobble = (graphics.Wobble() && (frame > graphics.Duration() * 0.5f)) ? glm::vec3(0.f, WOBBLING_OFFSET, 0.f) : glm::vec3(0.f);
        graphics.RenderAnim(screen_pos + wobble, screen_size, info, frameIdx, orientation, paletteIdx);
    }

    void Animator::RenderAlt(const glm::vec3& screen_pos, const glm::vec2& screen_size, const glm::vec4& color, bool noTexture, int action, int orientation, const glm::uvec4& info) {
        SpriteGroup& graphics = data->GetGraphics(action);
        int frameIdx = graphics.FrameIdx(frame);
        graphics.RenderAnimAlt(screen_pos, screen_size, info, color, noTexture, frameIdx, orientation, paletteIdx);
    }

    void Animator::SetFrameIdx(int action, int frameIdx) {
        SpriteGroup& graphics = data->GetGraphics(action);
        frame = (graphics.Duration() * frameIdx) / graphics.FrameCount();
    }

    int Animator::GetCurrentFrameIdx() const {
        return data->GetGraphics(lastAction).FrameIdx(frame);
    }

    int Animator::GetCurrentFrameCount() const {
        return data->GetGraphics(lastAction).FrameCount();
    }

    float Animator::GetAnimationDuration(int actionIdx) const {
        return data->GetGraphics(actionIdx).Duration();
    }

    bool Animator::KeyframeSignal() const {
        SpriteGroup& graphics = data->GetGraphics(lastAction);
        return frame >= (graphics.Keyframe() * graphics.Duration());
    }

    bool Animator::KeyframeSignal(int actionIdx) const {
        ASSERT_MSG(((unsigned int)actionIdx < data->ActionCount()), "Animator::KeyframeSignal - actionIdx out of bounds ({}, max {})", actionIdx, data->ActionCount());
        SpriteGroup& graphics = data->GetGraphics(actionIdx);
        return frame >= (graphics.Keyframe() * graphics.Duration());
    }

    void Animator::DBG_Print(int actionIdx) const {
        if(actionIdx == -1) actionIdx = lastAction;
        SpriteGroup& graphics = data->GetGraphics(actionIdx);
        ENG_LOG_INFO("FRAME: {}, DURATION: {}, KEYFRAME: {}, ACTION: {} (LAST: {})", frame, graphics.Duration(), graphics.Keyframe(), actionIdx, actionIdx == lastAction);
    }

}//namespace eng
