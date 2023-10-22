#pragma once

#include "engine/game/faction.h"

namespace eng {

    //===== NatureFactionController =====

    class NatureFactionController : public FactionController {
    public:
        NatureFactionController(FactionsFile::FactionEntry&& entry, const glm::ivec2& mapSize);

        virtual int GetColorIdx(const glm::ivec3& num_id) const override;
    private:

    };

}//namespace eng
