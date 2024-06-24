#include "engine/core/texture.h"

#include "engine/utils/utils.h"
#include "engine/utils/dbg_gui.h"

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

    int Texture::TextureHandle::counter = 0;

    Texture::TextureHandle::TextureHandle(GLuint handle_) : handle(handle_) {
        ENG_LOG_TRACE("[C] TextureHandle ({})", handle);
        counter++;
    }

    Texture::TextureHandle::~TextureHandle() {
        if(handle != 0) {
            ENG_LOG_TRACE("[D] TextureHandle ({})", handle);
            glDeleteTextures(1, &handle);
            handle = 0;
            counter--;
        }
    }

    Texture::Texture(const std::string& filepath, int flags, const TextureParams& params_) : Texture(filepath, params_, flags) {}

    Texture::Texture(const std::string& filepath, const TextureParams& params_, int flags) : name(GetFilename(filepath)), params(params_) {
        if(!LoadFromFile(filepath, flags)) {
            ENG_LOG_DEBUG("Texture - Failed to load from '{}'.", filepath.c_str());
            throw std::exception();
        }
        Merge_UpdateData(std::make_shared<TextureHandle>(handle), glm::ivec2(0), glm::vec2(params.width, params.height));
        ENG_LOG_TRACE("[C] Texture '{}' ({})", name.c_str(), handle);
    }

    Texture::Texture(const TextureParams& params, const std::string& name) : Texture(TextureParams::EmptyTexture(params), nullptr, name) {}

    Texture::Texture(const TextureParams& params_, void* data, const std::string& name_)
        : params(params_) {
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

        Merge_UpdateData(std::make_shared<TextureHandle>(handle), glm::ivec2(0), glm::vec2(params.width, params.height));

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
        Texture::Bind(slot, handle);
    }

    void Texture::Bind(int slot, GLuint handle) {
        ASSERT_MSG(handle != 0, "Attempting to use uninitialized texture.");
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, handle);
    }

    void Texture::Unbind(int slot) {
        glActiveTexture(GL_TEXTURE0 + slot);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void Texture::UpdateData(void* data) {
        Bind(0);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, params.width, params.height, params.format, params.dtype, data);
        // ENG_LOG_TRACE("[R] Texture data update '{}' ({}x{}).", name.c_str(), params.width, params.height);
        Unbind(0);
    }

    void Texture::Merge_UpdateData(TextureHandleRef new_handle, const glm::ivec2& offset, const glm::vec2& size) {
        handle_data = new_handle;
        handle = new_handle->handle;
        merge_offset = offset;
        merge_size = size;
    }

    void Texture::Merge_CopyTo(const TextureRef& other, const glm::ivec2& offset) {
        glCopyImageSubData(handle, GL_TEXTURE_2D, 0, merge_offset.x, merge_offset.y, 0, other->handle, GL_TEXTURE_2D, 0, offset.x, offset.y, 0, params.width, params.height, 1);
    }

    TexCoords Texture::GetTexCoords() const {
        glm::vec2 m = glm::vec2(merge_offset         ) / merge_size;
        glm::vec2 M = glm::vec2(merge_offset + Size()) / merge_size;

        return TexCoords(
            glm::vec2(m.x, M.y),
            glm::vec2(m.x, m.y),
            glm::vec2(M.x, M.y),
            glm::vec2(M.x, m.y)
        );
    }

    TexCoords Texture::GetTexCoords(const glm::ivec2& rect_offset, const glm::ivec2& rect_size) const {
        glm::vec2 m = glm::vec2(merge_offset + rect_offset            ) / merge_size;
        glm::vec2 M = glm::vec2(merge_offset + rect_offset + rect_size) / merge_size;

        return TexCoords(
            glm::vec2(m.x, M.y),
            glm::vec2(m.x, m.y),
            glm::vec2(M.x, M.y),
            glm::vec2(M.x, m.y)
        );
    }

    TexCoords Texture::GetTexCoords(const glm::ivec2& rect_offset, const glm::ivec2& rect_size, bool flip) const {
        glm::ivec2 of = merge_offset + rect_offset;
        glm::ivec2 sz = rect_size;
        glm::vec2 m = glm::vec2(merge_offset + rect_offset            ) / merge_size;
        glm::vec2 M = glm::vec2(merge_offset + rect_offset + rect_size) / merge_size;

        if(!flip) {
            return TexCoords(
                glm::vec2(m.x, M.y),
                glm::vec2(m.x, m.y),
                glm::vec2(M.x, M.y),
                glm::vec2(M.x, m.y)
            );
        }
        else {
            return TexCoords(
                glm::vec2(M.x, M.y),
                glm::vec2(M.x, m.y),
                glm::vec2(m.x, M.y),
                glm::vec2(m.x, m.y)
            );
        }
    }

    void Texture::DBG_GUI() const {
#ifdef ENGINE_ENABLE_GUI
        // ImVec2 wsize = ImGui::GetWindowSize();
        ImVec2 wsize = ImGui::GetContentRegionAvail();
        
        //to preserve texture sides ratio while allowing scaling
        ImVec2 r = ImVec2(wsize.x / merge_size.x, wsize.y / merge_size.y);
        float ratio = float(merge_size.x) / merge_size.y;
        wsize = (r.x > r.y) ? ImVec2(wsize.y * ratio, wsize.y) : ImVec2(wsize.x, wsize.x / ratio);

        ImGui::Image((ImTextureID)handle, wsize, ImVec2(0, 1), ImVec2(1, 0));
#endif
    }

    void Texture::Release() noexcept { 
        if(handle != 0) {
            ENG_LOG_TRACE("[D] Texture '{}' ({})", name.c_str(), handle);
            handle_data = nullptr;
            handle = 0;
        }
    }

    void Texture::Move(Texture&& t) noexcept {
        handle = t.handle;
        handle_data = t.handle_data;
        params = t.params;
        name = std::move(t.name);
        merge_offset = t.merge_offset;
        merge_size = t.merge_size;
        t.handle = 0;
        t.handle_data = nullptr;
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

    //===== Image =====

    Image::Image(const std::string& filepath, int flags) : name(GetFilename(filepath)) {
        if(!LoadFromFile(filepath, flags)) {
            ENG_LOG_DEBUG("Image - Failed to load from '{}'.", filepath.c_str());
            throw std::exception();
        }
        ENG_LOG_TRACE("[C] Image '{}'", name.c_str());
    }

    Image::~Image() {
        Release();
    }

    Image::Image(Image&& i) noexcept {
        Move(std::move(i));
    }

    Image& Image::operator=(Image&& i) noexcept {
        Release();
        Move(std::move(i));
        return *this;
    }

    Image Image::operator()(int y, int x, int h, int w) {
        return Image(*this, y, x, h, w);
    }

    Image::Image(const Image& img, int iy, int ix, int h, int w) : height(h), width(w), channels(img.channels), name(img.name) {
        data = new uint8_t[w * h * channels];
        int c = channels;

        if(((unsigned int)iy) >= img.height || ((unsigned int)ix) >= img.width || ((unsigned int)iy+h) > img.height || ((unsigned int)ix+w) > img.width) {
            ENG_LOG_ERROR("Image - invalid ROI coordinates (({}, {}, {}, {}) when original has size ({}, {}))", iy, ix, h, w, img.height, img.width);
            throw std::runtime_error("");
        }

        for(int y = 0; y < h; y++) {
            memcpy(&data[y*w*c], &img.data[(iy+y)*img.width*c + ix*c], sizeof(uint8_t) * w * c);
        }
    }

    void Image::Release() noexcept {
        if(data != nullptr) {
            ENG_LOG_TRACE("[D] Image '{}'", name.c_str());
            delete[] data;
            width = height = channels = 0;
        }
    }

    void Image::Move(Image&& i) noexcept {
        width = i.width;
        height = i.height;
        channels = i.channels;
        data = i.data;
        i.data = nullptr;
        i.width = i.height = i.channels = 0;
    }

}//namespace eng
