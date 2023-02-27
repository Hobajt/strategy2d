#pragma once

#include <vector>
#include <utility>

#include "engine/utils/mathdefs.h"

namespace eng {

    //===== AABB =====

    //Axis-aligned bounding box.
    struct AABB {
        union {
            struct {
                glm::vec2 min;
                glm::vec2 max;
            };
            glm::vec2 bounds[2];
        };

        glm::vec2& operator[](int i);
        const glm::vec2& operator[](int i) const;
    public:
        AABB();
        AABB(const glm::vec2& m, const glm::vec2& M);

        bool IsInside(const glm::vec2& pos);
    };

    //===== ScreenObject =====

    //Base class for any object that is rendered onto the screen and needs to be interacted with.
    class ScreenObject {
    public:
        virtual void OnHover() {}
        virtual void OnClick() {}

        virtual AABB GetAABB() = 0;

        //In case screen object wraps around other objects and does the lookup by itself.
        virtual ScreenObject* FetchLeaf(const glm::vec2& mousePos) { return this; }
    };

}//namespace eng
