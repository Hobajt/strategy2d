#include "engine/core/input.h"

#include <vector>
#include <utility>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "engine/utils/setup.h"
#include "engine/game/config.h"

// #define INPUT_CALLBACK_CHAINING

namespace eng {

    void MouseButtonInputCallback(GLFWwindow* window, int button, int action, int mods);
    void KeyInputCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void ScrollInputCallback(GLFWwindow* window, double xOffset, double yOffset);

    //Helper struct for FPS counting.
    //FPS is averaged over a specified interval.
    struct FPSCounter {
        float acc = 0.f;
        int frameCount = 0;
        float fps = 0.f;

        int numFrames = 100;
    public:
        float Update(float deltaTime);
        //Alternatives to averaging are immediate FPS (sucks) and rolling average (requires circular buffer and I'm lazy).
    };

    //Wrapper for all the helper variables that manage input state.
    struct InternalInputData {
        FPSCounter fps = {};
        double prevTime = 0.0;

        bool mouseButtonStates[3];
        glm::vec2 prevMousePos = glm::vec2(0.f);
        glm::vec2 scroll = glm::vec2(0.f);

        std::vector<std::pair<int, std::pair<KeyPressCallbackFn, void*>>> keyCallbacks;
#ifdef INPUT_CALLBACK_CHAINING
        GLFWkeyfun prevKeyCallback = nullptr;
        GLFWscrollfun prevScrollCallback = nullptr;
        GLFWmousebuttonfun prevMouseButtonCallback = nullptr;
#endif
    };

    //====== Input ======

    static InternalInputData data = {};

    Input& Input::Get() {
        static Input instance = Input();
        return instance;
    }

    void Input::Update() {
        deltaTime_real = UpdateDeltaTime();
        deltaTime = deltaTime_real * Config::GameSpeed();
        fps = data.fps.Update(deltaTime_real);
        anim_frame += deltaTime;
        if(paused)
            deltaTime = 0.f;

        UpdateKeys();
        UpdateMouse();
    }

    void Input::SetFPSInterval(int frameInterval) {
        data.fps.numFrames = frameInterval;
        data.fps.acc = 0.f;
        data.fps.frameCount = 0;
    }

    void Input::AddKeyCallback(int keycode, KeyPressCallbackFn callback, bool replace, void* userData) {
        if (!replace) {
            data.keyCallbacks.push_back({ keycode, { callback, userData } });
        }
        else {
            for (auto& [kc, cbd] : data.keyCallbacks) {
                if (kc == keycode) {
                    cbd = { callback, userData };
                    return;
                }
            }
            data.keyCallbacks.push_back({ keycode, { callback, userData } });
        }
        ENG_LOG_TRACE("Input::AddKeyCallback - new callback for '{}' ({} callbacks in total)", keycode, data.keyCallbacks.size());
    }

    double Input::CurrentTime() {
        return glfwGetTime();
    }

    float Input::GameTimeDelay(float delay) {
        //TODO: might want to track action starting time, rather then end time
        //because when game speed changes, end time stays unchanged
        return float(glfwGetTime()) + delay / Config::GameSpeed();
    }

    void Input::ClampCursorPos(const glm::vec2& min, const glm::vec2& max) {
        // ENG_LOG_TRACE("({}, {}), ({}, {}), ({}, {}), ({}, {})", mousePos.x, mousePos.y, mousePos_n.x, mousePos_n.y, min.x, min.y, max.x, max.y);
        bool update = false;
        if(mousePos_n.x < min.x) {
            mousePos_n.x = min.x;
            update = true;
        }
        else if(mousePos_n.x > max.x) {
            mousePos_n.x = max.x;
            update = true;
        }

        if(mousePos_n.y < min.y) {
            mousePos_n.y = min.y;
            update = true;
        }
        else if(mousePos_n.y > max.y) {
            mousePos_n.y = max.y;
            update = true;
        }

        if(update) {
            Window& window = Window::Get();
            window.SetMousePos(mousePos_n);
            mousePos = window.GetMousePos();
            mousePos_n = mousePos / glm::vec2(window.Size());
        }

        // ENG_LOG_TRACE("({}, {}), ({}, {})", mousePos.x, mousePos.y, mousePos_n.x, mousePos_n.y);
    }

    float Input::CustomAnimationFrame(int seed) {
        float frame = anim_frame + seed / 7.f;
        return frame - int(frame);
    }

    Input::~Input() {
        ENG_LOG_TRACE("[D] Input");
    }

    void Input::UpdateKeys() {
        Window& window = Window::Get();

        alt    = window.GetKeyState(GLFW_KEY_LEFT_ALT);
        shift  = window.GetKeyState(GLFW_KEY_LEFT_SHIFT);
        ctrl   = window.GetKeyState(GLFW_KEY_LEFT_CONTROL);
        space  = window.GetKeyState(GLFW_KEY_SPACE);
        enter  = window.GetKeyState(GLFW_KEY_ENTER);

        bool w = window.GetKeyState(GLFW_KEY_W);
        bool a = window.GetKeyState(GLFW_KEY_A);
        bool s = window.GetKeyState(GLFW_KEY_S);
        bool d = window.GetKeyState(GLFW_KEY_D);
        move1 = glm::vec2(float(int(d) - int(a)), float(int(w) - int(s)));
        isMove1 = (w || a || s || d);

        bool up    = window.GetKeyState(GLFW_KEY_UP);
        bool left  = window.GetKeyState(GLFW_KEY_LEFT);
        bool down  = window.GetKeyState(GLFW_KEY_DOWN);
        bool right = window.GetKeyState(GLFW_KEY_RIGHT);
        move_arrows = glm::vec2(float(int(right) - int(left)), float(int(up) - int(down)));
        isMoveArrows = (up || left || down || right);

        bool i = window.GetKeyState(GLFW_KEY_I);
        bool j = window.GetKeyState(GLFW_KEY_J);
        bool k = window.GetKeyState(GLFW_KEY_K);
        bool l = window.GetKeyState(GLFW_KEY_L);
        move2 = glm::vec2(float(int(l) - int(j)), float(int(i) - int(k)));
        isMove2 = (i || j || k || l);

        bool r = window.GetKeyState(GLFW_KEY_R);
        bool t = window.GetKeyState(GLFW_KEY_T);
        zoom = float(int(r) - int(t));
    }

    void Input::UpdateMouse() {
        Window& window = Window::Get();

        lmb.Update(data.mouseButtonStates[0]);
        rmb.Update(data.mouseButtonStates[1]);
        mmb.Update(data.mouseButtonStates[2]);

        scroll = data.scroll;
        data.scroll = glm::vec2(0.f);

        mousePos = window.GetMousePos();
        mouseDelta = mousePos - data.prevMousePos;
        data.prevMousePos = mousePos;

        mousePos_n = mousePos / glm::vec2(window.Size());
    }

    float Input::UpdateDeltaTime() {
        double currTime = glfwGetTime();
        float deltaTime = float(currTime - data.prevTime);
        data.prevTime = currTime;
        return deltaTime;
    }

    void Input::InitCallbacks(const Window& window) {
#ifndef INPUT_CALLBACK_CHAINING
        glfwSetMouseButtonCallback(window.Handle(), MouseButtonInputCallback);
        glfwSetKeyCallback(window.Handle(), KeyInputCallback);
        glfwSetScrollCallback(window.Handle(), ScrollInputCallback);
#else
        data.prevMouseButtonCallback = glfwSetMouseButtonCallback(window.Handle(), MouseButtonInputCallback);
        data.prevKeyCallback = glfwSetKeyCallback(window.Handle(), KeyInputCallback);
        data.prevScrollCallback = glfwSetScrollCallback(window.Handle(), ScrollInputCallback);
#endif
    }

    //====== Callbacks ======

    void KeyInputCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
        //only trigger for press signals
        if (action != GLFW_RELEASE) {
            bool single_press = (action == GLFW_PRESS);
            for (auto& [keycode, cbd] : data.keyCallbacks) {
                auto& [callback, userData] = cbd;
                if (keycode == key || keycode == -1) {
                    callback(key, mods, single_press, userData);
                    break;
                }
            }
        }

#ifdef INPUT_CALLBACK_CHAINING
        //callback chaining
        if (data.prevKeyCallback)
            data.prevKeyCallback(window, key, scancode, action, mods);
#endif
    }

    void MouseButtonInputCallback(GLFWwindow* window, int button, int action, int mods) {
        switch (button) {
            case GLFW_MOUSE_BUTTON_1:
                data.mouseButtonStates[0] = (action == GLFW_PRESS);
                break;
            case GLFW_MOUSE_BUTTON_2:
                data.mouseButtonStates[1] = (action == GLFW_PRESS);
                break;
            case GLFW_MOUSE_BUTTON_3:
                data.mouseButtonStates[2] = (action == GLFW_PRESS);
                break;
        }

#ifdef INPUT_CALLBACK_CHAINING
        //callback chaining
        if (data.prevMouseButtonCallback)
            data.prevMouseButtonCallback(window, button, action, mods);
#endif
    }

    void ScrollInputCallback(GLFWwindow* window, double xOffset, double yOffset) {
        data.scroll += glm::vec2(xOffset, yOffset);

#ifdef INPUT_CALLBACK_CHAINING
        //callback chaining
        if(data.prevScrollCallback)
            data.prevScrollCallback(window, xOffset, yOffset);
#endif
    }

    //=============================

    float FPSCounter::Update(float deltaTime) {
        acc += deltaTime;
        if((++frameCount) > numFrames) {
            fps = numFrames / acc;
            frameCount = 0;
            acc = 0.f;
        }
        return fps;
    }

}//namespace eng
