#pragma once

#include "engine/utils/mathdefs.h"
#include "engine/core/window.h"

namespace eng {

    //void KeyPressCallback(int keycode, int modifiers, void* data);
    typedef void(*KeyPressCallbackFn)(int keycode, int modifiers, void* userData);

    //====== InputButton ======

    //Wrapper for button management, tracks key presses and generates up/down/hold signals from it.
    struct InputButton {
        int state = 0;
        int keycode = GLFW_KEY_A;
    public:
        InputButton() {}
        InputButton(int keycode_) : keycode(keycode_) {}

        bool up() const { return state == -1; }
        bool down() const { return state == 1; }
        bool pressed() const { return state > 0; }
        bool released() const { return state <= 0; }

        void Update(bool isPressed) {
            int pp = int(state > 0);    //previous pressed
            int cp = int(isPressed);    //currently pressed
            state = (pp + cp) * cp - (pp + cp) * (1 - cp);
            // if (isPressed) state = state > 0 ? 2 : 1;
            // else state = state < 0 ? -1 : 0;
        }

        void Update(Window& w) { Update(w.GetKeyState(keycode)); }
        void Update() { Update(Window::Get().GetKeyState(keycode)); }
    };

    //====== Input ======

    //Class for input handling and polling. Singleton pattern.
    //Class updates its values either through Update() method (should be called every frame) or through callbacks from the GLFW.
    //Callbacks are used for mouse btns & scroll or custom keys (via AddKeyCallback()).
    //Mouse callbacks are set up automatically by Window class.
    class Input {
        friend class Window;
    public:
        static Input& Get();
    public:
        //copy disabled
        Input(const Input&) = delete;
        Input& operator=(const Input&) const = delete;

        //move disabled
        Input(Input&&) noexcept = delete;
        Input& operator=(Input&&) noexcept = delete;

        void Update();

        //Changes FPS measurement interval (value represents number of frames, not time).
        void SetFPSInterval(int frameInterval);

        //Registers new callback for provided key presses.
        //Data is stored along with the callback, can be used to pass object reference (watch out for object lifespan).
        //If replace == true, replaces the first found callback for the same key.
        //Use keycode=-1 for any key.
        void AddKeyCallback(int keycode, KeyPressCallbackFn callback, bool replace = false, void* userData = nullptr);

        static double CurrentTime();
    private:
        Input() = default;
        ~Input();

        void UpdateKeys();
        void UpdateMouse();
        float UpdateDeltaTime();

        //for callback initialization from the window class
        void InitCallbacks(const Window& window);
    public:
        float deltaTime;
        float fps;

        glm::vec2 move1, move2;
        bool isMove1, isMove2;
        float zoom;

        InputButton lmb,rmb,mmb;
        glm::vec2 mousePos, mouseDelta, scroll;
        glm::vec2 mousePos_n;

        bool alt, shift, ctrl, space, enter;
    };

}//namespace eng
