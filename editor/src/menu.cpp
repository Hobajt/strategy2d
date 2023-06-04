#include "menu.h"

#include <nfd.h>
#include <filesystem>

using namespace eng;

#define BUF_SIZE 1024

#define MIN_PLAYERS_LIMIT 2
#define MAX_PLAYERS_LIMIT 8

#define TABNAME_FILE            "File"
#define TABNAME_LEVELINFO       "Level Info"
#define TABNAME_TECHTREE        "Techtree"
#define TABNAME_DIPLOMACY       "Diplomacy"
#define TABNAME_TOOL            "Tools"
#define TABNAME_HOTKEYS         "Hotkeys"

std::string tileIdx2name(int idx);

TilesetChoices TilesetChoice::choices = {};

const char* TilesetChoices::Default() {
    return FindName("summer");
}

const char* TilesetChoices::FindName(const char* name) {
    for(size_t i = 0; i < names_ptr.size(); i++) {
        if(strcmp(names_ptr[i], name) == 0)
            return names_ptr[i];
    }
    return nullptr;
}

void TilesetChoices::ReloadTilesets() {
    names_ptr.clear();
    names.clear();

    //iterate over tilesets directory & load the names of all JSON files
    std::string dir_path = "res/json/tilemaps";
    for(const auto& entry : std::filesystem::directory_iterator(dir_path)) {
        if(entry.is_directory() || !entry.path().has_extension())
            continue;
        
        if(entry.path().extension() == ".json") {
            names.push_back(GetFilename(entry.path().string(), true));
            // ENG_LOG_TRACE("TILESET: {}", names.back());
        }
    }

    for(std::string& s : names)
        names_ptr.push_back(s.c_str());
}

TilesetChoice::TilesetChoice() {
    choices.ReloadTilesets();
    selection = choices.Default();
}

void TilesetChoice::Reset() {
    selection = choices.Default();
}

bool TilesetChoice::GUI_Combo() {
    bool res = false;
    if (ImGui::BeginCombo("##combo", selection)) {
        for(size_t i = 0; i < choices.names_ptr.size(); i++) {
            bool is_selected = selection == choices.names_ptr[i];
            if(ImGui::Selectable(choices.names_ptr[i], is_selected)) {
                selection = choices.names_ptr[i];
                res = true;
            }
            if(is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::SameLine();
    if(ImGui::Button("RELOAD")) {
        choices.ReloadTilesets();
        selection = choices.Default();
    }
    return res;
}

eng::TilesetRef TilesetChoice::LoadTilesetOrDefault(bool forceReload) {
    return (selection != nullptr) ? Resources::LoadTileset(selection, forceReload) : Resources::DefaultTileset();
}

//===== FileMenu =====

FileMenu::FileMenu() {
    filepath = new char[BUF_SIZE];
    Reset();
}

FileMenu::~FileMenu() {
    delete[] filepath;
}

int FileMenu::Update() {
    int signal = FileMenuSignal::NOTHING;
#ifdef ENGINE_ENABLE_GUI
    ImGui::Begin(TABNAME_FILE);

    if(ImGui::Button("New", ImVec2(ImGui::GetWindowSize().x * 0.3f, 0.f))) {
        submenu = (submenu != 1) ? 1 : 0;
        err = false;
    }
    ImGui::SameLine();
    if(ImGui::Button("Load", ImVec2(ImGui::GetWindowSize().x * 0.3f, 0.f))) {
        submenu = (submenu != 2) ? 2 : 0;
        err = false;
    }
    ImGui::SameLine();
    if(ImGui::Button("Save", ImVec2(ImGui::GetWindowSize().x * 0.3f, 0.f))) {
        submenu = (submenu != 3) ? 3 : 0;
        err = false;
    }

    ImGui::Separator();
    ImGui::PushID(1);

    if(!err) {
        switch(submenu) {
            case 1:
                signal = Submenu_New();
                break;
            case 2:
                signal = Submenu_Load();
                break;
            case 3:
                signal = Submenu_Save();
                break;
        }
    }
    else {
        Submenu_Error();
    }

    ImGui::PopID();
    ImGui::Separator();

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetWindowSize().x * 0.5f - 0.5f * ImGui::CalcTextSize("Quit").x - ImGui::GetStyle().FramePadding.x);
    if(ImGui::Button("Quit")) {
        signal = FileMenuSignal::QUIT;
    }

    ImGui::End();
#endif

    return signal;
}

void FileMenu::Reset() {
    submenu = 0;
    filepath[0] = '\0';
    terrainSize = glm::ivec2(10, 10);
    errMsg = "---";
    err = false;
    tileset.Reset();
}

void FileMenu::SignalError(const std::string& msg) {
    errMsg = msg;
    err = true;
}

int FileMenu::Submenu_New() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Text("New Terrain");
    if(ImGui::DragInt2("Size", (int*)&terrainSize)) {
        if(terrainSize.x < 1) terrainSize.x = 1;
        if(terrainSize.y < 1) terrainSize.y = 1;
    }
    tileset.GUI_Combo();

    if(ImGui::Button("Generate")) {
        return FileMenuSignal::NEW;
    }
#endif
    return FileMenuSignal::NOTHING;
}

int FileMenu::Submenu_Load() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Text("Load from File (expects absolute filepath)");
    ImGui::InputText("", filepath, sizeof(char) * BUF_SIZE);
    ImGui::SameLine();
    if(ImGui::Button("Browse")) {
        FileDialog();
    }

    if(ImGui::Button("Load")) {
        return FileMenuSignal::LOAD;
    }
#endif
    return FileMenuSignal::NOTHING;
}

int FileMenu::Submenu_Save() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Text("Save Terrain (expects absolute filepath)");
    ImGui::InputText("", filepath, sizeof(char) * BUF_SIZE);
    ImGui::SameLine();
    if(ImGui::Button("Browse")) {
        FileDialog();
    }

    if(ImGui::Button("Save")) {
        return FileMenuSignal::SAVE;
    }
#endif
    return FileMenuSignal::NOTHING;
}

void FileMenu::Submenu_Error() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Text("Operation could not be completed due to an error.");
    ImGui::Text("Reason: %s", errMsg.c_str());
    if(ImGui::Button("OK")) {
        err = false;
    }
#endif
}

void FileMenu::FileDialog() {
    //TODO: might want to move to a separate thread

    nfdchar_t *outPath = NULL;
    nfdresult_t result = NFD_OpenDialog( NULL, NULL, &outPath );

    if (result == NFD_OKAY) {
        ENG_LOG_DEBUG("NFD: Filepath: '{}'", outPath);

        size_t len = strlen(outPath);
        if(len > BUF_SIZE) {
            filepath[0] = '\0';
            ENG_LOG_WARN("NDF: Filepath too long ({})", len);
        }
        else if(strncpy_s(filepath, BUF_SIZE, outPath, len)) {
            filepath[0] = '\0';
            ENG_LOG_WARN("NDF: Failed to copy the filepath.");
        }

        free(outPath);
    }
    else if ( result == NFD_CANCEL ) {
        ENG_LOG_DEBUG("NDF: User pressed cancel.");
    }
    else {
        ENG_LOG_WARN("NFD: Error: {}", NFD_GetError());
    }
}

//===== LevelInfoMenu =====

LevelInfoMenu::LevelInfoMenu() {
    levelName = new char[BUF_SIZE];
    snprintf(levelName, sizeof(char) * BUF_SIZE, "%s", "unnamed_level");

    maxPlayers = 2;
    for(int i = 0; i < 8; i++) {
        startingLocations.push_back(glm::ivec2(i, 0));
    }
}

LevelInfoMenu::~LevelInfoMenu() {
    delete[] levelName;
}

void LevelInfoMenu::Update() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Begin(TABNAME_LEVELINFO);

    if(level == nullptr)
        return;

    ImGui::Text("Level info");
    ImGui::InputText("Name", levelName, sizeof(char) * BUF_SIZE);
    glm::ivec2& size = level->MapSize();
    ImGui::Text("Size: [%d x %d]", size.x, size.y);
    ImGui::Separator();
    ImGui::Text("Tileset");
    if(tileset.GUI_Combo()) {
        level->map.ChangeTileset(tileset.LoadTilesetOrDefault(tileset_forceReload));
    }
    ImGui::Checkbox("Forced reload", &tileset_forceReload);
    ImGui::Separator();
    ImGui::Text("Player count & starting locations");
    if(ImGui::SliderInt("Max players", &maxPlayers, MIN_PLAYERS_LIMIT, MAX_PLAYERS_LIMIT)) {
        if(maxPlayers < MIN_PLAYERS_LIMIT) maxPlayers = MIN_PLAYERS_LIMIT;
        else if(maxPlayers > MAX_PLAYERS_LIMIT) maxPlayers = MAX_PLAYERS_LIMIT;
    }
    if(ImGui::CollapsingHeader("Starting locations")) {
        ImGui::PushID(&startingLocations);
        for(int i = 0; i < maxPlayers; i++) {
            char buf[256];
            snprintf(buf, sizeof(buf), "player[%d]", i);
            if(ImGui::DragInt2(buf, (int*)&startingLocations[i])) {
                if(startingLocations[i].x < 0)
                    startingLocations[i].x = 0;
                else if(startingLocations[i].x >= level->map.Width())
                    startingLocations[i].x = level->map.Width()-1;

                if(startingLocations[i].y < 0)
                    startingLocations[i].y = 0;
                else if(startingLocations[i].y >= level->map.Height())
                    startingLocations[i].y = level->map.Height()-1;

                //TODO: add starting location overlap checks
            }
        }
        ImGui::PopID();
    }
    ImGui::Checkbox("Render locations", &renderStartingLocations);

    ImGui::End();
#endif
}

void LevelInfoMenu::NewLevelCreated() {
    tileset.selection = tileset.choices.FindName(level->map.GetTilesetName().c_str());
}

//===== EditorTool =====

void SelectionTool::OnLMB(int lmbState, const glm::vec2& lmb_startPos) {
    ENG_LOG_INFO("SelectionTool::OnLMB");
}

void SelectionTool::OnHover() {}

void SelectionTool::Update() {
    ImGui::Text("Selection Tool");
    ImGui::Separator();
}

void PaintingTool::OnLMB(int lmbState, const glm::vec2& lmb_startPos) {
    Camera& cam = Camera::Get();
    glm::ivec2 ms = level->map.Size();

    if(lmbState == 1) {
        currentPaint.clear();
    }
    else {
         //hovering tile map coord
        glm::ivec2 coord = glm::ivec2(cam.GetMapCoords(Input::Get().mousePos_n * 2.f - 1.f) + 0.5f);

        //brush corners
        int xm = std::max(0, coord.x-bl);
        int xM = std::min(coord.x+br, ms.x);
        int ym = std::max(0, coord.y-bl);
        int yM = std::min(coord.y+br, ms.y);

        // for(int y = ym; y < yM; y++) {
        //     for(int x = xm; x < xM; x++) {
        //         int idx = y*ms.x + x;
        //         if(!currentPaint.count(idx)) {
        //             // currentPaint.insert({ idx,  });

        //         }
        //     }
        // }
    }

    /*TODO:
        - duplicate map struct
            - one in level
            - 2nd in editor tools
            - the one in editor tools will be "live" - displayed on the screen
                - to be able to see tool editing live
                - after operation is finalized, changes are committed to level map
                - after each operation, both maps should be the same
            - maybe also duplicate objects - cuz the other tool works with those
                - or maybe not, if the other tool can create a single object per operation
                - 
    */
    
    /*
    lmb == down:
        - cleanup - flush all previous variables
    lmb == pressed:
        - keep track of marked tiles
    */

   /*
    - apply changes directly into the level data
    - need to make sure that there's no way an operation can be interrupted without submission into the op stack
    - 
   */
}

void PaintingTool::OnHover() {
    Camera& cam = Camera::Get();
    glm::ivec2 ms = level->map.Size();

    //hovering tile map coord
    glm::ivec2 coord = glm::ivec2(cam.GetMapCoords(Input::Get().mousePos_n * 2.f - 1.f) + 0.5f);

    //highlight corners
    int xm = std::max(0, coord.x-bl);
    int xM = std::min(coord.x+br, ms.x);
    int ym = std::max(0, coord.y-bl);
    int yM = std::min(coord.y+br, ms.y);

    float width = 0.1f;
    float zIdx = -0.1f;
    glm::vec4 clr = glm::vec4(1.f, 0.f, 0.f, 1.f);
    
    //render quads as highlight borders
    Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(xm, ym)), zIdx), glm::vec2(xM - xm + width, width) * cam.Mult(), clr));
    Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(xm, yM)), zIdx), glm::vec2(xM - xm + width, width) * cam.Mult(), clr));
    Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(xm, ym)), zIdx), glm::vec2(width, yM - ym) * cam.Mult(), clr));
    Renderer::RenderQuad(Quad::FromCorner(glm::vec3(cam.map2screen(glm::vec2(xM, ym)), zIdx), glm::vec2(width, yM - ym) * cam.Mult(), clr));
}

void PaintingTool::SignalUp() {
    UpdateBrushSize(brushSize+1);
}

void PaintingTool::SignalDown() {
    UpdateBrushSize(brushSize-1);
}

void PaintingTool::Finalize(OpStack& opStack) {
    //TODO: submit operation to the stack
}

void PaintingTool::Update() {
    ImGui::Text("Tile Painting Tool");
    ImGui::Separator();

    ImGui::PushID(0);
    if(ImGui::BeginListBox("Tile to paint:")) {
        for(int i = 0; i < TileType::COUNT; i++) {
            if(ImGui::Selectable(tileIdx2name(i).c_str(), selection == i)) {
                selection = i;
            }
        }
        ImGui::EndListBox();
    }
    ImGui::PopID();

    ImGui::Separator();
    if(ImGui::DragInt("Brush size", &brushSize)) {
        UpdateBrushSize(brushSize);
    }
}


void PaintingTool::UpdateBrushSize(int newSize) {
    brushSize = newSize;
    if(brushSize < 1) brushSize = 1;
    br = (brushSize+1) / 2;
    bl = brushSize - br;
}

void ObjectPlacementTool::OnLMB(int lmbState, const glm::vec2& lmb_startPos) {
    ENG_LOG_INFO("ObjectPlacementTool::OnLMB");
}

void ObjectPlacementTool::OnHover() {}

void ObjectPlacementTool::Update() {
    ImGui::Text("Object Placement Tool");
    ImGui::Separator();
}

//===== ToolsMenu =====

void ToolsMenu::Init(Level& level) {
    tools.painting.level = &level;
    tools.placement.level = &level;
    tools.selection.level = &level;
}

void ToolsMenu::Update() {
    ImGui::Begin(TABNAME_TOOL);

    if(tools.currentTool != nullptr) {
        tools.currentTool->Update();
    }

    ImGui::End();
}

//===== InputHandler =====

void InputHandler::Init(ToolsMenu& toolsMenu) {
    Input::Get().AddKeyCallback(-1, [](int keycode, int modifiers, void* userData) {
        static_cast<InputHandler*>(userData)->InputCallback(keycode, modifiers);
    }, true, this);

    tools = &toolsMenu.tools;
    InputCallback(GLFW_KEY_ESCAPE, 0);
}

void InputHandler::InputCallback(int keycode, int modifiers) {
    if(suppressed)
        return;

    switch(keycode) {
        case GLFW_KEY_ESCAPE:
            ENG_LOG_TRACE("SELECTION TOOL");
            ENGINE_IF_GUI(ImGui::SetWindowFocus(TABNAME_TOOL));
            tools->currentTool = &tools->selection;
            tools->toolName = ToolName::SELECT;
            break;
        case GLFW_KEY_1:        //painting tool
            ENG_LOG_TRACE("PAINTING TOOL");
            ENGINE_IF_GUI(ImGui::SetWindowFocus(TABNAME_TOOL));
            tools->currentTool = &tools->painting;
            tools->toolName = ToolName::TILE_PAINT;
            break;
        case GLFW_KEY_2:        //object placement tool
            ENG_LOG_TRACE("OBJECT PLACEMENT TOOL");
            ENGINE_IF_GUI(ImGui::SetWindowFocus(TABNAME_TOOL));
            tools->currentTool = &tools->placement;
            tools->toolName = ToolName::OBJECT_PLACEMENT;
            break;
        case GLFW_KEY_3:        //techtree tab
            ENG_LOG_TRACE("TECHTREE TAB");
            ENGINE_IF_GUI(ImGui::SetWindowFocus(TABNAME_TECHTREE));
            tools->currentTool = &tools->selection;
            tools->toolName = ToolName::SELECT;
            break;
        case GLFW_KEY_4:        //diplomacy tab
            ENG_LOG_TRACE("DIPLOMACY TAB");
            ENGINE_IF_GUI(ImGui::SetWindowFocus(TABNAME_DIPLOMACY));
            tools->currentTool = &tools->selection;
            tools->toolName = ToolName::SELECT;
            break;
        case GLFW_KEY_5:        //level info tab
            ENG_LOG_TRACE("LEVEL INFO TAB");
            ENGINE_IF_GUI(ImGui::SetWindowFocus(TABNAME_LEVELINFO));
            tools->currentTool = &tools->selection;
            tools->toolName = ToolName::SELECT;
            break;
        case GLFW_KEY_6:        //file tab
            ENG_LOG_TRACE("FILE TAB");
            ENGINE_IF_GUI(ImGui::SetWindowFocus(TABNAME_FILE));
            tools->currentTool = &tools->selection;
            tools->toolName = ToolName::SELECT;
            break;
        case GLFW_KEY_H:        //hotkeys tab
            ENG_LOG_TRACE("HOTKEYS TAB");
            ENGINE_IF_GUI(ImGui::SetWindowFocus(TABNAME_HOTKEYS));
            tools->currentTool = &tools->selection;
            tools->toolName = ToolName::SELECT;
            break;
        case GLFW_KEY_KP_ADD:
            if(tools->currentTool != nullptr)
                tools->currentTool->SignalUp();
            break;
        case GLFW_KEY_KP_SUBTRACT:
            if(tools->currentTool != nullptr)
                tools->currentTool->SignalDown();
            break;
    }
}

void InputHandler::Update() {
    Input& input = Input::Get();
    Camera& camera = Camera::Get();

    switch(input.lmb.state) {
        case 1:     //down
            //alt - switch between camera & tool control
            lmb_alt = input.alt;
            //track starting click pos for camera movement
            lmb_startingPos = camera.Position();
            lmb_startingMousePos = input.mousePos_n * 2.f - 1.f;
        case 2:     //pressed
            if(lmb_alt) {
                camera.PositionFromMouse(lmb_startingPos, lmb_startingMousePos, input.mousePos_n * 2.f - 1.f);
            }
            else if(tools->currentTool != nullptr) {
                tools->currentTool->OnLMB(input.lmb.state, lmb_startingPos);
            }
            break;
        case -1:    //up
            if(tools->currentTool != nullptr)
                tools->currentTool->Finalize(tools->opStack);
            break;
        case 0:     //released
            lmb_alt = false;
            if(tools->currentTool != nullptr) {
                tools->currentTool->OnHover();
            }
            break;
    }

    if(input.scroll.y) {
        if(input.ctrl && tools->currentTool != nullptr) {
            if(input.scroll.y > 0)
                tools->currentTool->SignalUp();
            else
                tools->currentTool->SignalDown();
        }
        else {
            camera.ZoomUpdate(true);
        }
    }
}

void RenderHotkeysTab() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Begin(TABNAME_HOTKEYS);
    float f = 0.1f * ImGui::GetWindowSize().x;

    ImGui::Text("1 - tile painting tool");
    ImGui::Text("2 - object placement tool");
    ImGui::Text("3 - techtree tab");
    ImGui::Text("4 - diplomacy tab");
    ImGui::Text("5 - level info tab");
    ImGui::Text("6 - file tab");
    ImGui::Text("h - hotkeys hint (this tab)");
    ImGui::Separator();

    ImGui::Text("ESC - selection tool");
    ImGui::Text("camera movement");
    ImGui::Indent(f);
    ImGui::Text("- WASD");
    ImGui::Text("- alt + LMB drag");
    ImGui::Unindent(f);
    ImGui::Text("scroll - zoom");

    ImGui::Text("brush size");
    ImGui::Indent(f);
    ImGui::Text("numpad-/numpad+");
    ImGui::Text("ctrl + scroll");
    ImGui::Unindent(f);

    ImGui::End();
#endif
}

std::string tileIdx2name(int idx) {
    static std::string names[] = {
        "GROUND", "MUD",
        "WALL_BROKEN", "ROCK_BROKEN", "TREES_FELLED",
        "WATER",
        "ROCK", "WALL_HU", "WALL_HU_DAMAGED", "WALL_OC", "WALL_OC_DAMAGED",
        "TREES"
    };
    if(idx >= TileType::COUNT)
        return std::string("INVALID TILE TYPE");
    else
        return names[idx];
}
