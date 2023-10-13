#pragma once

#include "engine/game/object_data.h"
#include "engine/utils/json.h"

namespace eng {

    GameObjectDataRef GameObjectData_ParseNew(const nlohmann::json& config);

    void GameObjectData_ParseExisting(const nlohmann::json& config, GameObjectDataRef data);

    void GameObjectData_FinalizeButtonDescriptions(GameObjectDataRef& data);

}//namespace eng
