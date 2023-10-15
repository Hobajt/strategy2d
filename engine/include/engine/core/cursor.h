#pragma once

#include <vector>
#include <string>

struct GLFWcursor;

namespace eng {

    class CursorIconManager {
    public:
        CursorIconManager(const std::string& config_filepath);

        CursorIconManager() = default;
        ~CursorIconManager();

        //copy disabled
        CursorIconManager(const CursorIconManager&) = delete;
        CursorIconManager& operator=(const CursorIconManager&) = delete;

        //move enabled
        CursorIconManager(CursorIconManager&&) noexcept;
        CursorIconManager& operator=(CursorIconManager&&) noexcept;

        void UpdateIcon();

        void SetIcon(int idx_) { idx = idx_; }

        void DBG_GUI();
    private:
        void Move(CursorIconManager&&) noexcept;
        void Release() noexcept;

        void LoadFromConfig(const std::string& config_filepath);
    private:
        std::vector<GLFWcursor*> icons;
        int idx = -1;
    };

}//namespace eng
