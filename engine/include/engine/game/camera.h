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

        //Update zoom based on scroll input.
        void ZoomUpdate(bool forced = false);

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

        void ZoomToFit(const glm::vec2& bounds);

        void SetupZoomUpdates(bool enabled) { enableZoomUpdates = enabled; }

        void EnableBoundaries(bool enabled) { checkForBounds = enabled; }
        void SetBounds(const glm::vec2& bounds_) { bounds = bounds_; }

        //Mapping from screen space to map coords. mousePos_n should be in NDC (<-1,1> range).
        glm::vec2 GetMapCoords(const glm::vec2& mousePos_ndc) const;
        glm::vec2 GetMapCoords(const glm::vec2& position, const glm::vec2& mousePos_ndc) const;

        glm::vec2 map2screen(const glm::vec2& pos_map) const;
        glm::vec2 w2s(const glm::vec2& pos_world, const glm::vec2& size_world) const;

        //Sets camera position based on offset of given position from the anchor. Anchor is in map coords.
        void PositionFromMouse(const glm::vec2& anchor, const glm::vec2& mousePos_start, const glm::vec2& mousePos_now);
    private:
        Camera() = default;

        void BoundariesCheck();
    private:
        glm::vec2 position = glm::vec2(0.f);
        glm::vec2 mult = glm::vec2(1.f);
        
        float zoom = 1.f;
        float zoom_log = 0.f;
        bool enableZoomUpdates = false;

        bool checkForBounds = true;
        glm::vec2 bounds = glm::vec2(10.f);
    };

}//namespace eng
