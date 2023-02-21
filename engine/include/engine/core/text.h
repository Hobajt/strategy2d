#pragma once

#include "engine/core/texture.h"
#include "engine/utils/mathdefs.h"

#include <memory>

namespace eng {

    struct CharInfo {
        glm::ivec2 size;
        glm::ivec2 bearing;		//aka. offsets
        glm::ivec2 advance;

        //character's x-offset within the atlas texture
        float textureOffset;
    };

    class Font {
    public:
        Font(const std::string& filepath, int fontHeight = 48);

        Font() = default;
        ~Font();

        //copy disabled
        Font(const Font&) = delete;
        Font& operator=(const Font&) = delete;

        //move enabled
        Font(Font&&) noexcept;
        Font& operator=(Font&&) noexcept;

        CharInfo& operator[](char c) { return chars[int(c)]; }
        const CharInfo& operator[](char c) const { return chars[int(c)]; }

        CharInfo& GetChar(char c) { return operator[](c); }

        glm::vec2 AtlasSize_Inv() const { return atlasSize_inv; }

        void RenderText(const char* text, const glm::vec2 topLeft, float scale, const glm::vec4& color = glm::vec4(1.f), float zIndex = -0.9f, const glm::uvec4& info = glm::uvec4(0));
        void RenderTextCentered(const char* text, const glm::vec2 center, float scale, const glm::vec4& color = glm::vec4(1.f), float zIndex = -0.9f, const glm::uvec4& info = glm::uvec4(0));
    private:
        void Load(const std::string& filepath);
        
        void Release() noexcept;
        void Move(Font&&) noexcept;
    private:
        std::string name;

        int fontHeight;
        CharInfo chars[128];
        TextureRef texture = nullptr;

        glm::vec2 atlasSize_inv;
    };
    using FontRef = std::shared_ptr<Font>;

}//namespace eng
