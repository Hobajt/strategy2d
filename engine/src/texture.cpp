#include "engine/core/texture.h"

#include "engine/utils/utils.h"
#include "engine/utils/gui.h"

namespace eng {

    //======== TextureParams ========

    TextureParams::TextureParams(GLenum filtering_, GLenum wrapping_)
        : filtering(filtering_), wrapping(wrapping_) {}

    TextureParams::TextureParams(int width_, int height_, GLenum internalFormat_, GLenum format_, GLenum dtype_, GLenum filtering_, GLenum wrapping_)
        : width(width_), height(height_), internalFormat(internalFormat_), format(format_), dtype(dtype_), filtering(filtering_), wrapping(wrapping_) {}

    TextureParams TextureParams::EmptyTexture(int width, int height, GLenum internalFormat, GLenum filtering, GLenum wrapping) {
        return TextureParams(width, height, internalFormat, GL_RGB, GL_UNSIGNED_BYTE, filtering, wrapping);
    }

    TextureParams TextureParams::EmptyTexture(const TextureParams& p) {
        return TextureParams(p.width, p.height, p.internalFormat, GL_RGB, GL_UNSIGNED_BYTE, p.filtering, p.wrapping);
    }

    TextureParams TextureParams::CustomData(int width, int height, GLenum internalFormat, GLenum format, GLenum dtype, GLenum filtering, GLenum wrapping) {
        return TextureParams(width, height, internalFormat, format, dtype, filtering, wrapping);
    }

    //======== Texture ========

    Texture::Texture(const std::string& filepath, int flags, const TextureParams& params_) : Texture(filepath, params_, flags) {}

    Texture::Texture(const std::string& filepath, const TextureParams& params_, int flags) : name(GetFilename(filepath)), params(params_) {
        if(!LoadFromFile(filepath, flags)) {
            ENG_LOG_DEBUG("Texture - Failed to load from '{}'.", filepath.c_str());
            throw std::exception();
        }
        ENG_LOG_TRACE("[C] Texture '{}' ({})", name.c_str(), handle);
    }

    Texture::Texture(const TextureParams& params, const std::string& name) : Texture(TextureParams::EmptyTexture(params), nullptr, name) {}

    Texture::Texture(const TextureParams& params_, void* data, const std::string& name_) : params(params_) {
        name = name_;

        glActiveTexture(GL_TEXTURE0);
        glGenTextures(1, &handle);
        glBindTexture(GL_TEXTURE_2D, handle);

        if (params.filtering != GL_NONE) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, params.filtering);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, params.filtering);
        }
        if (params.wrapping != GL_NONE) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, params.wrapping);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, params.wrapping);
            if(params.wrapping == GL_CLAMP_TO_BORDER) {
                glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, (float*)&params.borderColor);
            }
        }
        glTexImage2D(GL_TEXTURE_2D, 0, params.internalFormat, params.width, params.height, 0, params.format, params.dtype, data);

        glBindTexture(GL_TEXTURE_2D, 0);

        ENG_LOG_TRACE("[R] Created custom texture '{}' ({}x{}).", name.c_str(), params.width, params.height);
        ENG_LOG_TRACE("[C] Texture '{}' ({})", name.c_str(), handle);
    }

    Texture::~Texture() {
        Release();
    }

    Texture::Texture(Texture&& t) noexcept {
        Move(std::move(t));
    }

    Texture& Texture::operator=(Texture&& t) noexcept {
        Release();
        Move(std::move(t));
        return *this;
    }

    bool Texture::operator==(const Texture& rhs) const {
        return handle == rhs.handle;
    }

    bool Texture::operator!=(const Texture& rhs) const {
        return handle != rhs.handle;
    }

    void Texture::Bind(int slot) const {
        ASSERT_MSG(handle != 0, "Attempting to use uninitialized texture.");
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, handle);
    }

    void Texture::Unbind(int slot) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture::DBG_GUI() const {
#ifdef ENGINE_ENABLE_GUI
        ImVec2 wsize = ImGui::GetWindowSize();
        ImGui::Image((ImTextureID)handle, wsize, ImVec2(0, 1), ImVec2(1, 0));
#endif
    }

    void Texture::Release() noexcept {
        if(handle != 0) {
            ENG_LOG_TRACE("[D] Texture '{}' ({})", name.c_str(), handle);
            glDeleteTextures(1, &handle);
            handle = 0;
        }
    }

    void Texture::Move(Texture&& t) noexcept {
        handle = t.handle;
        params = t.params;
        name = std::move(t.name);
        t.handle = 0;
    }

    //======= TexCoords =======

    TexCoords& TexCoords::Default() {
        static TexCoords instance = TexCoords(
            glm::vec2(0.f, 1.f),
            glm::vec2(0.f, 0.f),
            glm::vec2(1.f, 1.f),
            glm::vec2(1.f, 0.f)
        );
        return instance;
    }

    TexCoords::TexCoords(const glm::vec2& tc1, const glm::vec2& tc2, const glm::vec2& tc3, const glm::vec2& tc4) : texCoords{tc1, tc2, tc3, tc4} {}

    TexCoords TexCoords::operator +(const glm::vec2& offset) const {
        return TexCoords(texCoords[0] + offset, texCoords[1] + offset, texCoords[2] + offset, texCoords[3] + offset);
    }

    TexCoords& TexCoords::operator +=(const glm::vec2& offset) {
        texCoords[0] += offset;
        texCoords[1] += offset;
        texCoords[2] += offset;
        texCoords[3] += offset;
        return *this;
    }


}//namespace eng
