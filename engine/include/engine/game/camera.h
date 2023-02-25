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
    private:
        Camera() = default;
    private:
        glm::vec2 position = glm::vec2(0.f);
        glm::vec2 mult = glm::vec2(1.f);
        
        float zoom = 1.f;
        float zoom_log = 0.f;
    };

}//namespace eng
