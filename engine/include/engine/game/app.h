#pragma once

#include "engine/core/window.h"

namespace eng {

    class App : public WindowResizeHandler {
    public:
        App(int windowWidth, int windowHeight, const char* windowName);
        virtual ~App();

        //copy disabled
        App(const App&) = delete;
        App& operator=(const App&) const = delete;

        //move disabled
        App(App&&) noexcept = delete;
        App& operator=(App&&) noexcept = delete;

        int Run();

        virtual void OnResize(int width, int height) override;
    private:
        virtual void OnInit() {}
        virtual void OnUpdate() = 0;
        virtual void OnGUI() {}
    };

}//namespace eng
