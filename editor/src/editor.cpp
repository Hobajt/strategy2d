#include "editor.h"

using namespace eng;

Editor::Editor(int argc, char** argv) : App(1200, 900, "Editor") {}

void Editor::OnResize(int width, int height) {
    LOG_INFO("Resize triggered ({}x{})", width, height);
}

void Editor::OnInit() {
    inputHandler.Init(toolsMenu);
    toolsMenu.Init(level);
    infoMenu.SetLevelRef(&level);
    Camera::Get().EnableBoundaries(false);

    try {
        shader = std::make_shared<Shader>("res/shaders/test_shader");
        shader->InitTextureSlots(Renderer::TextureSlotsCount());

        Terrain_SetupNew();
    } catch(std::exception& e) {
        LOG_INFO("ERROR: {}", e.what());
        LOG_ERROR("Failed to load resources; Terminating...");
        throw e;
    }

    btn_toggleFullscreen = InputButton(GLFW_KEY_T);
    btn_toggleGUI = InputButton(GLFW_KEY_P);
}

void Editor::OnUpdate() {
    Window& window = Window::Get();
    Input& input = Input::Get();
    Camera& camera = Camera::Get();

    input.Update();
    camera.Update();

    Renderer::StatsReset();

#ifdef ENGINE_ENABLE_GUI
    btn_toggleGUI.Update();
    if(btn_toggleGUI.down())
        guiEnabled = !guiEnabled;
#endif

    btn_toggleFullscreen.Update();
    if(btn_toggleFullscreen.down()) {
        static bool fullscreen = false;
        fullscreen = !fullscreen;
        LOG_INFO("SETTING FULLSCREEN = {}", fullscreen);
        window.SetFullscreen(fullscreen);
    }

    Renderer::Begin(shader, true);
    inputHandler.Update();
    level.Render();
    Renderer::End();
}

void Editor::OnGUI() {
#ifdef ENGINE_ENABLE_GUI
    if(guiEnabled) {
        Audio::DBG_GUI();
        Camera::Get().DBG_GUI();

        ImGui::Begin("General");
        ImGui::Text("FPS: %.1f", Input::Get().fps);
        ImGui::Text("Draw calls: %d | Total quads: %d (%d wasted)", Renderer::Stats().drawCalls, Renderer::Stats().totalQuads, Renderer::Stats().wastedQuads);
        ImGui::Text("Textures: %d", Renderer::Stats().numTextures);
        ImGui::End();

        infoMenu.Update();
        toolsMenu.Update();
        RenderHotkeysTab();
        FileMenu_Update();

        //TODO: move to custom classes & use macros from menu.cpp for naming
        ImGui::Begin("Techtree"); ImGui::End();
        ImGui::Begin("Diplomacy"); ImGui::End();
    }
#endif
}

void Editor::FileMenu_Update() {
    switch(fileMenu.Update()) {
        case FileMenuSignal::NEW:
            Terrain_SetupNew();
            break;
        case FileMenuSignal::LOAD:
        {
            int res = Terrain_Load();
            if(res == 0)
                fileMenu.Reset();
            else
                fileMenu.SignalError("File not found.");
            break;
        }
        case FileMenuSignal::SAVE:
        {
            int res = Terrain_Save();
            if(res == 0)
                fileMenu.Reset();
            else
                fileMenu.SignalError("No clue.");
            break;
        }
        case FileMenuSignal::QUIT:
            Window::Get().Close();
            break;
    }
}

/*TODO: tile painting tool progress
    - add onHover() callback to all tools from InputHandler - for brush highlights
    - figure out how should the tool interact with respect to inputs (when to update the tile, etc)
    - implement tileset transitions
    - tool GUI - tile type selection, variations logic
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

void Editor::Terrain_SetupNew() {
    //init terrain based on values from FileMenu, reset the camera
    level = Level(fileMenu.terrainSize, fileMenu.tileset.LoadTilesetOrDefault());

    Camera& camera = Camera::Get();
    camera.SetBounds(fileMenu.terrainSize);
    camera.Position(glm::vec2(fileMenu.terrainSize) * 0.5f - 0.5f);
    camera.ZoomToFit(glm::vec2(fileMenu.terrainSize) + 1.f);

    infoMenu.NewLevelCreated();
}

int Editor::Terrain_Load() {
    //TODO:
    return 0;
}

int Editor::Terrain_Save() {
    //TODO:
    return 1;
}

