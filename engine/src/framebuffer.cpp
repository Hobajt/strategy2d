#include "engine/core/framebuffer.h"
#include "engine/core/window.h"

namespace eng {

    //===== AttachmentParams =====

    AttachmentParams AttachmentParams::Depth(GLenum filtering, GLenum wrapping, const glm::vec4 borderColor) {
        AttachmentParams ap = {};
        ap.slot = GL_DEPTH_ATTACHMENT;
        ap.params.internalFormat = GL_DEPTH_COMPONENT;
        ap.params.format = GL_DEPTH_COMPONENT;
        ap.params.filtering = filtering;
        ap.params.wrapping = wrapping;
        ap.params.borderColor = borderColor;
        return ap;
    }

    AttachmentParams AttachmentParams::Regular(int colorAttachmentIdx, GLenum internalFormat, GLenum format, GLenum dtype, GLenum filtering, GLenum wrapping) {
        AttachmentParams ap = {};
        ap.slot = GL_COLOR_ATTACHMENT0 + colorAttachmentIdx;
        ap.params = TextureParams::CustomData(0, 0, internalFormat, format, dtype, filtering, wrapping);
        return ap;
    }

    //===== Renderbuffer =====

    Renderbuffer::Renderbuffer(int width, int height, GLenum internalFormat, const std::string& name) : size(glm::ivec2(width, height)) {
        glGenRenderbuffers(1, &handle);
        glBindRenderbuffer(GL_RENDERBUFFER, handle);

        glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, width, height);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
        ENG_LOG_TRACE("[C] Renderbuffer '{}' ({})", name.c_str(), handle);
    }

    Renderbuffer::~Renderbuffer() {
        Release();
    }

    Renderbuffer::Renderbuffer(Renderbuffer&& rbo) noexcept {
        Move(std::move(rbo));
    }

    Renderbuffer& Renderbuffer::operator=(Renderbuffer&& rbo) noexcept {
        Release();
        Move(std::move(rbo));
        return *this;
    }

    GLuint Renderbuffer::Handle() const {
        return handle;
    }

    void Renderbuffer::Release() noexcept {
        if (handle != 0) {
            ENG_LOG_TRACE("[D] Renderbuffer '{}' ({})", name.c_str(), handle);
            glDeleteRenderbuffers(1, &handle);
            handle = 0;
        }
    }

    void Renderbuffer::Move(Renderbuffer&& rbo) noexcept {
        handle = rbo.handle;
        size = rbo.size;
        name = rbo.name;

        rbo.handle = 0;
    }

    //===== Framebuffer =====

    Framebuffer::Framebuffer(int width, int height, const AttachmentParams& attachment, const AttachmentParams& rboAttachment)
        : Framebuffer(width, height, std::vector<AttachmentParams>{ attachment }, rboAttachment) {}

    Framebuffer::Framebuffer(int width, int height, const std::vector<AttachmentParams>& attDesc, const AttachmentParams& rboAttachment)
        : size(glm::ivec2(width, height)) {
        glGenFramebuffers(1, &handle);
        glBindFramebuffer(GL_FRAMEBUFFER, handle);

        //create each attachment and attach it
        char buf[256];
        for (auto& desc : attDesc) {
            snprintf(buf, sizeof(buf), "fbo%d_texture_slot%d", handle, desc.slot);

            TextureParams params = desc.params;
            params.width = width;
            params.height = height;

            TextureRef tex = std::make_shared<Texture>(params, nullptr, std::string(buf));
            
            if (attachments.count(desc.slot) != 0) {
                ENG_LOG_WARN("Framebuffer {} - attachment slot conflicts during initialization (slot {}).", handle, desc.slot);
            }
            attachments[desc.slot] = tex;

            glFramebufferTexture2D(GL_FRAMEBUFFER, desc.slot, GL_TEXTURE_2D, tex->Handle(), 0);
        }

        //setup rbo attachment (if defined)
        if (rboAttachment.slot != GL_NONE) {
            rbo_slot = rboAttachment.slot;

            if (attachments.count(rbo_slot) != 0) {
                ENG_LOG_WARN("Framebuffer {} - attachment slot conflicts during initialization (slot {}; texture x RBO).", handle, rbo_slot);
            }

            snprintf(buf, sizeof(buf), "fbo%d_rbo_slot%d", handle, rbo_slot);
            rbo = Renderbuffer(width, height, rboAttachment.params.internalFormat, std::string(buf));
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, rbo_slot, GL_RENDERBUFFER, rbo.Handle());
        }


        if (!IsComplete()) {
            throw std::exception();
        }

        UpdateDrawBuffers();

        ENG_LOG_TRACE("[C] Framebuffer {} ({} texture attachment{})", handle, (int)attachments.size(), ((attachments.size() > 1) ? 's' : ' '));
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    Framebuffer::~Framebuffer() {
        Release();
    }

    Framebuffer::Framebuffer(Framebuffer&& fbo) noexcept {
        Move(std::move(fbo));
    }

    Framebuffer& Framebuffer::operator=(Framebuffer&& fbo) noexcept {
        Release();
        Move(std::move(fbo));
        return *this;
    }

    void Framebuffer::Bind() const {
        ASSERT_MSG(handle != 0, "Attempting to use uninitialized framebuffer.");
        glBindFramebuffer(GL_FRAMEBUFFER, handle);
    }

    void Framebuffer::Unbind() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void Framebuffer::ReadOnlyBind(GLenum slot) const {
        ASSERT_MSG(handle != 0, "Attempting to use uninitialized framebuffer.");
        ASSERT_MSG(attachments.count(slot) != 0, "Framebuffer::ReadOnlyBind - FBO doesn't have attachment {}", slot);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, handle);
        glReadBuffer(slot);
    }

    bool Framebuffer::IsComplete() const {
        Bind();

        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            const char* statusText = "OTHER ERROR";
            switch (status) {
                case GL_FRAMEBUFFER_UNDEFINED:						statusText = "GL_FRAMEBUFFER_UNDEFINED "; break;
                case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:			statusText = "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT "; break;
                case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:	statusText = "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT "; break;
                case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:			statusText = "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER "; break;
                case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:			statusText = "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER "; break;
                case GL_FRAMEBUFFER_UNSUPPORTED:					statusText = "GL_FRAMEBUFFER_UNSUPPORTED"; break;
                case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:			statusText = "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE "; break;
                case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:		statusText = "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS "; break;
            }

            ENG_LOG_ERROR("Framebuffer - completeness check failed (status = '{}').", statusText);
            return false;
        }

        return true;
    }

    void Framebuffer::UpdateDrawBuffers() {
        std::vector<GLenum> slots;

        for (auto& [key, val] : attachments) {
            //only search for color attachments, no depth/stencil stuff
            if (val != nullptr && key >= GL_COLOR_ATTACHMENT0 && key <= GL_COLOR_ATTACHMENT31) {
                slots.push_back(key);
            }
        }
        //add RBO too, if it exists & is a color attachment
        if (rbo_slot >= GL_COLOR_ATTACHMENT0 && rbo_slot <= GL_COLOR_ATTACHMENT31) {
            slots.push_back(rbo_slot);
        }

        UpdateDrawBuffers(slots);
    }

    void Framebuffer::UpdateDrawBuffers(const std::vector<GLenum>& slots) {
        Bind();
        glDrawBuffers(slots.size(), slots.data());
    }

    TextureRef Framebuffer::Attachment(GLenum i) const {
        return attachments.at(i);
    }

    TextureRef Framebuffer::ColorAttachment(int i) const {
        return attachments.at(GL_COLOR_ATTACHMENT0 + i);
    }

    void Framebuffer::Release() noexcept {
        if (handle != 0) {
            ENG_LOG_TRACE("[D] Framebuffer {} ({} texture attachment{})", handle, (int)attachments.size(), ((attachments.size() > 1) ? 's' : ' '));
            attachments.clear();
            glDeleteFramebuffers(1, &handle);
            handle = 0;
        }
    }

    void Framebuffer::Move(Framebuffer&& fbo) noexcept {
        handle = fbo.handle;
        size = fbo.size;
        attachments = fbo.attachments;
        rbo = std::move(fbo.rbo);
        rbo_slot = fbo.rbo_slot;

        fbo.handle = 0;
    }

    //===== ClickSelectionHandler =====


    ClickSelectionHandler::ClickSelectionHandler(const FramebufferRef& fbo_) : fbo(fbo_), data(glm::uvec4(0)) {
        glGenBuffers(1, &pbo);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
        glBufferData(GL_PIXEL_PACK_BUFFER, sizeof(float) * 4, 0, GL_STREAM_READ);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }

    ClickSelectionHandler::~ClickSelectionHandler() {
        if (pbo != 0) {
            ENG_LOG_TRACE("[D] ClickSelectionHandler {}", pbo);
            glDeleteBuffers(1, &pbo);
            pbo = 0;
            fbo = nullptr;
        }
    }

    ClickSelectionHandler::ClickSelectionHandler(ClickSelectionHandler&& c) noexcept {
        fbo = c.fbo;
        pbo = c.pbo;
        data = c.data;

        c.pbo = 0;
        c.fbo = nullptr;
    }

    ClickSelectionHandler& ClickSelectionHandler::operator=(ClickSelectionHandler&& c) noexcept {
        fbo = c.fbo;
        pbo = c.pbo;
        data = c.data;

        c.pbo = 0;
        c.fbo = nullptr;

        return *this;
    }

    void ClickSelectionHandler::FetchPixel(int x, int y) {
        ASSERT_MSG(pbo != 0 && fbo != nullptr, "Attempting to use uninitialized ClickSelectionHandler");
        fbo->ReadOnlyBind(GL_COLOR_ATTACHMENT1);

        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
        glReadPixels(x, y, 1, 1, GL_RGBA_INTEGER, GL_UNSIGNED_INT, 0);

        fbo->UpdateDrawBuffers();
    }

    glm::uvec4 ClickSelectionHandler::Retrieve() {
        ASSERT_MSG(pbo != 0 && fbo != nullptr, "Attempting to use uninitialized ClickSelectionHandler");
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbo);
        float* f = (float*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
        if (f) {
            for (int i = 0; i < 4; i++) data[i] = (unsigned int)f[i];
            glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        }
        else {
            ENG_LOG_WARN("ClickSelectionHandler::Retrieve - map buffer failed.");
            data = glm::uvec4(0);
        }

        return data;
    }

    //===== ClickSelectionController =====

    ClickSelectionController::ClickSelectionController(const FramebufferRef& fbo_) : handler(ClickSelectionHandler(fbo_)), clickPos(glm::ivec2(0)), transfer(false) {}

    ClickSelectionController::~ClickSelectionController() {}

    ClickSelectionController::ClickSelectionController(ClickSelectionController&& c) noexcept {
        handler = std::move(c.handler);
        clickPos = c.clickPos;
        transfer = c.transfer;
    }

    ClickSelectionController& ClickSelectionController::operator=(ClickSelectionController&& c) noexcept {
        handler = std::move(c.handler);
        clickPos = c.clickPos;
        transfer = c.transfer;

        return *this;
    }

    void ClickSelectionController::Register(bool transfer_) {
        transfer = transfer_;
        if(transfer) {
            clickPos = Window::Get().CursorPos_VFlip();
        }
    }

    void ClickSelectionController::Update() {
        if(transfer) {
            handler.FetchPixel(clickPos.x, clickPos.y);
        }
    }

    void ClickSelectionController::Finalize() {
        if(transfer) {
            handler.Retrieve();
        }
    }

}//namespace eng
