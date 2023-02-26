#pragma once

class MainMenu {
public:
    int Update();
private:
    int state;
};

class TransitionHandler {
public:
    bool IsHappening();
    bool IsDone();
};
