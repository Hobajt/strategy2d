#include "engine/game/camera.h"

#include "engine/utils/dbg_gui.h"
#include "engine/core/input.h"
#include "engine/core/window.h"
#include "engine/core/audio.h"

#include "engine/game/config.h"

//TODO: reorganize camera constants

//speed constants
static constexpr float CAM_ZOOM_SPEED = 10.f;
static constexpr float CAM_MOVE_SPEED = 5.f;

//zoom limit in log10
static constexpr float CAM_ZOOM_LOG_RANGE = 4.f;

//zoom limits in normal space
static const float CAM_ZOOM_MIN = std::pow(10, -CAM_ZOOM_LOG_RANGE);
static const float CAM_ZOOM_MAX = std::pow(10,  CAM_ZOOM_LOG_RANGE);

namespace eng {

    Camera& Camera::Get() {
        static Camera instance = {};
        return instance;
    }

    void Camera::Update() {
        Input& input = Input::Get();

        // position += (input.move1 * input.deltaTime_real * CAM_MOVE_SPEED) / zoom;

        //discrete movement, controlled by arrow keys
        if(input.isMoveArrows) {
            arrows_movement_acc += (input.deltaTime_real * Config::Map_KeySpeed()) / zoom;
            while(arrows_movement_acc >= 1.f) {
                position += input.move_arrows;
                arrows_movement_acc -= 1.f;
            }
        }

        ZoomUpdate();

        if(checkForBounds)
            BoundariesCheck();

        
        Audio::UpdateListenerPosition(position);
    }

    void Camera::Move(const glm::vec2& step) {
        mouse_movement_acc += (Input::Get().deltaTime_real * Config::Map_MouseSpeed()) / zoom;
        while(mouse_movement_acc >= 1.f) {
            position += step;
            mouse_movement_acc -= 1.f;
        }

        // movement_acc += Input::Get().deltaTime_real;

        // float thresh = Config::Map_MouseSpeed();
        // while(movement_acc >= thresh) {
        //     position += step;
        //     movement_acc -= thresh;
        // }

        if(checkForBounds)
            BoundariesCheck();
        
        Audio::UpdateListenerPosition(position);
    }

    void Camera::Center(const glm::vec2& pos) {
        position = pos;
        
        if(checkForBounds)
            BoundariesCheck();
        
        Audio::UpdateListenerPosition(position);
    }

    void Camera::ZoomUpdate(bool forced) {
        Input& input = Input::Get();

        if(enableZoomUpdates || forced) {
            zoom_log += input.scroll.y * input.deltaTime_real * 10.f;
            if(zoom_log < -CAM_ZOOM_LOG_RANGE) zoom_log = -CAM_ZOOM_LOG_RANGE;
            else if(zoom_log > CAM_ZOOM_LOG_RANGE) zoom_log = CAM_ZOOM_LOG_RANGE;
            zoom = std::pow(10, zoom_log);
        }

        UpdateMultiplier();
    }

    void Camera::UpdateMultiplier() {
        UpdateMultiplier(Window::Get().Aspect());
    }

    void Camera::UpdateMultiplier(const glm::vec2& aspect) {
        mult = Window::Get().Aspect() * zoom;
    }

    std::pair<glm::ivec2, glm::ivec2> Camera::RectangleCoords() const {
        glm::vec2 screen_half = 1.f / mult;
        float xOff = GUI_bounds_offset / mult.x;

        glm::ivec2 a = glm::ivec2(position - screen_half);
        glm::ivec2 b = glm::ivec2(position + screen_half);
        a.x += xOff;

        a = glm::clamp(a, glm::ivec2(0), glm::ivec2(bounds+1.f));
        b = glm::clamp(b, glm::ivec2(0), glm::ivec2(bounds+1.f));

        return { a, b };
    }

    void Camera::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        ImGui::Begin("Camera");

        ImGui::SliderFloat2("position", (float*)&position, -10.f, 10.f, "%.2f");
        ImGui::DragFloat("GUI bounds offset", &GUI_bounds_offset);

        ImGui::Separator();
        if(ImGui::SliderFloat("zoom", &zoom, CAM_ZOOM_MIN, CAM_ZOOM_MAX, "%.4f", ImGuiSliderFlags_Logarithmic)) {
            Zoom(zoom);
        }
        ImGui::SliderFloat("zoom_log", &zoom_log, -CAM_ZOOM_LOG_RANGE, CAM_ZOOM_LOG_RANGE, "%.4f");
        ImGui::Checkbox("scroll to zoom", &enableZoomUpdates);

        ImGui::Separator();
        static glm::vec2 center_target = glm::vec2(0.f);
        ImGui::Text("Camera Centering");
        ImGui::DragFloat2("location", (float*)&center_target);
        if(ImGui::Button("Center at location")) {
            position = center_target;
        }

        ImGui::Separator();
        ImGui::Checkbox("boundaries", &checkForBounds);
        ImGui::DragFloat2("bounds", (float*)&bounds);

        ImGui::End();
#endif
    }

    void Camera::Zoom(float zoom_) {
        zoom_log = std::log10(zoom_);
        zoom = zoom_;
        UpdateMultiplier();
    }

    void Camera::ZoomLog(float zoom_log_) {
        zoom_log = zoom_log_;
        zoom = std::pow(10, zoom_log);
        UpdateMultiplier();
    }

    void Camera::ZoomToFit(const glm::vec2& bounds) {
        glm::vec2 v = 2.f / (bounds * Window::Get().Aspect());
        Zoom(std::min(v.x, v.y));
    }

    glm::vec2 Camera::GetMapCoords(const glm::vec2& mousePos_ndc) const {
        return GetMapCoords(position, mousePos_ndc);
    }

    glm::vec2 Camera::GetMapCoords(const glm::vec2& position, const glm::vec2& mousePos_ndc) const {
        glm::vec2 n = glm::vec2(mousePos_ndc.x, -mousePos_ndc.y);
        return position + n / mult;
    }

    glm::vec2 Camera::map2screen(const glm::vec2& pos_map) const {
        return (pos_map - 0.5f - position) * mult;
    }

    glm::vec2 Camera::w2s(const glm::vec2& pos_world, const glm::vec2& size_world) const {
        return (pos_world - size_world * 0.5f - position) * mult;
    }

    void Camera::PositionFromMouse(const glm::vec2& anchor, const glm::vec2& mousePos_start, const glm::vec2& mousePos_now) {
        position = GetMapCoords(anchor, mousePos_start - mousePos_now);
    }

    void Camera::BoundariesCheck() {
        glm::vec2 screen_half = 1.f / mult;
        glm::vec2 b = bounds - screen_half - 0.5f;
        float xOff = GUI_bounds_offset / mult.x;

        glm::vec2 v = screen_half * 2.f;
        if(v.x < bounds.x && v.y < bounds.y) {
            if(position.x > b.x)
                position.x = b.x;
            else if (position.x < screen_half.x - 0.5f - xOff)
                position.x = screen_half.x - 0.5f - xOff;

            if(position.y > b.y)
                position.y = b.y;
            else if (position.y < screen_half.y - 0.5f)
                position.y = screen_half.y - 0.5f;
        }
        else {
            //zoomed too far out - entire bounding box visible (setting fixed position)
            position = bounds * 0.5f - 0.5f;
        }
    }

}//namespace eng
