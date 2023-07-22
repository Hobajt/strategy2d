#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>

#include "engine/utils/mathdefs.h"

#include "engine/core/sprite.h"
#include "engine/core/quad.h"

namespace eng {

    //===== AnimatorData =====

    class AnimatorData {
    public:
        AnimatorData() = default;
        AnimatorData(const std::string& name, const std::map<int, SpriteGroup>& anims);

        std::string Name() const { return name; }

        SpriteGroup& GetGraphics(int action);
        int ActionCount() const { return anims.size(); }
    private:
        std::map<int, SpriteGroup> anims;
        std::string name;
    };
    using AnimatorDataRef = std::shared_ptr<AnimatorData>;

    //===== Animator =====

    //Handles animation switching logic, contains the animation state.
    class Animator {
    public:
        Animator() = default;
        Animator(const AnimatorDataRef& data);

        //Updates animation switching logic
        bool Update(int action);

        void Render(const glm::vec3& screen_pos, const glm::vec2& screen_size, int action, int orientation, const glm::uvec4& info = glm::uvec4(QuadType::Animator,0,0,0));
        void RenderAlt(const glm::vec3& screen_pos, const glm::vec2& screen_size, const glm::vec4& color, bool noTexture, int action, int orientation, const glm::uvec4& info = glm::uvec4(QuadType::Animator,0,0,0));

        void SetPaletteIdx(float paletteIdx_) { paletteIdx = paletteIdx_; }

        void SetFrameIdx(int action, int idx);

        int ActionCount() const { return data->ActionCount(); }
        SpriteGroup& GetGraphics(int action) { return data->GetGraphics(action); }

        int GetCurrentFrameIdx() const;
        int GetCurrentFrameCount() const;
        float GetCurrentFrame() const { return frame; }

        bool KeyframeSignal() const;
    private:
        AnimatorDataRef data = nullptr;
        float frame = 0;
        int lastAction = 0;

        float paletteIdx = -1.f;
    };

}//namespace eng
