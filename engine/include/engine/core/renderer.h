#pragma once

#include "engine/core/quad.h"
#include "engine/core/shader.h"
#include "engine/core/framebuffer.h"

namespace eng {

    //Manages quad batch rendering to the screen.
    namespace Renderer {

        //Stats for the ongoing (or last) rendering session. Resets with Begin() calls.
        struct RenderStats {
            int drawCalls = 0;
            int totalQuads = 0;
            
            int wastedQuads = 0;        //unused capacity of the internal quad buffer (ignoring the last call; high number = too many textures)
        };

        //Fetches renderer stats structure of ongoing/last rendering session.
        const RenderStats& Stats();

        void StatsReset();

        int TextureSlotsCount();

        //======================

        //Initialization call. Can be omitted, since initialization is checked at each session start.
        void Initialize();

        //Cleanup call, to release all the internally allocated resources.
        void Release();

        //======================

        //Signals a beginning of the rendering session.
        //    shader - shader, which should be used for this session; cannot be nullptr
        //       fbo - output framebuffer used for this render session; use nullptr for default framebuffer
        //  clearFBO - clears the specified framebuffer before the rendering starts
        void Begin(const ShaderRef& shader, const FramebufferRef& fbo = nullptr, bool clearFBO = true);
        void Begin(const ShaderRef& shader, bool clearFBO, const FramebufferRef& fbo = nullptr);

        //Ends the rendering session, triggers flush.
        void End();

        //Flushes all currently queued quads for rendering. Triggers a draw call.
        void Flush();

        //======================

        //Queue a quad for rendering within active rendering session.
        void RenderQuad(const Quad& quad);

        //Enables or disables wireframe rendering mode.
        void WireframeMode(bool enabled);

        //Binds texture to specific slot for the ongoing render call. Returns the index of assigned slot (or -1 if all slots are forcefully assigned).
        //Force-binded textures stay bound through flushes.
        int ForceBindTexture(const TextureRef& texture);

    }//namespace Renderer

}//namespace eng
