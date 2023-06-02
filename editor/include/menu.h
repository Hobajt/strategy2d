#pragma once

#include <engine/engine.h>

#include <vector>
#include <string>

struct TilesetChoices {
    std::vector<std::string> names;
    std::vector<const char*> names_ptr;
public:
    const char* Default();
    void ReloadTilesets();
    void GUI_Combo(const char*& selection);
};


//===== FileMenu =====

namespace FileMenuSignal { enum { QUIT=-1, NOTHING=0, NEW, LOAD, SAVE }; }

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
    const char* tilesetName = nullptr;
private:
    int submenu = 0;
    bool err = false;
    std::string errMsg = "---";

    TilesetChoices tilesets;
};

//===== ToolsMenu =====

class ToolsMenu {

};

//===== LevelInfoMenu =====

class LevelInfoMenu {
public:
    LevelInfoMenu();
    ~LevelInfoMenu();

    void SetLevelRef(eng::Level* level_) { level = level_; }

    void Update();
private:
    eng::Level* level;
    char* levelName;
};
