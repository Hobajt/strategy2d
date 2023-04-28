#pragma once

#include <engine/engine.h>

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
private:
    int submenu = 0;
    bool err = false;
    std::string errMsg = "---";
};

class ToolsMenu {

};
