#include "editor.h"

using namespace eng;

Editor::Editor(int argc, char** argv) : App(1200, 900, "Editor") {}

void Editor::OnResize(int width, int height) {
    LOG_INFO("Resize triggered ({}x{})", width, height);
}

void Editor::OnInit() {
    try {
        shader = std::make_shared<Shader>("res/shaders/test_shader");
        shader->InitTextureSlots(Renderer::TextureSlotsCount());

        context.Terrain_SetupNew(glm::ivec2(10), Resources::DefaultTileset());
    } catch(std::exception& e) {
        LOG_INFO("ERROR: {}", e.what());
        LOG_ERROR("Failed to load resources; Terminating...");
        throw e;
    }

    btn_toggleFullscreen = InputButton(GLFW_KEY_T);
}

void Editor::OnUpdate() {
    Window& window = Window::Get();

    context.input.Update();

    Renderer::StatsReset();

    if(window.GetKeyState(GLFW_KEY_Q))
        window.Close();

    btn_toggleFullscreen.Update();
    if(btn_toggleFullscreen.down()) {
        static bool fullscreen = false;
        fullscreen = !fullscreen;
        LOG_INFO("SETTING FULLSCREEN = {}", fullscreen);
        window.SetFullscreen(fullscreen);
    }

    Renderer::Begin(shader, true);
    context.level.map.Render();
    context.tools.Render();
    Renderer::End();
}

void Editor::OnGUI() {
#ifdef ENGINE_ENABLE_GUI
    Audio::DBG_GUI();
    Camera::Get().DBG_GUI();

    ImGui::Begin("General");
    ImGui::Text("FPS: %.1f", Input::Get().fps);
    ImGui::Text("Draw calls: %d | Total quads: %d (%d wasted)", Renderer::Stats().drawCalls, Renderer::Stats().totalQuads, Renderer::Stats().wastedQuads);
    ImGui::Text("Textures: %d", Renderer::Stats().numTextures);
    ImGui::End();

    for(auto& comp : context.components) {
        comp->GUI_Update();
    }
#endif
}

/*TODO: tile painting tool progress
    - maybe add operation cancel option (esc?)
    - implement valid tileset transitions
*/

/*PAINT BEHAVIOUR:
    - all types have predefined tile types, that they can neighbor with
        - water  -> mud1
        - walls  -> grass1
        - mud2   -> mud1
        - grass2 -> grass1
        - rocks  -> mud1
        - trees  -> grass1
    - when painting & the neighboring tile doesn't match a transition tile is inserted
    - transition tile override anything that was previously there
*/

/*Map logic checks:
    - valid tile type transitions (water -> mud; not grass directly) - only if I decide to enforce this
    - building placement validity (not just when placing, also when updating tiles -> can cause building removal)
*/

//TODO: render starting locations
//TODO: add starting locations overlap checks (or solve in some other way)
//TODO: savefile impl

//TODO: as many things to be controlled through hotkeys

/*What to save:
    - map info
    - max players/starting locations
    - techree
    - faction objects
*/



/*EDITOR TOOLSET & CAPABILITIES:
    - undo/redo capability - will need struct to describe operations & keep em in a stack
        - probably just track tile painting & object placements (no need to track techtree changes for example)

    - tile painting tool
        - changes tile types
        - option to change brush size (and maybe shape as well)
        - option to randomize tile variations while painting (checkbox)
        - highlight brush boundaries when hovering over the map
    - techtree editor
        - purely in GUI, to modify techtree struct of each faction (probably just checkboxes)
        - faction count == max players?
    - starting location movement
        - either as separate tool or add buttons next to the location print in GUI
    - object placement/erase tool
    - diplomacy tab
    - general selection tool
        - on click selection
        - select tile -> shows tile info (type & variation) + options to change it
        - select object -> show it's info (stats, if resource then amount left, etc.)
*/

/*NOTES:
    - would like to be able to create maps even for campaigns here

    - what will the map file contain:
        - tile info
        - resources
        - rocks
        - starting locations
        - techtrees
        - faction objects
        - faction relations (diplomacy)
    
    - custom vs campaign game:
        - techtree and faction-related stuff are optional fields (can be empty)
        - map is custom if it has no techtree/faction stuff & has starting locations
        - otherwise it's a campaign map
    
    - starting location:
        - 2 values - worker & main building position
        - has to be defined for all the possible factions (up to max map players)

    - tileset option:
        - there will be map default
        - it will be possible to change it ingame tho (for custom games)
        - have option in editor to change tileset (without modifying the default) - for visualization
    
*/

