#include "engine/utils/mathdefs.h"
#include "engine/utils/setup.h"

#include <stdio.h>
#include <algorithm>

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
        return (w.x == 0 || w.y == 0 || (w.x - w.y) == 0) && (w.x != 0 || w.y != 0);
    }

    bool has_valid_direction(const glm::vec2& v) {
        glm::vec2 w = glm::abs(v);
        return w.x < 1e-3f || w.y < 1e-3f || (w.x - w.y) < 1e-3f;
    }

    glm::ivec2 make_even(const glm::ivec2& v) {
        return glm::ivec2((v.x >> 1) << 1, (v.y >> 1) << 1);
    }

    int get_range(const glm::ivec2& pos, const glm::ivec2& m, const glm::ivec2& M) {
        glm::ivec2 a = glm::abs(pos - m);
        glm::ivec2 b = glm::abs(pos - M);
        return std::min({ std::max(a.x, a.y), std::max(b.x, b.y) });
    }

    int get_range(const glm::ivec2& m1, const glm::ivec2& M1, const glm::ivec2& m2, const glm::ivec2& M2) {
        glm::ivec2 a = glm::abs(m1 - m2);
        glm::ivec2 b = glm::abs(m1 - M2);
        glm::ivec2 c = glm::abs(M1 - m2);
        glm::ivec2 d = glm::abs(M1 - M2);
        return std::min({ std::max(a.x, a.y), std::max(b.x, b.y), std::max(c.x, c.y), std::max(d.x, d.y) });
    }

    int chessboard_distance(const glm::ivec2& p1, const glm::ivec2& p2) {
        glm::ivec2 v = glm::abs(p2 - p1);
        return std::max(v.x, v.y);
    }

    int DirVectorCoord(int ori) {
        ori = ori % 8;
        return 1 - 2*(ori/5) - int(ori % 4 == 0);
    }

    glm::ivec2 DirectionVector(int orientation) {
        return glm::ivec2( DirVectorCoord(orientation), -DirVectorCoord(orientation+6) );
    }

    int VectorOrientation(const glm::ivec2& v) {
        int orientation = int(4.f * (1.f + (std::atan2(v.y, v.x) * glm::one_over_pi<float>())));
        ASSERT_MSG(orientation >= 1 && orientation <= 8, "VectorOrientation - invalid conversion.");
        orientation = (8-orientation+6) % 8;
        return orientation;
    }

    int VectorOrientation(const glm::vec2& v) {
        int orientation = int(4.f * (1.f + (std::atan2(v.y, v.x) * glm::one_over_pi<float>())));
        ASSERT_MSG(orientation >= 1 && orientation <= 8, "VectorOrientation - invalid conversion.");
        orientation = (8-orientation+6) % 8;
        return orientation;
    }

    std::pair<glm::vec2, glm::vec2> order_vals(const glm::vec2& a, const glm::vec2& b) {
        glm::vec2 m = (a.x < b.x) ? glm::vec2(a.x, b.x) : glm::vec2(b.x, a.x);
        glm::vec2 M = (a.y < b.y) ? glm::vec2(a.y, b.y) : glm::vec2(b.y, a.y);
        
        float t = m.y;
        m.y = M.x;
        M.x = t;

        return { m, M };
    }

}//namespace eng
