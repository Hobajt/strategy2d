#pragma once

#include <engine/engine.h>

#include "stage.h"

class RecapController : public GameStageController {
public:
    RecapController();

    virtual void Update() override;
    virtual void Render() override;

    virtual int GetStageID() const override { return GameStageName::RECAP; }

    virtual void OnPreStart(int prevStageID, int info, void* data) override;
    virtual void OnStart(int prevStageID, int info, void* data) override;
    virtual void OnStop() override;
private:

};
