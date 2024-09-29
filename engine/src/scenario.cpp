#include "engine/game/scenario.h"
#include "engine/utils/log.h"

#include "engine/game/level.h"
#include "engine/core/input.h"

constexpr float CONDITION_UPDATE_FREQ = 2.f;

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

    void Sc00_ScenarioController::Update(Level& level) {
        if(EndConditionsUpdate(level))
            return;
    }

    bool Sc00_ScenarioController::EndConditionsUpdate(Level& level) {
        t += Input::Get().deltaTime;
        if(t < CONDITION_UPDATE_FREQ)
            return false;

        t = 0.f;
        
        //lose if player is eliminated
        PlayerFactionControllerRef player = level.factions.Player();
        if(player->IsEliminated()) {
            level.info.end_conditions.at(EndConditionType::LOSE).disabled = false;
            level.info.end_conditions.at(EndConditionType::LOSE).Set();
            return true;
        }
        
        //win if player has barracks and 4 farms
        const FactionStats& stats = player->Stats();
        if(stats.buildings.at(BuildingType::BARRACKS) >= 1 && stats.buildings.at(BuildingType::FARM) >= 4) {
            level.info.end_conditions.at(EndConditionType::WIN).disabled = false;
            level.info.end_conditions.at(EndConditionType::WIN).Set();
            return true;
        }

        return false;
    }

    //=============================================

    Sc01_ScenarioController::Sc01_ScenarioController(const std::vector<int>& data, int race) {}

    void Sc01_ScenarioController::Update(Level& level) {
        if(EndConditionsUpdate(level))
            return;

        //TODO: enemy AI would probably be controlled from here
    }

    bool Sc01_ScenarioController::EndConditionsUpdate(Level& level) {
        t += Input::Get().deltaTime;
        if(t < CONDITION_UPDATE_FREQ)
            return false;

        t = 0.f;
        
        //lose if player is eliminated
        PlayerFactionControllerRef player = level.factions.Player();
        if(player->IsEliminated()) {
            level.info.end_conditions.at(EndConditionType::LOSE).disabled = false;
            level.info.end_conditions.at(EndConditionType::LOSE).Set();
            return true;
        }
        
        //win if player eliminates all the enemy factions
        bool has_enemies = false;
        int pID = player->ControllerID();
        for(auto& faction : level.factions) {
            int cID = faction->ControllerID();
            if(cID == FactionControllerID::NATURE || cID == FactionControllerID::LOCAL_PLAYER)
                continue;
            has_enemies |= level.factions.Diplomacy().AreHostile(player->ID(), faction->ID()) && !faction->IsEliminated();
        }
        if(!has_enemies) {
            level.info.end_conditions.at(EndConditionType::WIN).disabled = false;
            level.info.end_conditions.at(EndConditionType::WIN).Set();
            return true;
        }

        return false;
    }

}//namespace eng
