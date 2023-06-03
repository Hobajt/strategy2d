#include "menu.h"

#include <nfd.h>
#include <filesystem>

using namespace eng;

#define BUF_SIZE 1024

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
    ImGui::Begin("File");

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
}

LevelInfoMenu::~LevelInfoMenu() {
    delete[] levelName;
}

void LevelInfoMenu::Update() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Begin("Level Info");

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

    ImGui::End();
#endif
}

void LevelInfoMenu::NewLevelCreated() {
    tileset.selection = tileset.choices.FindName(level->map.GetTilesetName().c_str());
}