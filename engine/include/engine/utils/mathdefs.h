#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>

namespace eng {

    std::string to_string(const glm::vec2& v);
    std::string to_string(const glm::vec3& v);

}//namespace eng
