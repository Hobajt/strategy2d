#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>

#include <glad/glad.h>

#include "engine/core/texture.h"
#include "engine/utils/mathdefs.h"

namespace eng {

    //===== AttachmentParams =====

    struct AttachmentParams {
        GLenum slot = GL_NONE;
        TextureParams params;
    public:
        static AttachmentParams Depth(GLenum filtering = GL_LINEAR, GLenum wrapping = GL_CLAMP_TO_EDGE, const glm::vec4 borderColor = glm::vec4(0.f));
        static AttachmentParams Regular(int colorAttachmentIdx, GLenum internalFormat, GLenum format = GL_RGB, GLenum dtype = GL_UNSIGNED_BYTE, GLenum filtering = GL_LINEAR, GLenum wrapping = GL_CLAMP_TO_EDGE);
    };

    //===== Renderbuffer =====

    class Renderbuffer {
    public:
        Renderbuffer(int width, int height, GLenum internalFormat, const std::string& name = "renderbuffer");

        Renderbuffer() = default;
        ~Renderbuffer();

        //copy disabled
        Renderbuffer(const Renderbuffer&) = delete;
        Renderbuffer& operator=(const Renderbuffer&) = delete;

        //move enabled
        Renderbuffer(Renderbuffer&&) noexcept;
        Renderbuffer& operator=(Renderbuffer&&) noexcept;

        GLuint Handle() const;
    private:
        void Release() noexcept;
        void Move(Renderbuffer&&) noexcept;
    private:
        GLuint handle = 0;
        glm::ivec2 size = glm::ivec2(0);

        std::string name;
    };

    //===== Framebuffer =====

    //Wrapper for framebuffer handling.
    //Can use multiple texture attachments but only 1 renderbuffer attachment.
    class Framebuffer {
    public:
        Framebuffer(int width, int height, const AttachmentParams& attachment, const AttachmentParams& rboAttachment = {});
        Framebuffer(int width, int height, const std::vector<AttachmentParams>& attachments, const AttachmentParams& rboAttachment = {});

        Framebuffer() = default;
        ~Framebuffer();

        //copy disabled
        Framebuffer(const Framebuffer&) = delete;
        Framebuffer& operator=(const Framebuffer&) = delete;

        //move enabled
        Framebuffer(Framebuffer&&) noexcept;
        Framebuffer& operator=(Framebuffer&&) noexcept;

        void Bind() const;
        static void Unbind();

        //Binds FBO only for reading & only binds specific attachment.
        void ReadOnlyBind(GLenum slot) const;

        bool IsComplete() const;

        //Set all defined attachments in this object as draw buffers.
        void UpdateDrawBuffers();

        //Specify what attachments should be used for pixel output.
        void UpdateDrawBuffers(const std::vector<GLenum>& slots);

        TextureRef Attachment(GLenum i) const;
        TextureRef ColorAttachment(int i) const;
    private:
        void Release() noexcept;
        void Move(Framebuffer&&) noexcept;
    private:
        GLuint handle = 0;

        Renderbuffer rbo;
        std::map<GLenum, TextureRef> attachments;

        glm::ivec2 size = glm::ivec2(0);
        GLenum rbo_slot = GL_NONE;
    };
    using FramebufferRef = std::shared_ptr<Framebuffer>;

    //===== ClickSelectionHandler =====

    //Manages asynchronous transfer of object info from the GPU to CPU.
    //Utilizes pixel buffers for this purpose.
    class ClickSelectionHandler {
        friend class ClickSelectionController;
    public:
        ClickSelectionHandler(const FramebufferRef& fbo);

        ClickSelectionHandler() = default;
        ~ClickSelectionHandler();

        //copy disabled
        ClickSelectionHandler(const ClickSelectionHandler&) = delete;
        ClickSelectionHandler& operator=(const ClickSelectionHandler&) = delete;

        //move enabled
        ClickSelectionHandler(ClickSelectionHandler&&) noexcept;
        ClickSelectionHandler& operator=(ClickSelectionHandler&&) noexcept;
    
        //Initializes data transfer from GPU to CPU (for value at provided pixel coords).
        void FetchPixel(int x, int y);

        //Retrieves object info, previously loaded by FetchPixel().
        //The call will block until the transfer is completed (ie. calling right after FetchPixel()).
        glm::uvec4 Retrieve();
    private:
        FramebufferRef fbo;
        GLuint pbo;
        glm::uvec4 data;
    };

    //Wrapper for ClickSelectionHandler that provides additional functionality.
    class ClickSelectionController {
    public:
        ClickSelectionController(const FramebufferRef& fbo);

        ClickSelectionController() = default;
        ~ClickSelectionController();

        //copy disabled
        ClickSelectionController(const ClickSelectionController&) = delete;
        ClickSelectionController& operator=(const ClickSelectionController&) = delete;

        //move enabled
        ClickSelectionController(ClickSelectionController&&) noexcept;
        ClickSelectionController& operator=(ClickSelectionController&&) noexcept;

        void Register(bool transfer);
        void Update();
        void Finalize();

        bool IsRegistered() const { return transfer; }
        glm::ivec2 ClickPos() const { return clickPos; }
        glm::uvec4 Retrieve() const { return handler.data; }
    private:
        ClickSelectionHandler handler;
        glm::ivec2 clickPos;
        bool transfer;
    };

}//namespace eng
