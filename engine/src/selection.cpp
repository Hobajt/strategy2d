#include "engine/core/selection.h"

#include "engine/utils/log.h"

namespace eng {

    //===== AABB =====

    AABB::AABB() : min(glm::vec2(0.f)), max(glm::vec2(0.f)) {}
    AABB::AABB(const glm::vec2& m, const glm::vec2& M) : min(m), max(M) {}

    bool AABB::IsInside(const glm::vec2& pos) {
        bool checkMin = (min.x < pos.x) && (min.y < pos.y);
        bool checkMax = (max.x > pos.x) && (max.y > pos.y);
        return (checkMin && checkMax);
    }

}//namespace eng
