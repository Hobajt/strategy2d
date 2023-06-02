#pragma once

#include "engine/utils/mathdefs.h"

namespace eng {

    //Singleton.
    class Camera {
    public:
        static Camera& Get();

        //copy deleted
        Camera(const Camera&) = delete;
        Camera& operator=(const Camera&) = delete;

        //move deleted
        Camera(Camera&&) noexcept = delete;
        Camera& operator=(Camera&&) noexcept = delete;

        //Camera update method. Relies on values from Input class.
        void Update();

        void UpdateMultiplier();
        void UpdateMultiplier(const glm::vec2& aspect);

        void DBG_GUI();

        //=== getters/setters ===

        void Position(const glm::vec2& pos) { position = pos; }
        glm::vec2 Position() const { return position; }
        glm::vec3 Position3d() const { return glm::vec3(position, 0.f); }

        glm::vec2 Mult() const { return mult; }
        glm::vec3 Mult3d() const { return glm::vec3(mult, 1.f); }

        void Zoom(float zoom_);
        void ZoomLog(float zoom_log_);

        void EnableBoundaries(bool enabled) { checkForBounds = enabled; }
        void SetBounds(const glm::vec2& bounds_) { bounds = bounds; }
    private:
        Camera() = default;

        void BoundariesCheck();
    private:
        glm::vec2 position = glm::vec2(0.f);
        glm::vec2 mult = glm::vec2(1.f);
        
        float zoom = 1.f;
        float zoom_log = 0.f;

        bool checkForBounds = true;
        glm::vec2 bounds = glm::vec2(10.f);
    };

}//namespace eng
