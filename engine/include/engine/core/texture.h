#pragma once

#include <memory>
#include <string>

#include "engine/utils/mathdefs.h"
#include "engine/utils/setup.h"

#include <glad/glad.h>

namespace eng {

    class Texture;
    using TextureRef = std::shared_ptr<Texture>;

    //Texture construction options.
    namespace TextureFlags {
        enum {
            LOAD_SRGB		= BIT(0),
            VERTICAL_FLIP	= BIT(1),
        };
    }//namespace TextureFlags

    //======== TextureParams ========

    //Container for all the texture parameters.
    struct TextureParams {
        GLenum internalFormat = GL_RGB;
        GLenum format = GL_RGB;
        GLenum dtype = GL_UNSIGNED_BYTE;
        
        GLenum filtering = GL_NEAREST;
        GLenum wrapping = GL_CLAMP_TO_EDGE;
        glm::vec4 borderColor = glm::vec4(0.f);

        int width = 0;
        int height = 0;
    public:
        TextureParams() = default;

        //Constructor for textures loaded from files.
        TextureParams(GLenum filtering, GLenum wrapping);

        TextureParams(int width, int height, GLenum internalFormat, GLenum format, GLenum dtype, GLenum filtering = GL_LINEAR, GLenum wrapping = GL_CLAMP_TO_EDGE);

        static TextureParams EmptyTexture(int width, int height, GLenum internalFormat, GLenum filtering = GL_LINEAR, GLenum wrapping = GL_CLAMP_TO_EDGE);
        static TextureParams EmptyTexture(const TextureParams& params);
        static TextureParams CustomData(int width, int height, GLenum internalFormat, GLenum format, GLenum dtype, GLenum filtering = GL_LINEAR, GLenum wrapping = GL_CLAMP_TO_EDGE);

        TextureParams(const TextureParams& params, bool dummy);
    };

    //======= TexCoords =======

    //Wrapper for texture coordinates, to properly initialize quads.
    struct TexCoords {
        glm::vec2 texCoords[4];
    public:
        TexCoords() = default;
        TexCoords(const glm::vec2& tc1, const glm::vec2& tc2, const glm::vec2& tc3, const glm::vec2& tc4);

        //Generates coords that cover the entire texture.
        static TexCoords& Default();

        glm::vec2& operator[](int i) { return texCoords[i]; }
        const glm::vec2& operator[](int i) const { return texCoords[i]; }

        TexCoords operator +(const glm::vec2& offset) const;
        TexCoords& operator +=(const glm::vec2& offset);
    };

    //======== Texture ========

    //Wrapper for the underlaying texture resource on the GPU.
    class Texture {
    private:
        //Texture handle wrapper, has ownership of the handle.
        //Used internally by Texture to share ownership when working with merged textures.
        struct TextureHandle {
            GLuint handle = 0;

            static int counter;
        public:
            TextureHandle() = default;
            TextureHandle(GLuint handle_);
            ~TextureHandle();
        };
        using TextureHandleRef = std::shared_ptr<TextureHandle>;
    public:
        // static uint64_t DefaultTextureID();

        //Contructors for textures loaded from files. In TextureParams, only 'filtering' & 'wrapping' fields are used.
        Texture(const std::string& filepath, int flags = 0, const TextureParams& params = {});
        Texture(const std::string& filepath, const TextureParams& params, int flags = 0);

        //Constructor for empty textures (8-bit color channels). TextureParams fields 'dtype' & 'format' are ignored.
        Texture(const TextureParams& params, const std::string& name = "custom");

        //Constructor for textures with custom data type and specific data (data can be nullptr). All TextureParams fields matter.
    	Texture(const TextureParams& params, void* data, const std::string& name = "custom");

        Texture() = default;
        ~Texture();

        //copy disabled
        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;

        //move enabled
        Texture(Texture&&) noexcept;
        Texture& operator=(Texture&&) noexcept;

        //comparison operator (compares based on handle)
        bool operator==(const Texture& rhs) const;
        bool operator!=(const Texture& rhs) const;

        void Bind(int slot) const;
        static void Bind(int slot, GLuint handle);
	    static void Unbind(int slot);

        void UpdateData(void* data);

        int Width() const { return params.width; }
        int Height() const { return params.height; }
        glm::ivec2 Size() const { return glm::ivec2(params.width, params.height); }

        std::string Name() const { return name; }

        GLuint Handle() const { return handle; }

        //Returns TexCoords for given rectangle in the texture. Takes into account texture merging.
        TexCoords GetTexCoords() const;
        TexCoords GetTexCoords(const glm::ivec2& offset, const glm::ivec2& size) const;
        TexCoords GetTexCoords(const glm::ivec2& offset, const glm::ivec2& size, bool flip) const;

        static void MergeTextures(std::vector<TextureRef>& texturesToMerge, GLenum filteringMode, bool rgba = true);
        static int HandleCount() { return TextureHandle::counter; }

        void DBG_GUI() const;
    private:
        void Release() noexcept;
        void Move(Texture&&) noexcept;

        //Loads texture from provided file.
        bool LoadFromFile(const std::string& filepath, int flags);

        //To update handle and offset into the texture after texture merging.
        void Merge_UpdateData(TextureHandleRef handle, const glm::ivec2& offset, const glm::vec2& size);

        //Copy this texture's data into another texture at given pixel offset.
        void Merge_CopyTo(const TextureRef& other, const glm::ivec2& offset);
    private:
        std::string name;
        TextureParams params;

        TextureHandleRef handle_data = nullptr;
        GLuint handle = 0;     //keeping a copy, to avoid the pointer dereference
        glm::ivec2 merge_offset;
        glm::vec2 merge_size;
    };

    //===== Image =====

    //Image buffer (not a texture, not uploaded to the GPU).
    class Image {
    public:
        Image(const std::string& filepath, int flags = 0);
        Image(int width, int height, int channels, const void* data);

        Image() = default;
        ~Image();

        //copy disabled
        Image(const Image&) = delete;
        Image& operator=(const Image&) = delete;

        //move enabled
        Image(Image&&) noexcept;
        Image& operator=(Image&&) noexcept;

        Image operator()(int y, int x, int h, int w);

        uint8_t* ptr() { return data; }

        bool Write(const std::string& filepath);
    private:
        Image(const Image& img, int y, int x, int h, int w);

        void Release() noexcept;
        void Move(Image&&) noexcept;

        bool LoadFromFile(const std::string& filepath, int flags);
        bool WriteToFile(const std::string& filepath);
    private:
        uint8_t* data;
        int channels;
        int height;
        int width;

        std::string name;
    };

}//namespace eng
