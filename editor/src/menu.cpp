#include "menu.h"

#include <nfd.h>

using namespace eng;

#define BUF_SIZE 1024

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
}

void FileMenu::SignalError(const std::string& msg) {
    errMsg = msg;
    err = true;
}

int FileMenu::Submenu_New() {
    ImGui::Text("New Terrain");
    if(ImGui::DragInt2("Size", (int*)&terrainSize)) {
        if(terrainSize.x < 1) terrainSize.x = 1;
        if(terrainSize.y < 1) terrainSize.y = 1;
    }
    //TODO: add tileset option
    if(ImGui::Button("Generate")) {
        return FileMenuSignal::NEW;
    }
    return FileMenuSignal::NOTHING;
}

int FileMenu::Submenu_Load() {
    ImGui::Text("Load from File (expects absolute filepath)");
    ImGui::InputText("", filepath, sizeof(char) * BUF_SIZE);
    ImGui::SameLine();
    if(ImGui::Button("Browse")) {
        FileDialog();
    }

    if(ImGui::Button("Load")) {
        return FileMenuSignal::LOAD;
    }

    return FileMenuSignal::NOTHING;
}

int FileMenu::Submenu_Save() {
    ImGui::Text("Save Terrain (expects absolute filepath)");
    ImGui::InputText("", filepath, sizeof(char) * BUF_SIZE);
    ImGui::SameLine();
    if(ImGui::Button("Browse")) {
        FileDialog();
    }

    if(ImGui::Button("Save")) {
        return FileMenuSignal::SAVE;
    }
    return FileMenuSignal::NOTHING;
}

void FileMenu::Submenu_Error() {
    ImGui::Text("Operation could not be completed due to an error.");
    ImGui::Text("Reason: %s", errMsg.c_str());
    if(ImGui::Button("OK")) {
        err = false;
    }
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