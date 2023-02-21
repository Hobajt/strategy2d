#pragma once

#include <string>
#include <vector>

#include "engine/utils/setup.h"

#ifdef ENGINE_ENABLE_GUI
    #include <imgui.h>
#endif

namespace eng::GUI {

    //Initializes imgui context. Must be called after window initialization.
    void Initialize();

    //Destroys imgui context, call before window destruction. 
    void Release();

    //=====

    //Signals a beginning of the frame for GUI context.
    void Begin();

    //Marks the end of the frame.
    void End();

    //===== custom widgets =====

}//namespace eng::GUI
