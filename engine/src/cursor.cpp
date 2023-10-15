#include "engine/core/cursor.h"

#include "engine/core/window.h"
#include "engine/core/texture.h"
#include "engine/utils/json.h"
#include "engine/utils/log.h"
#include "engine/utils/utils.h"
#include "engine/utils/mathdefs.h"
#include "engine/utils/dbg_gui.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace eng {

    CursorIconManager::CursorIconManager(const std::string& config_filepath) {
        LoadFromConfig(config_filepath);
    }

    CursorIconManager::~CursorIconManager() {
        Release();
    }

    CursorIconManager::CursorIconManager(CursorIconManager&& c) noexcept {
        Move(std::move(c));
    }

    CursorIconManager& CursorIconManager::operator=(CursorIconManager&& c) noexcept {
        Release();
        Move(std::move(c));
        return *this;
    }

    void CursorIconManager::UpdateIcon() {
        //not sure I'm doing it right (in the doc, it says that the cursor stays until you move out of window, but I've got to re-call this every frame)
        if(idx >= 0) {
            if(idx >= icons.size()) {
                ENG_LOG_WARN("CursorIconManager::UpdateIcon - invalid icon idx ({})", idx);
                idx = -1;
            }
            else {
                glfwSetCursor(Window::Get().Handle(), icons[idx]);
            }
        }
    }

    void CursorIconManager::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        ImGui::Begin("Cursors");

        ImGui::Text("Cursor count: %d", (int)icons.size());
        ImGui::SliderInt("idx", &idx, 0, (int)icons.size()-1);

        ImGui::End();
#endif
    }

    void CursorIconManager::Move(CursorIconManager&& c) noexcept {
        icons = std::move(c.icons);
        idx = c.idx;
        c.idx = -1;
    }

    void CursorIconManager::Release() noexcept {
        for(GLFWcursor* cursor : icons) {
            glfwDestroyCursor(cursor);
        }
        icons.clear();
        idx = -1;
    }

    void CursorIconManager::LoadFromConfig(const std::string& config_filepath) {
        using json = nlohmann::json;

        auto config = json::parse(ReadFile(config_filepath.c_str()));
        for(auto& entry : config) {
            std::string filepath            = entry.at("filepath");
            glm::ivec2 icon_size_global     = eng::json::parse_ivec2(entry.at("icon_size"));
            auto& icon_descs                = entry.at("icons");

            Image img                       = Image(filepath);

            int i = 0;
            for(auto& icon_desc : icon_descs) {
                glm::ivec2 pos          = eng::json::parse_ivec2(icon_desc.at("pos"));
                glm::ivec2 size         = eng::json::parse_ivec2(icon_desc.at("size"));
                glm::ivec2 hotspot      = icon_desc.count("hotspot") ? eng::json::parse_ivec2(icon_desc.at("hotspot")) : glm::ivec2(0, 0);
                glm::ivec2 icon_size    = icon_desc.count("icon_size") ? eng::json::parse_ivec2(icon_desc.at("icon_size")) : icon_size_global;

                Image subimg = img(pos.y, pos.x, size.y, size.x);

                GLFWimage image;
                image.width = icon_size.x;
                image.height = icon_size.y;
                image.pixels = subimg.ptr();
                //create sub-image from the original image and pass it as pixels

                GLFWcursor* cursor = glfwCreateCursor(&image, hotspot.x, hotspot.y);
                if(cursor == nullptr) {
                    ENG_LOG_WARN("CursorIconManager::LoadFromConfig - failed to create cursor image (file '{}')", filepath);
                    continue;
                }

                icons.push_back(cursor);
            }
        }
    }

}//namespace eng
