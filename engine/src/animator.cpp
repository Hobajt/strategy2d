#include "engine/core/animator.h"

#include "engine/core/input.h"

namespace eng {

    //===== AnimatorData =====

    AnimatorData::AnimatorData(const std::string& name_, const std::map<int, SpriteGroup>& anims_) : name(name_), anims(anims_) {}

    SpriteGroup& AnimatorData::GetGraphics(int action) {
        if(anims.count(action)) {
            return anims.at(action);
        }
        else {
            //TODO: AnimatorData - invalid actionID - add some defaulting mechanism
            return anims.begin()->second;
        }
    }

    void AnimatorData::AddAction(int actionIdx, const SpriteGroup& graphics) {
        anims.insert({ actionIdx, graphics });
    }

    //===== Animator =====

    Animator::Animator(const AnimatorDataRef& data_, float anim_speed_) : data(data_), anim_speed(anim_speed_) {}

    bool Animator::Update(int action) {
        SpriteGroup& graphics = data->GetGraphics(action);

        if(lastAction != action) {
            frame = graphics.FirstFrameF();
        }
        lastAction = action;

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
        graphics.RenderAnim(screen_pos, screen_size, info, frameIdx, orientation, paletteIdx);
    }

    void Animator::Render(const glm::vec3& screen_pos, const glm::vec2& screen_size, int action, int orientation, float frame, const glm::uvec4& info) {
        SpriteGroup& graphics = data->GetGraphics(action);
        int frameIdx = graphics.FrameIdx(frame);
        graphics.RenderAnim(screen_pos, screen_size, info, frameIdx, orientation, paletteIdx);
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

    bool Animator::KeyframeSignal() const {
        SpriteGroup& graphics = data->GetGraphics(lastAction);
        return frame >= graphics.Keyframe() * graphics.Duration();
    }

}//namespace eng
