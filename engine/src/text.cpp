#include "engine/core/text.h"

#include "engine/core/quad.h"
#include "engine/core/renderer.h"
#include "engine/core/window.h"
#include "engine/utils/utils.h"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace eng {

    Font::Font(const std::string& filepath_, int fontHeight_) : fontHeight(fontHeight_), name(GetFilename(filepath_)), filepath(filepath_) {
        Load(filepath);
        ENG_LOG_TRACE("[C] Font '{}' (height = {})", name.c_str(), fontHeight);
    }

    Font::~Font() {
        Release();
    }

    Font::Font(Font&& f) noexcept {
        Move(std::move(f));
    }

    Font& Font::operator=(Font&& f) noexcept {
        Release();
        Move(std::move(f));
        return *this;
    }

    void Font::RenderText(const char* text, const glm::vec2 topLeft, float scale, const glm::vec4& color, float zIndex, const glm::uvec4& info) {
        glm::vec2 size_mult = scale / glm::vec2(Window::Get().Size());
        glm::vec2 pos = topLeft;

        for(const char* c = text; *c; c++) {
            const CharInfo& ch = GetChar(*c);

            Renderer::RenderQuad(Quad::Char(info, ch, pos, size_mult, atlasSize_inv, color, texture, zIndex));

            pos += glm::vec2(ch.advance) * size_mult;
        }
    }

    void Font::RenderText(const char* text, size_t max_len, const glm::vec2 topLeft, float scale, const glm::vec4& color, float zIndex, const glm::uvec4& info) {
        glm::vec2 size_mult = scale / glm::vec2(Window::Get().Size());
        glm::vec2 pos = topLeft;

        int i = 0;
        for(const char* c = text; *c && i < max_len; c++, i++) {
            const CharInfo& ch = GetChar(*c);

            Renderer::RenderQuad(Quad::Char(info, ch, pos, size_mult, atlasSize_inv, color, texture, zIndex));

            pos += glm::vec2(ch.advance) * size_mult;
        }
    }

    void Font::RenderTextClippedTop(const char* text, const glm::vec2 topLeft, float scale, float cutPos, const glm::vec4& color, float zIndex, const glm::uvec4& info) {
        glm::vec2 size_mult = scale / glm::vec2(Window::Get().Size());
        glm::vec2 pos = topLeft;

        int clipPos = int((1.f-cutPos) * rowHeight);

        for(const char* c = text; *c; c++) {
            const CharInfo& ch = GetChar(*c);

            Renderer::RenderQuad(Quad::CharClippedTop(info, ch, pos, size_mult, atlasSize_inv, color, texture, zIndex, clipPos));

            pos += glm::vec2(ch.advance) * size_mult;
        }
    }

    void Font::RenderTextClippedBot(const char* text, const glm::vec2 topLeft, float scale, float cutPos, const glm::vec4& color, float zIndex, const glm::uvec4& info) {
        glm::vec2 size_mult = scale / glm::vec2(Window::Get().Size());
        glm::vec2 pos = topLeft;

        int clipPos = int((1.f-cutPos) * rowHeight);

        for(const char* c = text; *c; c++) {
            const CharInfo& ch = GetChar(*c);

            Renderer::RenderQuad(Quad::CharClippedBot(info, ch, pos, size_mult, atlasSize_inv, color, texture, zIndex, clipPos));

            pos += glm::vec2(ch.advance) * size_mult;
        }
    }

    void Font::RenderTextCentered(const char* text, const glm::vec2 center, float scale, const glm::vec4& color, float zIndex, const glm::uvec4& info) {
        int width = 0;
        int height = 0;
        for (const char* c = text; *c; c++) {
            const CharInfo& ch = GetChar(*c);
            
            int charHeight = ch.advance.x * scale;
            height = std::max(charHeight, height);
            width += ch.advance.x * scale;
        }

        glm::vec2 offset = glm::vec2(width / 2, height / 2) / glm::vec2(Window::Get().Size());
        RenderText(text, center - offset, scale, color, zIndex, info);
    }

    void Font::RenderTextAlignLeft(const char* text, const glm::vec2 center, float scale, const glm::vec4& color, float zIndex, const glm::uvec4& info) {
        int height = 0;
        for (const char* c = text; *c; c++) {
            const CharInfo& ch = GetChar(*c);
            
            int charHeight = ch.advance.x * scale;
            height = std::max(charHeight, height);
        }

        glm::vec2 offset = glm::vec2(0.f, height / 2) / glm::vec2(Window::Get().Size());
        RenderText(text, center - offset, scale, color, zIndex, info);
    }

    void Font::RenderTextAlignRight(const char* text, size_t max_len, const glm::vec2 center, float scale, const glm::vec4& color, float zIndex, const glm::uvec4& info) {
        int height = 0;
        int width = 0;
        int i = 0;
        for (const char* c = text; *c; c++) {
            const CharInfo& ch = GetChar(*c);
            
            int charHeight = ch.advance.x * scale;
            height = std::max(charHeight, height);
            width += ch.advance.x * scale;

            if(i++ >= max_len)
                break;
        }

        glm::vec2 offset = glm::vec2(width, height / 2) / glm::vec2(Window::Get().Size());
        RenderText(text, max_len, center - offset, scale, color, zIndex, info);
    }

    void Font::RenderTextAlignLeft(const char* text, const glm::vec2 center, float scale, const glm::vec4& color1, const glm::vec4& color2, int letterIdx, float zIndex, const glm::uvec4& info) {
        int height = 0;
        for (const char* c = text; *c; c++) {
            const CharInfo& ch = GetChar(*c);
            
            int charHeight = ch.advance.x * scale;
            height = std::max(charHeight, height);
        }

        glm::vec2 offset = glm::vec2(0.f, height / 2) / glm::vec2(Window::Get().Size());
        RenderText(text, center - offset, scale, color1, color2, letterIdx, zIndex, info);
    }

    void Font::RenderTextAlignLeft(const char* text, const glm::ivec2& highlightRange, const glm::vec2 center, float scale, const glm::vec4& color1, const glm::vec4& color2, float zIndex, const glm::uvec4& info) {
        int height = 0;
        for (const char* c = text; *c; c++) {
            const CharInfo& ch = GetChar(*c);
            
            int charHeight = ch.advance.x * scale;
            height = std::max(charHeight, height);
        }

        glm::vec2 offset = glm::vec2(0.f, height / 2) / glm::vec2(Window::Get().Size());
        RenderText(text, center - offset, scale, color1, color2, highlightRange, zIndex, info);
    }

    void Font::RenderTextKeyValue(const char* text, size_t max_len, const glm::vec2 center, float scale, const glm::vec4& color, float zIndex, const glm::uvec4& info) {
        int height = 0;
        int width = 0;
        for (const char* c = text; *c; c++) {
            const CharInfo& ch = GetChar(*c);
            
            int charHeight = ch.advance.x * scale;
            height = std::max(charHeight, height);

            if(c < text+max_len) {
                width += ch.advance.x * scale;
            }
        }
        glm::vec2 offset;

        //render key - aligned right (center anchors separator character)
        offset = glm::vec2(width, height / 2) / glm::vec2(Window::Get().Size());
        RenderText(text, max_len, center - offset, scale, color, zIndex, info);

        //render value - aligned left
        offset = glm::vec2(0.f, height / 2) / glm::vec2(Window::Get().Size());
        RenderText(text + max_len, center - offset, scale, color, zIndex, info);
    }

    void Font::RenderTextKeyValue(const char* text, size_t max_len, const glm::vec2 center, float scale, const glm::vec4& color1, const glm::vec4& color2, const glm::ivec2& highlightRange, float zIndex, const glm::uvec4& info) {
        int height = 0;
        int width = 0;
        for (const char* c = text; *c; c++) {
            const CharInfo& ch = GetChar(*c);
            
            int charHeight = ch.advance.x * scale;
            height = std::max(charHeight, height);

            if(c < text+max_len) {
                width += ch.advance.x * scale;
            }
        }
        glm::vec2 offset;

        //render key - aligned right (center anchors separator character)
        offset = glm::vec2(width, height / 2) / glm::vec2(Window::Get().Size());
        RenderText(text, max_len, center - offset, scale, color1, zIndex, info);

        //render value - aligned left
        offset = glm::vec2(0.f, height / 2) / glm::vec2(Window::Get().Size());
        RenderText(text + max_len, center - offset, scale, color1, color2, highlightRange, zIndex, info);
    }

    void Font::RenderText(const char* text, const glm::vec2 topLeft, float scale, const glm::vec4& color1, const glm::vec4& color2, 
            int letterIdx, float zIndex, const glm::uvec4& info) {
        
        glm::vec2 size_mult = scale / glm::vec2(Window::Get().Size());
        glm::vec2 pos = topLeft;

        int i = 0;
        for(const char* c = text; *c; c++) {
            const CharInfo& ch = GetChar(*c);

            Renderer::RenderQuad(Quad::Char(info, ch, pos, size_mult, atlasSize_inv, (i != letterIdx ? color1 : color2), texture, zIndex));

            pos += glm::vec2(ch.advance) * size_mult;
            i++;
        }
    }

    void Font::RenderText(const char* text, const glm::vec2 topLeft, float scale, const glm::vec4& color1, const glm::vec4& color2, 
            const glm::ivec2& highlightRange, float zIndex, const glm::uvec4& info) {
        
        glm::vec2 size_mult = scale / glm::vec2(Window::Get().Size());
        glm::vec2 pos = topLeft;
        int a = highlightRange.x;
        int b = highlightRange.y;

        int i = 0;
        for(const char* c = text; *c; c++) {
            const CharInfo& ch = GetChar(*c);

            Renderer::RenderQuad(Quad::Char(info, ch, pos, size_mult, atlasSize_inv, ((i >= a && i < b) ? color2 : color1), texture, zIndex));

            pos += glm::vec2(ch.advance) * size_mult;
            i++;
        }
    }

    void Font::RenderTextCentered(const char* text, const glm::vec2 center, float scale, const glm::vec4& color1, const glm::vec4& color2, 
            int letterIdx, float zIndex, const glm::uvec4& info) {
        int width = 0;
        int height = 0;
        for (const char* c = text; *c; c++) {
            const CharInfo& ch = GetChar(*c);
            
            int charHeight = ch.advance.x * scale;
            height = std::max(charHeight, height);
            width += ch.advance.x * scale;
        }

        glm::vec2 offset = glm::vec2(width / 2, height / 2) / glm::vec2(Window::Get().Size());
        RenderText(text, center - offset, scale, color1, color2, letterIdx, zIndex, info);
    }

    void Font::RenderTextCentered(const char* text, const glm::vec2 center, float scale, const glm::vec4& color1, const glm::vec4& color2,
            int letterIdx, const glm::ivec2& pxOffset, float zIndex, const glm::uvec4& info) {
        int width = 0;
        int height = 0;
        for (const char* c = text; *c; c++) {
            const CharInfo& ch = GetChar(*c);
            
            int charHeight = ch.advance.x * scale;
            height = std::max(charHeight, height);
            width += ch.advance.x * scale;
        }

        glm::vec2 offset = glm::vec2(width / 2 - pxOffset.x, height / 2 + pxOffset.y) / glm::vec2(Window::Get().Size());
        RenderText(text, center - offset, scale, color1, color2, letterIdx, zIndex, info);
    }

    void Font::RenderTextCentered(const char* text, const glm::vec2 center, float scale, const glm::vec4& color1, const glm::vec4& color2,
            const glm::ivec2& highlightRange, const glm::ivec2& pxOffset, float zIndex, const glm::uvec4& info) {
        int width = 0;
        int height = 0;
        for (const char* c = text; *c; c++) {
            const CharInfo& ch = GetChar(*c);
            
            int charHeight = ch.advance.x * scale;
            height = std::max(charHeight, height);
            width += ch.advance.x * scale;
        }

        glm::vec2 offset = glm::vec2(width / 2 - pxOffset.x, height / 2 + pxOffset.y) / glm::vec2(Window::Get().Size());
        RenderText(text, center - offset, scale, color1, color2, highlightRange, zIndex, info);
    }

    void Font::Resize(int newHeight) {
        if(fontHeight != newHeight) {
            fontHeight = newHeight;
            Load(filepath);
        }
    }

    void Font::Load(const std::string& filepath) {
        FT_Library ft;
	    FT_Face face;

        //library initialization
        if (FT_Init_FreeType(&ft)) {
            ENG_LOG_ERROR("FreeType - Initialization failed.");
            throw std::exception();
        }

        //load font data
        if (FT_New_Face(ft, filepath.c_str(), 0, &face)) {
            ENG_LOG_ERROR("FreeType - Font failed to load.");
            throw std::exception();
        }

        //set font size
	    FT_Set_Pixel_Sizes(face, 0, fontHeight);

        unsigned int atlasWidth = 0;
        unsigned int atlasHeight = 0;

        unsigned char char_start = 32;
        unsigned char char_end = 128;
        FT_GlyphSlot g = face->glyph;

        //compute texture atlas dimensions (all glyphs will be in 1 row)
        for (unsigned char c = char_start; c < char_end; c++) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                ENG_LOG_DEBUG("FreeType - Glyph '{}' failed to load.", c);
                continue;
            }

            atlasWidth += g->bitmap.width;
            atlasHeight = std::max(atlasHeight, face->glyph->bitmap.rows);
        }

        //initialize atlas
        texture = std::make_shared<Texture>(TextureParams::EmptyTexture(atlasWidth, atlasHeight, GL_RED), std::string("atlas_") + name);
        texture->Bind(0);
        atlasSize_inv = glm::vec2(1.f / atlasWidth, 1.f / atlasHeight);

        //zero the texture out to get rid of the noise - not sure if required or not
        uint8_t* data = new uint8_t[atlasWidth * atlasHeight];
        for(int i = 0; i < atlasWidth * atlasHeight; i++) data[i] = 0;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, atlasWidth, atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, data);
        delete[] data;

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);


        rowHeight = 0;

        //load each glyph into the texture
        int x = 0;
        for (unsigned char c = char_start; c < char_end; c++) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                continue;
            }

            glTexSubImage2D(GL_TEXTURE_2D, 0, x, 0, g->bitmap.width, g->bitmap.rows, GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);

            chars[c] = CharInfo{
                glm::ivec2(g->bitmap.width, g->bitmap.rows),
                glm::ivec2(g->bitmap_left, g->bitmap_top),
                glm::ivec2(g->advance.x >> 6, g->advance.y >> 6),
                float(x)
            };

            rowHeight = std::max(rowHeight, chars[c].bearing.y);

            x += g->bitmap.width;
        }

        // glPixelStorei(GL_UNPACK_ALIGNMENT, 0);
        Texture::Unbind(0);

        //library cleanup
        FT_Done_Face(face);
        FT_Done_FreeType(ft);
    }

    void Font::Release() noexcept {
        if(texture != nullptr) {
            texture = nullptr;
            ENG_LOG_TRACE("[D] Font '{}' (height = {})", name.c_str(), fontHeight);
        }
    }

    void Font::Move(Font&& f) noexcept {
        texture = std::move(f.texture);
        name = std::move(f.name);
        filepath = std::move(f.filepath);
        atlasSize_inv = f.atlasSize_inv;
        fontHeight = f.fontHeight;
        memcpy(chars, f.chars, sizeof(chars));

        f.texture = nullptr;
    }

}//namespace eng
