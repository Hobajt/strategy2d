#include "engine/utils/mathdefs.h"
#include <stdio.h>

namespace eng {

    std::string to_string(const glm::vec2& v) {
        char buf[256];
        snprintf(buf, sizeof(buf), "(%5.2f, %5.2f)", v.x, v.y);
        return std::string(buf);
    }

    std::string to_string(const glm::vec3& v) {
        char buf[256];
        snprintf(buf, sizeof(buf), "(%5.2f, %5.2f, %5.2f)", v.x, v.y, v.z);
        return std::string(buf);
    }

    glm::vec4 clr_interpolate(const glm::vec4& a, const glm::vec4& b, float t) {
        return glm::vec4(
            (1.f - t) * a.x + t * b.x,
            (1.f - t) * a.y + t * b.y,
            (1.f - t) * a.z + t * b.z,
            (1.f - t) * a.w + t * b.w
        );
    }

    bool has_valid_direction(const glm::ivec2& v) {
        glm::ivec2 w = glm::abs(v);
        return w.x == 0 || w.y == 0 || (w.x - w.y) == 0;
    }

    bool has_valid_direction(const glm::vec2& v) {
        glm::vec2 w = glm::abs(v);
        return w.x < 1e-3f || w.y < 1e-3f || (w.x - w.y) < 1e-3f;
    }

    glm::ivec2 make_even(const glm::ivec2& v) {
        return glm::ivec2((v.x >> 1) << 1, (v.y >> 1) << 1);
    }

}//namespace eng
