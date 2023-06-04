#pragma once

#include <engine/engine.h>

#include <vector>
#include <string>

struct TilesetChoices {
    std::vector<std::string> names;
    std::vector<const char*> names_ptr;
public:
    const char* Default();
    const char* FindName(const char* name);
    void ReloadTilesets();
};

struct TilesetChoice {
    static TilesetChoices choices;
    const char* selection = nullptr;
public:
    TilesetChoice();
    void Reset();
    bool GUI_Combo();

    eng::TilesetRef LoadTilesetOrDefault(bool forceReload = false);
};

void RenderHotkeysTab();

//===== FileMenu =====

namespace FileMenuSignal { enum { QUIT=-1, NOTHING=0, NEW, LOAD, SAVE }; }

//Menu with map creation options (save/load/new).
class FileMenu {
public:
    FileMenu();
    ~FileMenu();

    int Update();

    void Reset();

    void SignalError(const std::string& msg);
private:
    int Submenu_New();
    int Submenu_Load();
    int Submenu_Save();
    void Submenu_Error();

    void FileDialog();
public:
    glm::ivec2 terrainSize;
    char* filepath;
    TilesetChoice tileset;
private:
    int submenu = 0;
    bool err = false;
    std::string errMsg = "---";
};

//===== LevelInfoMenu =====

//Menu to display/modify general level info - tileset, number of players
class LevelInfoMenu {
public:
    LevelInfoMenu();
    ~LevelInfoMenu();

    void SetLevelRef(eng::Level* level_) { level = level_; }

    void Update();

    void NewLevelCreated();
private:
    eng::Level* level;
    char* levelName;

    TilesetChoice tileset;
    bool tileset_forceReload = false;

    int maxPlayers = 2;
    std::vector<glm::ivec2> startingLocations;
    bool renderStartingLocations = true;
};

struct EditorOperation {
    struct Entry {
        glm::vec2 pos;
        int id;
    };
    bool painting;
    std::vector<Entry> entries;
};

class OpStack {
    //TODO: circular buffer
};

//===== EditorTool =====

class EditorTool {
public:
    virtual void Update() = 0;
    virtual void OnLMB(int lmbState, const glm::vec2& lmb_startPos) = 0;
    virtual void OnHover() = 0;

    virtual void SignalUp() {}
    virtual void SignalDown() {}

    virtual void Finalize(OpStack& opStack) {}
public:
    eng::Level* level = nullptr;
};

class PaintingTool : public EditorTool {
public:
    virtual void Update() override;
    virtual void OnLMB(int lmbState, const glm::vec2& lmb_startPos) override;
    virtual void OnHover() override;

    virtual void SignalUp() override;
    virtual void SignalDown() override;

    virtual void Finalize(OpStack& opStack) override;
private:
    void UpdateBrushSize(int newSize);
private:
    int brushSize = 1;
    int bl = 0;
    int br = 1;

    int selection = eng::TileType::GROUND;
    std::unordered_map<int,int> currentPaint;
};

class SelectionTool : public EditorTool {
public:
    virtual void Update() override;
    virtual void OnLMB(int lmbState, const glm::vec2& lmb_startPos) override;
    virtual void OnHover() override;
};

class ObjectPlacementTool : public EditorTool {
public:
    virtual void Update() override;
    virtual void OnLMB(int lmbState, const glm::vec2& lmb_startPos) override;
    virtual void OnHover() override;
};

namespace ToolName { enum { SELECT, TILE_PAINT, OBJECT_PLACEMENT }; }

struct EditorTools {
    PaintingTool painting;
    SelectionTool selection;
    ObjectPlacementTool placement;

    EditorTool* currentTool = nullptr;
    int toolName = ToolName::SELECT;

    OpStack opStack;
};

//===== ToolsMenu =====

class ToolsMenu {
public:
    void Init(eng::Level& level);

    void Update();
public:
    EditorTools tools;
};

//===== InputHandler =====

class InputHandler {
public:
    void Init(ToolsMenu& toolsMenu);

    void Update();

    void InputCallback(int keycode, int modifiers);
private:
    bool suppressed = false;

    bool lmb_alt = false;
    glm::vec2 lmb_startingPos;
    glm::vec2 lmb_startingMousePos;

    EditorTools* tools = nullptr;
};

