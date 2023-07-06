#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <string>

namespace eng {

    std::string to_string(const glm::vec2& v);
    std::string to_string(const glm::vec3& v);

    glm::vec4 clr_interpolate(const glm::vec4& a, const glm::vec4& b, float t);

    //Returns true if vector goes in one of the 8 directions.
    bool has_valid_direction(const glm::ivec2& v);
    bool has_valid_direction(const glm::vec2& v);

    glm::ivec2 make_even(const glm::ivec2& v);

}//namespace eng
