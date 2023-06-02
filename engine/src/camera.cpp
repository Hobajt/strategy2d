#include "engine/game/camera.h"

#include "engine/utils/dbg_gui.h"
#include "engine/core/input.h"
#include "engine/core/window.h"

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

        position += (input.move1 * input.deltaTime * CAM_MOVE_SPEED) / zoom;

        zoom_log += input.scroll.y * input.deltaTime * 10.f;
        if(zoom_log < -CAM_ZOOM_LOG_RANGE) zoom_log = -CAM_ZOOM_LOG_RANGE;
        else if(zoom_log > CAM_ZOOM_LOG_RANGE) zoom_log = CAM_ZOOM_LOG_RANGE;
        zoom = std::pow(10, zoom_log);
        UpdateMultiplier();

        if(checkForBounds)
            BoundariesCheck();
    }

    void Camera::UpdateMultiplier() {
        UpdateMultiplier(Window::Get().Aspect());
    }

    void Camera::UpdateMultiplier(const glm::vec2& aspect) {
        mult = Window::Get().Aspect() * zoom;
    }

    void Camera::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        ImGui::Begin("Camera");

        ImGui::SliderFloat2("position", (float*)&position, -10.f, 10.f, "%.2f");

        ImGui::Separator();
        if(ImGui::SliderFloat("zoom", &zoom, CAM_ZOOM_MIN, CAM_ZOOM_MAX, "%.4f", ImGuiSliderFlags_Logarithmic)) {
            Zoom(zoom);
        }
        ImGui::SliderFloat("zoom_log", &zoom_log, -CAM_ZOOM_LOG_RANGE, CAM_ZOOM_LOG_RANGE, "%.4f");

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

    void Camera::BoundariesCheck() {
        glm::vec2 screen_half = 1.f / mult;
        glm::vec2 b = bounds - screen_half - 0.5f;

        glm::vec2 v = screen_half * 2.f;
        if(v.x < bounds.x && v.y < bounds.y) {
            if(position.x > b.x)
                position.x = b.x;
            else if (position.x < screen_half.x - 0.5f)
                position.x = screen_half.x - 0.5f;

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
