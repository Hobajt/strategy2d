#pragma once

#include <engine/engine.h>

struct EditorContext;

class EditorInputHandler {
public:
    EditorInputHandler(EditorContext& context);
    EditorInputHandler(EditorContext&&) = delete;

    void InputCallback(int keycode, int modifiers);

    void Update();
private:
    EditorContext& context;
    bool suppressed = false;

    bool lmb_alt = false;
    glm::vec2 lmb_startCameraPos;
    glm::vec2 lmb_startMousePos;
};
