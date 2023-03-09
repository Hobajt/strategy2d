#pragma once

#include <engine/engine.h>

#include "stage.h"

class IngameController : public GameStageController {
public:
    IngameController();

    virtual void Update() override;
    virtual void Render() override;

    virtual int GetStageID() const override { return GameStageName::INGAME; }

    virtual void OnPreStart(int prevStageID, int data) override;
    virtual void OnStart(int prevStageID, int data) override;
    virtual void OnStop() override;
private:

};
