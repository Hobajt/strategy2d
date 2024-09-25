#include "engine/game/scenario.h"
#include "engine/utils/log.h"

#include "engine/game/level.h"

namespace eng {

    ScenarioControllerRef ScenarioController::Initialize(int campaignIdx, int race, const std::vector<int>& data) {
        ScenarioControllerRef ctrl = nullptr;

        //TODO:
        switch(campaignIdx) {
            case 0:
                ctrl = std::make_shared<Sc00_ScenarioController>(data, race);
                break;
            case 1:
                ctrl = std::make_shared<Sc01_ScenarioController>(data, race);
                break;
        }

        if(ctrl != nullptr)
            ENG_LOG_TRACE("ScenarioController::Initialize - controller initialized (idx: {}, race: {})", campaignIdx, race);
        else
            ENG_LOG_ERROR("ScenarioController::Initialize - invalid scenario! (idx: {}, race: {})", campaignIdx, race);

        return ctrl;
    }

    //=============================================

    Sc00_ScenarioController::Sc00_ScenarioController(const std::vector<int>& data, int race) {}

    void Sc00_ScenarioController::Update(Level& level) {}

    //=============================================

    Sc01_ScenarioController::Sc01_ScenarioController(const std::vector<int>& data, int race) {}

    void Sc01_ScenarioController::Update(Level& level) {}

}//namespace eng
