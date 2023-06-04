#pragma once

#include <engine/engine.h>

#include <vector>
#include <string>
#include <memory>

struct EditorContext;

//===== TilesetChoice =====

//GUI combo box with tileset choices.
struct TilesetChoice {
    struct ChoicesList {
        std::vector<std::string> names;
        std::vector<const char*> names_ptr;
    public:
        const char* Default();
        const char* FindName(const char* name);
        void ReloadTilesets();
    };

    static ChoicesList choices;
    const char* selection = nullptr;
public:
    TilesetChoice();
    void Reset();
    bool GUI_Combo();

    eng::TilesetRef LoadTilesetOrDefault(bool forceReload = false);
};

//===== EditorComponent =====

class EditorComponent {
public:
    EditorComponent(EditorContext& context);
    EditorComponent(EditorContext&& context) = delete;

    virtual void GUI_Update() = 0;
protected:
    EditorContext& context;
};
using EditorComponentRef = std::shared_ptr<EditorComponent>;

//===== HotkeysMenu =====

//Displays a list of hotkeys.
class HotkeysMenu : public EditorComponent {
public:
    HotkeysMenu(EditorContext& context);

    static const char* TabName() { return "Hotkeys"; }

    virtual void GUI_Update() override;
};

//===== FileMenu =====

//Options to save/load or setup a new map.
class FileMenu : public EditorComponent {
public:
    FileMenu(EditorContext& context);
    ~FileMenu();

    static const char* TabName() { return "File"; }

    virtual void GUI_Update() override;

    void Reset();
private:
    void Submenu_New();
    void Submenu_Load();
    void Submenu_Save();
    void Submenu_Error();

    void SignalError(const std::string& msg);

    void FileDialog();
private:
    int submenu = 0;
    bool err = false;
    std::string errMsg = "---";

    glm::ivec2 terrainSize;
    char* filepath;
    TilesetChoice tileset;
};

//===== InfoMenu =====

class InfoMenu : public EditorComponent {
public:
    InfoMenu(EditorContext& context);
    ~InfoMenu();

    virtual void GUI_Update() override;

    static const char* TabName() { return "Level Info"; }

    void NewLevelCreated();
private:
    char* levelName;

    int maxPlayers = 2;
    bool renderStartingLocations = true;
    std::vector<glm::ivec2> startingLocations;

    TilesetChoice tileset;
    bool tileset_forceReload = false;
};

//===== ToolsMenu =====

class ToolsMenu : public EditorComponent {
public:
    ToolsMenu(EditorContext& context);

    static const char* TabName() { return "Tools"; }

    virtual void GUI_Update() override;
private:
    void GUI_ToolSelect();
};

//===== TechtreeMenu =====

class TechtreeMenu : public EditorComponent {
public:
    TechtreeMenu(EditorContext& context);

    static const char* TabName() { return "Techtree"; }

    virtual void GUI_Update() override;
};

//===== DiplomacyMenu =====

class DiplomacyMenu : public EditorComponent {
public:
    DiplomacyMenu(EditorContext& context);

    static const char* TabName() { return "Diplomacy"; }

    virtual void GUI_Update() override;
};
