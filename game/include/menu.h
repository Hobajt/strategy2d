#pragma once

#include <engine/engine.h>

class MainMenu : public eng::GUI::ButtonCallbackHandler {
public:
    int Update();

    virtual void OnClick(int buttonID) override;
private:
    int state;
};

class TransitionHandler {
public:
    bool IsHappening();
    bool IsDone();
};
