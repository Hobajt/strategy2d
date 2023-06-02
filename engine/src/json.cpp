#include "engine/utils/json.h"

namespace eng::json {

    glm::ivec2 parse_ivec2(const nlohmann::json& json) {
        return glm::ivec2(json.at(0), json.at(1));
    }

    glm::ivec3 parse_ivec3(const nlohmann::json& json) {
        return glm::ivec3(json.at(0), json.at(1), json.at(2));
    }

    glm::vec2 parse_vec2(const nlohmann::json& json) {
        return glm::vec2(json.at(0), json.at(1));
    }

    glm::vec4 parse_vec4(const nlohmann::json& json) {
        return glm::vec4(json.at(0), json.at(1), json.at(2), json.at(3));
    }

}//namespace eng::json
