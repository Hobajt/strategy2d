#pragma once

#include "engine/core/texture.h"
#include "engine/core/shader.h"
#include "engine/utils/mathdefs.h"

#include <string>
#include <memory>

namespace eng {

    //Wrapper for a color palette texture.
    class ColorPalette {
    public:
        ColorPalette(const glm::ivec2& size, const std::string& name = "colorPalette");
        ColorPalette(bool dummy);

        ColorPalette() = default;
        ~ColorPalette();

        //copy disabled
        ColorPalette(const ColorPalette&) = delete;
        ColorPalette& operator=(const ColorPalette&) = delete;

        //move enabled
        ColorPalette(ColorPalette&&) noexcept;
        ColorPalette& operator=(ColorPalette&&) noexcept;

        //Any color changes don't update the texture, call UpdateTexture() manually afterwards.
        glm::u8vec4& operator()(int y, int x);
        const glm::u8vec4& operator()(int y, int x) const;

        void Bind(const ShaderRef& shader);

        void UpdateTexture();
        void UpdateShaderValues(const ShaderRef& shader);

        glm::vec2 GetCenterOffset() const;

        const TextureRef& GetTexture() const { return texture; }
        glm::ivec2 Size() const { return size; }
    private:
        void Move(ColorPalette&&) noexcept;
        void Release() noexcept;
    private:
        TextureRef texture = nullptr;

        glm::u8vec4* data = nullptr;
        glm::ivec2 size = glm::ivec2(0);

        std::string name = "unknownPalette";
    };

}//namespace eng
