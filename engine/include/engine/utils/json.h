#pragma once

#include <nlohmann/json.hpp>

#include "engine/utils/mathdefs.h"

namespace eng::json {

    glm::ivec2 parse_ivec2(const nlohmann::json& json);

    glm::ivec3 parse_ivec3(const nlohmann::json& json);

    glm::ivec4 parse_ivec4(const nlohmann::json& json);
    
    glm::vec2 parse_vec2(const nlohmann::json& json);

    glm::vec4 parse_vec4(const nlohmann::json& json);

}//namespace eng::json
