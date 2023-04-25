#pragma once

#include <engine/engine.h>

#include "stage.h"

namespace GameStartType { enum { INVALID, CAMPAIGN = 0, CUSTOM, LOAD }; }

class IngameController : public GameStageController {
public:
    IngameController();

    virtual void Update() override;
    virtual void Render() override;

    virtual int GetStageID() const override { return GameStageName::INGAME; }

    virtual void OnPreLoad(int prevStageID, int info, void* data) override;
    virtual void OnPreStart(int prevStageID, int info, void* data) override;
    virtual void OnStart(int prevStageID, int info, void* data) override;
    virtual void OnStop() override;
private:

};
