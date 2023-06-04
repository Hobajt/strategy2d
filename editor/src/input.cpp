#include "input.h"
#include "context.h"

using namespace eng;

EditorInputHandler::EditorInputHandler(EditorContext& context_) : context(context_) {
    
    //setup input callback
    Input::Get().AddKeyCallback(-1, [](int keycode, int modifiers, void* userData) {
        static_cast<EditorInputHandler*>(userData)->InputCallback(keycode, modifiers);
    }, true, this);

    Camera::Get().EnableBoundaries(false);
}

void EditorInputHandler::InputCallback(int keycode, int modifiers) {
    if(suppressed)
        return;

    //hotkeys dispatch
    switch(keycode) {
        case GLFW_KEY_ESCAPE:
            ENGINE_IF_GUI(ImGui::SetWindowFocus(ToolsMenu::TabName()));
            context.tools.SwitchTool(ToolType::SELECT);
            break;
        case GLFW_KEY_1:        //painting tool
            ENGINE_IF_GUI(ImGui::SetWindowFocus(ToolsMenu::TabName()));
            context.tools.SwitchTool(ToolType::TILE_PAINT);
            break;
        case GLFW_KEY_2:        //object placement tool
            ENGINE_IF_GUI(ImGui::SetWindowFocus(ToolsMenu::TabName()));
            context.tools.SwitchTool(ToolType::OBJECT_PLACEMENT);
            break;
        case GLFW_KEY_3:        //techtree tab
            ENGINE_IF_GUI(ImGui::SetWindowFocus(TechtreeMenu::TabName()));
            context.tools.SwitchTool(ToolType::SELECT);
            break;
        case GLFW_KEY_4:        //diplomacy tab
            ENGINE_IF_GUI(ImGui::SetWindowFocus(DiplomacyMenu::TabName()));
            context.tools.SwitchTool(ToolType::SELECT);
            break;
        case GLFW_KEY_5:        //level info tab
            ENGINE_IF_GUI(ImGui::SetWindowFocus(InfoMenu::TabName()));
            context.tools.SwitchTool(ToolType::SELECT);
            break;
        case GLFW_KEY_6:        //file tab
            ENGINE_IF_GUI(ImGui::SetWindowFocus(FileMenu::TabName()));
            context.tools.SwitchTool(ToolType::SELECT);
            break;
        case GLFW_KEY_H:        //hotkeys tab
            ENGINE_IF_GUI(ImGui::SetWindowFocus(HotkeysMenu::TabName()));
            context.tools.SwitchTool(ToolType::SELECT);
            break;
        case GLFW_KEY_KP_ADD:
            context.tools.CustomSignal(1);
            break;
        case GLFW_KEY_KP_SUBTRACT:
            context.tools.CustomSignal(-1);
            break;
    }
}

void EditorInputHandler::Update() {
    Input& input = Input::Get();
    Camera& camera = Camera::Get();

    //input & camera updates
    input.Update();
    camera.Update();

    //tools control events dispatch
    switch(input.lmb.state) {
        case InputButtonState::DOWN:
            lmb_alt = input.alt;
            lmb_startCameraPos = camera.Position();
            lmb_startMousePos = input.mousePos_n * 2.f - 1.f;
        case InputButtonState::UP:
        case InputButtonState::PRESSED:
            if(lmb_alt)
                camera.PositionFromMouse(lmb_startCameraPos, lmb_startMousePos, input.mousePos_n * 2.f - 1.f);
            else
                context.tools.OnLMB(input.lmb.state);
            break;
        case InputButtonState::RELEASED:
            lmb_alt = false;
            context.tools.OnHover();
            break;
    }

    //scroll control - tool signal or camera zoom
    if(input.ctrl && int(input.scroll.y) != 0) {
        context.tools.CustomSignal(input.scroll.y);
    }
    else {
        camera.ZoomUpdate(true);
    }
}
