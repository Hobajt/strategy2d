#pragma once

struct GLFWwindow;

#include "engine/utils/mathdefs.h"
#include "engine/utils/keycodes.h"

namespace eng {

    //Base class that can be used to process window resize events.
    class WindowResizeHandler {
    public:
        virtual void OnResize(int width, int height) = 0;
    };


    //Basic wrapper around GLFW window structure. Singleton.
    class Window {
    public:
        static Window& Get();
    public:
        void Initialize(int width, int height, const char* name, float ratio = 1.f, int samples = 1);
        bool IsInitialized() const { return window != nullptr; }

        //copy disabled
        Window(const Window&) = delete;
        Window& operator=(const Window&) const = delete;

        //move disabled
        Window(Window&&) noexcept = delete;
        Window& operator=(Window&&) noexcept = delete;

        void Close();
        bool ShouldClose();
        void SwapAndPoll();
        void Resize(int width, int height);

        glm::ivec2 Size() const { return size; }
        int Width() const { return size[0]; }
        int Height() const { return size[1]; }
        glm::vec2 Aspect() const { return aspect; }
        float Ratio() const { return ratio; }

        GLFWwindow* Handle() const { return window; }

        //position in pixels, (0,0) = top-left
        glm::ivec2 CursorPos() const;
        //position in pixels, (0,0) = bot-left
        glm::ivec2 CursorPos_VFlip() const;

        void UpdateViewport();
        void UpdateViewport_full();

        void SetClearColor(const glm::vec4& clr);
        void Clear();

        void SetResizeCallbackHandler(WindowResizeHandler* handler) { resizeHandler = handler; }

        //=== input processing ===

        bool GetKeyState(int keycode) const;
        bool GetMouseKeyState(int keycode) const;
        glm::vec2 GetMousePos() const;
        void SetMousePos(const glm::vec2& mousePos_n) const;

        void SetFullscreen(bool fullscreen);
        void SetRatio(float r);
    private:
        Window() = default;
        ~Window();

        void Release() noexcept;

        void UpdateSize(int width, int height);
    private:
        GLFWwindow* window = nullptr;
        glm::ivec2 size;            //current window size; always rectangular
        glm::vec2 aspect;           //no point in having this, but i'm lazy and don't wanna remove it

        WindowResizeHandler* resizeHandler = nullptr;

        bool is_fullscreen = false;
        glm::ivec2 windowed_size;   //size in windowed mode - to preserve between fullscreen switching (only preserves the smaller dimension)
        glm::ivec2 real_size;       //real size of window in pixels
        glm::ivec2 offset;          //pixel offset between window corner and screen corner
        float ratio;                //width : height ratio that the screen should maintain
    };

}//namespace eng
