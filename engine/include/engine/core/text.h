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

    class Font;
    using FontRef = std::shared_ptr<Font>;

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

        int GetRowHeight() { return rowHeight; }

        glm::vec2 AtlasSize_Inv() const { return atlasSize_inv; }

        void RenderText(const char* text, const glm::vec2 topLeft, float scale, const glm::vec4& color = glm::vec4(1.f), float zIndex = -0.9f, const glm::uvec4& info = glm::uvec4(0));
        void RenderTextCentered(const char* text, const glm::vec2 center, float scale, const glm::vec4& color = glm::vec4(1.f), float zIndex = -0.9f, const glm::uvec4& info = glm::uvec4(0));
        void RenderTextAlignLeft(const char* text, const glm::vec2 topLeft, float scale, const glm::vec4& color = glm::vec4(1.f), float zIndex = -0.9f, const glm::uvec4& info = glm::uvec4(0));

        //clipped = top/bottom parts of the letters are cut off; cutPos.x = fromTop, .y = fromBottom
        void RenderTextClippedTop(const char* text, const glm::vec2 topLeft, float scale, float cutPos, const glm::vec4& color = glm::vec4(1.f), float zIndex = -0.9f, const glm::uvec4& info = glm::uvec4(0));
        void RenderTextClippedBot(const char* text, const glm::vec2 topLeft, float scale, float cutPos, const glm::vec4& color = glm::vec4(1.f), float zIndex = -0.9f, const glm::uvec4& info = glm::uvec4(0));

        //renders one letter in different color
        void RenderText(const char* text, const glm::vec2 topLeft, float scale, const glm::vec4& color1, const glm::vec4& color2, int letterIdx, float zIndex = -0.9f, const glm::uvec4& info = glm::uvec4(0));
        void RenderTextCentered(const char* text, const glm::vec2 center, float scale, const glm::vec4& color1, const glm::vec4& color2, int letterIdx, float zIndex = -0.9f, const glm::uvec4& info = glm::uvec4(0));
        void RenderTextCentered(const char* text, const glm::vec2 center, float scale, const glm::vec4& color1, const glm::vec4& color2, int letterIdx, const glm::ivec2& pxOffset, float zIndex = -0.9f, const glm::uvec4& info = glm::uvec4(0));

        void Resize(int newHeight);

        TextureRef GetTexture() { return texture; }
    private:
        void Load(const std::string& filepath);
        
        void Release() noexcept;
        void Move(Font&&) noexcept;
    private:
        std::string name;
        std::string filepath;

        int fontHeight;
        CharInfo chars[128];
        TextureRef texture = nullptr;

        int rowHeight;

        glm::vec2 atlasSize_inv;
    };

}//namespace eng
