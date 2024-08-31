#include "engine/core/palette.h"

#include "engine/core/renderer.h"

using rgba = glm::u8vec4;

#define NUM_FACTION_COLORS 8

static constexpr int palette_upscale = 16;

namespace eng {

    ColorPalette::ColorPalette(const glm::ivec2& size_, const std::string& name_) : size(size_), name(name_) {
        int area = size.x * size.y;

        data = new rgba[area];
        for(int i = 0; i < area; i++)
            data[i] = rgba(0,0,0,255);

        UpdateTexture();
        ENG_LOG_TRACE("[C] ColorPalette '{}' ({}x{})", name, size.x, size.y);
    }

    ColorPalette::ColorPalette(bool dummy) : name("colorPalette_d"), size(glm::ivec2(4, NUM_FACTION_COLORS)) {
        data = new rgba[size.x * size.y] {
            rgba{68, 4, 0, 255},    rgba{92, 4, 0, 255},     rgba{124, 0, 0, 255},      rgba{164, 0, 0, 255},
            rgba{0, 4, 76, 255},    rgba{0, 20, 108, 255},   rgba{0, 36, 148, 255},     rgba{0, 60, 192, 255},
            rgba{0, 40, 12, 255},   rgba{4, 84, 44, 255},    rgba{20, 132, 92, 255},    rgba{44, 180, 148, 255},
            rgba{44, 8, 44, 255},   rgba{80, 16, 76, 255},   rgba{116, 48, 132, 255},   rgba{152, 72, 176, 255},
            rgba{108, 32, 12, 255}, rgba{152, 56, 16, 255},  rgba{196, 88, 16, 255},    rgba{240, 132, 20, 255},
            rgba{12, 12, 20, 255},  rgba{20, 20, 32, 255},   rgba{28, 28, 44, 255},     rgba{40, 40, 60, 255},
            rgba{36, 40, 76, 255},  rgba{84, 84, 128, 255},  rgba{152, 152, 180, 255},  rgba{224, 224, 224, 255},
            rgba{180, 116, 0, 255}, rgba{204, 160, 16, 255}, rgba{228, 204, 40, 255},   rgba{252, 252, 72, 255},
        };

        UpdateTexture();
        ENG_LOG_TRACE("[C] Default ColorPalette '{}' ({}x{})", name, size.x, size.y);
    }

    ColorPalette::~ColorPalette() {
        Release();
    }

    ColorPalette::ColorPalette(ColorPalette&& p) noexcept {
        Move(std::move(p));
    }

    ColorPalette& ColorPalette::operator=(ColorPalette&& p) noexcept {
        Release();
        Move(std::move(p));
        return *this;
    }

    glm::u8vec4& ColorPalette::operator()(int y, int x) {
        ASSERT_MSG((texture != nullptr) && (data != nullptr), "ColorPalette isn't properly initialized!");
        ASSERT_MSG((unsigned(y) < unsigned(size.y)) && (unsigned(x) < unsigned(size.x)), "Array index ({}, {}) is out of bounds.", y, x);
        return data[y*size.x + x];
    }

    const glm::u8vec4& ColorPalette::operator()(int y, int x) const {
        ASSERT_MSG((texture != nullptr) && (data != nullptr), "ColorPalette isn't properly initialized!");
        ASSERT_MSG((unsigned(y) < unsigned(size.y)) && (unsigned(x) < unsigned(size.x)), "Array index ({}, {}) is out of bounds.", y, x);
        return data[y*size.x + x];
    }

    void ColorPalette::Bind(const ShaderRef& shader) {
        ASSERT_MSG(texture != nullptr, "Attempting to use uninitialized color palette.");

        int slot = Renderer::ForceBindTexture(texture);
        ASSERT_MSG(slot >= 0, "Failed to bind color palette texture.");

        shader->Bind();
        shader->SetInt("colorPalette", slot);
    }

    void ColorPalette::UpdateTexture() {
        int s = palette_upscale;

        //tmp pixel array setup
        rgba* tex_data = new rgba[size.x*s * size.y*s];
        for(int y = 0; y < size.y; y++) {
            for(int x = 0; x < size.x; x++) {
                rgba clr = data[y*size.x + x];

                for(int i = 0; i < s; i++) {
                    for(int j = 0; j < s; j++) {
                        tex_data[(y*s+i)*(size.x*s) + (x*s+j)] = clr;
                    }
                }
            }
        }

        //texture create & upload
        if(texture == nullptr) {
            texture = std::make_shared<Texture>();
        }

        Texture& t = *texture.get();
        t = Texture(
            TextureParams::CustomData(size.x * s, size.y * s, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, GL_NEAREST),
            tex_data,
            name
        );

        Quad::paletteSize = size.y;

        delete[] tex_data;
    }

    void ColorPalette::UpdateShaderValues(const ShaderRef& shader) {
        shader->Bind();
        shader->SetVec2("centeringOffset", GetCenterOffset());
    }

    glm::vec2 ColorPalette::GetCenterOffset() const {
        return 1.f / (2.f * glm::vec2(size));
    }

    int ColorPalette::FactionColorCount() {
        return NUM_FACTION_COLORS;
    }

    std::array<glm::vec4, ColorPalette::COLOR_COUNT> ColorPalette::Colors() {
        return std::array<glm::vec4, ColorPalette::COLOR_COUNT>{
            rgba{164, 0, 0, 255},
            rgba{0, 60, 192, 255},
            rgba{44, 180, 148, 255},
            rgba{152, 72, 176, 255},
            rgba{240, 132, 20, 255},
            rgba{40, 40, 60, 255},
            rgba{224, 224, 224, 255},
            rgba{252, 252, 72, 255},
        };
    }

    void ColorPalette::Move(ColorPalette&& p) noexcept {
        texture = p.texture;
        data = p.data;
        size = p.size;
        
        p.texture = nullptr;
        p.data = nullptr;
        p.size = glm::ivec2(0);
    }

    void ColorPalette::Release() noexcept {
        if(data != nullptr) {
            ENG_LOG_TRACE("[D] ColorPalette ({}x{})", size.x, size.y);
            delete[] data;
        }
        texture = nullptr;
    }

}//namespace eng
