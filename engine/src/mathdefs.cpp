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

}//namespace eng
