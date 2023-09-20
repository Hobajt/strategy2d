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

    //Chebyshev(chessboard) distance between a point and the closest point on a rectangle.
    int get_range(const glm::ivec2& pos, const glm::ivec2& m, const glm::ivec2& M);

    //Chebyshev(chessboard) distance between two rectangles (returns the lowest distance).
    int get_range(const glm::ivec2& m1, const glm::ivec2& M1, const glm::ivec2& m2, const glm::ivec2& M2);

    int chessboard_distance(const glm::ivec2& p1, const glm::ivec2& p2);

    //direction conversions (between vector and number identifier)

    int DirVectorCoord(int orientation);
    glm::ivec2 DirectionVector(int orientation);
    int VectorOrientation(const glm::ivec2& v);
    int VectorOrientation(const glm::vec2& v);

}//namespace eng
