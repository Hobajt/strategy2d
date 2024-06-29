#include "menus.h"

#include "context.h"

#include <nfd.h>
#include <filesystem>

using namespace eng;

#define BUF_SIZE 1024

//===== TilesetChoice =====

TilesetChoice::ChoicesList TilesetChoice::choices = {};

const char* TilesetChoice::ChoicesList::Default() {
    return FindName("summer");
}

const char* TilesetChoice::ChoicesList::FindName(const char* name) {
    for(size_t i = 0; i < names_ptr.size(); i++) {
        if(strcmp(names_ptr[i], name) == 0)
            return names_ptr[i];
    }
    return nullptr;
}

void TilesetChoice::ChoicesList::ReloadTilesets() {
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
#ifdef ENGINE_ENABLE_GUI
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
#endif
    return res;
}

eng::TilesetRef TilesetChoice::LoadTilesetOrDefault(bool forceReload) {
    return (selection != nullptr) ? Resources::LoadTileset(selection, forceReload) : Resources::DefaultTileset();
}

//===== EditorComponent =====

EditorComponent::EditorComponent(EditorContext& context_) : context(context_) {}

//===== HotkeysMenu =====

HotkeysMenu::HotkeysMenu(EditorContext& context_) : EditorComponent(context_) {}

void HotkeysMenu::GUI_Update() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Begin(HotkeysMenu::TabName());
    float f = 0.1f * ImGui::GetWindowSize().x;

    ImGui::Text("1 - tile painting tool");
    ImGui::Text("2 - object placement tool");
    ImGui::Text("3 - starting location tool");
    ImGui::Text("4 - techtree tab");
    ImGui::Text("5 - diplomacy tab");
    ImGui::Text("6 - level info tab");
    ImGui::Text("7 - file tab");
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

//===== FileMenu =====

FileMenu::FileMenu(EditorContext& context_) : EditorComponent(context_) {
    filepath = new char[BUF_SIZE];
    Reset();
}

FileMenu::~FileMenu() {
    delete[] filepath;
}

void FileMenu::GUI_Update() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Begin(FileMenu::TabName());

    //submenu choices
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
                Submenu_New();
                break;
            case 2:
                Submenu_Load();
                break;
            case 3:
                Submenu_Save();
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
        Window::Get().Close();
    }

    ImGui::End();
#endif
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

void FileMenu::Submenu_New() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Text("New Terrain");
    if(ImGui::DragInt2("Size", (int*)&terrainSize)) {
        if(terrainSize.x < 1) terrainSize.x = 1;
        if(terrainSize.y < 1) terrainSize.y = 1;
    }
    tileset.GUI_Combo();

    if(ImGui::Button("Generate")) {
        context.Terrain_SetupNew(terrainSize, tileset.LoadTilesetOrDefault());
        Reset();
    }
#endif
}

void FileMenu::Submenu_Load() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Text("Load from File (expects absolute filepath)");
    ImGui::InputText("", filepath, sizeof(char) * BUF_SIZE);
    ImGui::SameLine();
    if(ImGui::Button("Browse")) {
        FileDialog();
    }

    if(ImGui::Button("Load")) {
        switch(context.Terrain_Load(std::string(filepath))) {
            case 0:
                Reset();
                break;
            case 1:
                SignalError("File not found.");
                break;
            case 2:
                SignalError("Failed to parse the file.");
                break;
        }
    }
#endif
}

void FileMenu::Submenu_Save() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Text("Save Terrain (expects absolute filepath)");
    ImGui::InputText("", filepath, sizeof(char) * BUF_SIZE);
    ImGui::SameLine();
    if(ImGui::Button("Browse")) {
        FileDialog();
    }

    if(ImGui::Button("Save")) {
        if(context.Terrain_Save(std::string(filepath)) != 0)
            SignalError("No clue what happened.");
        else
            Reset();
    }
#endif
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
        else if(strncpy(filepath, outPath, len)) {
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

//===== InfoMenu =====

InfoMenu::InfoMenu(EditorContext& context_) : EditorComponent(context_) {
    levelName = new char[BUF_SIZE];
    snprintf(levelName, sizeof(char) * BUF_SIZE, "%s", "unnamed_level");
}

InfoMenu::~InfoMenu() {
    delete[] levelName;
}

void InfoMenu::GUI_Update() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Begin(InfoMenu::TabName());

    Level& level = context.level;

    ImGui::Text("Level info");
    ImGui::InputText("Name", levelName, sizeof(char) * BUF_SIZE);
    glm::ivec2 size = level.map.Size();
    ImGui::Text("Size: [%d x %d]", size.x, size.y);
    ImGui::SliderInt("Preferred opponents", &level.info.preferred_opponents, 1, 7);
    ImGui::Separator();
    ImGui::Text("Tileset");
    if(tileset.GUI_Combo()) {
        level.map.ChangeTileset(tileset.LoadTilesetOrDefault(tileset_forceReload));
    }
    ImGui::Checkbox("Forced reload", &tileset_forceReload);
    ImGui::End();
#endif
}

void InfoMenu::NewLevelCreated() {
    tileset.selection = tileset.choices.FindName(context.level.map.GetTilesetName().c_str());
}

//===== ToolsMenu =====

ToolsMenu::ToolsMenu(EditorContext& context_) : EditorComponent(context_) {}

void ToolsMenu::GUI_Update() {
#ifdef ENGINE_ENABLE_GUI
    ImGui::Begin(ToolsMenu::TabName());

    GUI_ToolSelect();
    
    context.tools.GUI_Update();

    ImGui::End();
#endif
}

void ToolsMenu::GUI_ToolSelect() {
#ifdef ENGINE_ENABLE_GUI
    const auto& tools = context.tools.GetTools();
    if(tools.size() <= 4) {
        //render as separate buttons
        bool first = true;
        for(const auto& [type, tool] : tools) {
            if(!first) ImGui::SameLine();
            if(ImGui::Button(toolType2str(type), ImVec2(ImGui::GetWindowSize().x * 0.23f, 0.f))) {
                context.tools.SwitchTool(type);
            }
            first = false;
        }
    }
    else {
        //render in a  ListBox
        ImGui::PushID(0);
        if(ImGui::BeginListBox("Tools:")) {
            for(const auto& [type, tool] : tools) {
                if(ImGui::Selectable(toolType2str(type), context.tools.IsToolSelected(type))) {
                    context.tools.SwitchTool(type);
                }
            }
            ImGui::EndListBox();
        }
        ImGui::PopID();
    }
    ImGui::Separator();
#endif
}

//===== FactionsMenu =====

FactionsMenu::FactionsMenu(EditorContext& context_) : EditorComponent(context_) {}

void FactionsMenu::GUI_Update() {
    ImGui::Begin(FactionsMenu::TabName());
    Level& level = context.level;

    ImGui::Text("Game type:");
    ImGui::SameLine();
    bool& is_custom = level.info.custom_game;
    if(ImGui::Button(is_custom ? "Custom" : "Scenario")) {
        is_custom = !is_custom;
    }
    ImGui::Separator();

    if(!is_custom) {
        Factions& factions = level.factions;

        //slider that creates/deletes faction entries
        if(ImGui::SliderInt("faction count", &factionCount, 2, 12)) {
            UpdateFactionCount(factions, factionCount);
        }

        //recreate nature & player factions if somehow deleted (or initially)
        bool has_player, has_nature;
        CheckForRequiredFactions(factions, has_player, has_nature);
        if(!has_nature) AddFaction(factions, "Nature", FactionControllerID::NATURE, 5);
        if(!has_player) AddFaction(factions, "Player", FactionControllerID::LOCAL_PLAYER, 0);

        //display editable entry for each faction
        char buf[1024];
        int faction_idx = 0;
        std::vector<int> to_delete = {};
        for(auto& faction : factions) {
            FactionController& f = *faction.get();
            bool mandatory = f.ControllerID() == FactionControllerID::LOCAL_PLAYER || f.ControllerID() == FactionControllerID::NATURE;
            std::string name = f.Name();
            snprintf(buf, sizeof(buf), "[%d] %s###%lu", faction_idx, name.c_str(), uint64_t(&f));
            if(ImGui::CollapsingHeader(buf)) {
                ImGui::PushID(&f);
                strncpy(buf, name.c_str(), std::min(int(sizeof(buf)), int(name.size()+1)));
                if(ImGui::InputText("Name", buf, sizeof(buf))) {
                    Faction_Name(f, buf);
                }

                ImGui::SliderInt("Color", &Faction_Color(f), 0, ColorPalette::FactionColorCount());

                ImGui::Text("Race:"); ImGui::SameLine();
                ImGui::RadioButton("Human", &Faction_Race(f), 0); ImGui::SameLine();
                ImGui::RadioButton("Orc", &Faction_Race(f), 1);

                if(ImGui::CollapsingHeader("Techtree")) {
                    f.Tech().EditableGUI();
                }

                if(!mandatory) {
                    if(ImGui::SliderInt("Controller type", &Faction_ControllerType(f), 3, FactionControllerID::COUNT)) {
                        //prevents from switching to player/nature factions (hides the delete btn)
                        Faction_ControllerType(f) = std::max(3, Faction_ControllerType(f));
                    }
                }

                if(!mandatory) {
                    if(ImGui::Button("Delete faction")) {
                        to_delete.push_back(faction_idx);
                    }
                }

                ImGui::PopID();
            }
            faction_idx++;
        }

        //remove factions marked by clicking the delete button
        for(int i = to_delete.size()-1; i >= 0; i--) {
            RemoveFaction(factions, to_delete[i]);
            factionCount--;
        }

        ImGui::Separator();

        //display editable diplomacy matrix
        if(ImGui::CollapsingHeader("Diplomacy matrix")) {
            Diplomacy(factions).EditableGUI(factionCount);
        }
    }
    else {
        ImGui::Text("Diplomacy can only be defined for non-custom games...");
    }

    ImGui::End();
}
