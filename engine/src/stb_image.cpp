#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "engine/core/texture.h"

bool eng::Texture::LoadFromFile(const std::string& filepath, int flags) {
    //use texture unit 0 for manipulation
    glActiveTexture(GL_TEXTURE0);

    bool srgb = ((flags & TextureFlags::LOAD_SRGB) != 0);
    bool flip = ((flags & TextureFlags::VERTICAL_FLIP) != 0);
    stbi_set_flip_vertically_on_load(flip);

    //load image data
    int channels;
    uint8_t* buf = stbi_load(filepath.c_str(), &params.width, &params.height, &channels, 0);
    if (buf == nullptr) {
        ENG_LOG_WARN("Failed to load texture from '{}'.", filepath.c_str());
        return false;
    }

    //generate the texture
    glGenTextures(1, &handle);
    glBindTexture(GL_TEXTURE_2D, handle);

    //setup filtering & wrapping (based on options)
    if (params.filtering != GL_NONE) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, params.filtering);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, params.filtering);
    }
    if (params.wrapping != GL_NONE) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, params.wrapping);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, params.wrapping);
    }

    //resolve texture's data format
    switch (channels) {
        case 1:
            params.internalFormat = params.format = GL_RED;
            if (srgb) params.internalFormat = GL_SRGB8;
            break;
        default:
        case 3:
            params.internalFormat = params.format = GL_RGB;
            if (srgb) params.internalFormat = GL_SRGB;
            break;
        case 4:
            params.internalFormat = params.format = GL_RGBA;
            if (srgb) params.internalFormat = GL_SRGB_ALPHA;
            break;
    }
    params.dtype = GL_UNSIGNED_BYTE;

    //initialize the texture
    glTexImage2D(GL_TEXTURE_2D, 0, params.internalFormat, params.width, params.height, 0, params.format, params.dtype, buf);
    stbi_image_free(buf);

    //unbind
    glBindTexture(GL_TEXTURE_2D, 0);

    ENG_LOG_TRACE("[R] Loaded texture from '{}' ({}x{}).", name.c_str(), params.width, params.height);
    return true;
}

bool eng::Image::LoadFromFile(const std::string& filepath, int flags) {
    bool srgb = ((flags & TextureFlags::LOAD_SRGB) != 0);
    bool flip = ((flags & TextureFlags::VERTICAL_FLIP) != 0);
    stbi_set_flip_vertically_on_load(flip);

    //load image data
    uint8_t* buf = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
    if (buf == nullptr) {
        ENG_LOG_WARN("Failed to load image from '{}'.", filepath.c_str());
        return false;
    }

    data = new uint8_t[width * height * channels];
    memcpy(data, buf, sizeof(uint8_t) * width * height * channels);

    stbi_image_free(buf);

    ENG_LOG_TRACE("[R] Loaded image from '{}' ({}x{}).", name.c_str(), width, height);
    return true;
}
