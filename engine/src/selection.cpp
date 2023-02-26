#include "engine/core/selection.h"

#include "engine/utils/log.h"

namespace eng {

    //===== AABB =====

    glm::vec2& AABB::operator[](int i) {
        return bounds[i];
    }

    const glm::vec2& AABB::operator[](int i) const {
        return bounds[i];
    }

    bool AABB::IsInside(const glm::vec2& pos) {
        //TODO: maybe find optimized version online if it exists
        bool checkMin = (min.x < pos.x) && (min.y < pos.y);
        bool checkMax = (max.x > pos.x) && (max.y > pos.y);
        return (checkMin && checkMax);
    }

    //===== SelectionHandler =====

    ScreenObject* SelectionHandler::GetSelection(const glm::vec2& mousePos) {
        for(auto& [bb, obj] : visibleObjects) {
            // ENG_LOG_TRACE("Selection - box = ({}, {}), pos = {}", to_string(bb.min), to_string(bb.max), to_string(mousePos));
            if(bb.IsInside(mousePos)) {
                return obj->FetchLeaf(mousePos);
            }
        }
        return nullptr;
    }

}//namespace eng
