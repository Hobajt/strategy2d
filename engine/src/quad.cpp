#include "engine/core/quad.h"

namespace eng {

    //======= Vertex =======

    Vertex::Vertex(const glm::vec3& pos, const glm::vec2& tc, const glm::uvec4& i) : Vertex(pos, glm::vec4(1.f), tc, i) {}

    Vertex::Vertex(const glm::vec3& pos, const glm::vec4& clr, const glm::vec2& tc, const glm::uvec4& i) : position(pos), color(clr), texCoords(tc), info(i) {}

    //======= QuadVertices =======

    Vertex& QuadVertices::operator[](int i) {
        return vertices[i];
    }

    const Vertex& QuadVertices::operator[](int i) const {
        return vertices[i];
    }

    void QuadVertices::UpdateTextureIdx(uint32_t texIdx) {
        for(int i = 0; i < 4; i++) {
            vertices[i].textureID = texIdx;
        }
    }

    void QuadVertices::UpdateTextureCoords(const TexCoords& tc) {
        for(int i = 0; i < 4; i++) {
            vertices[i].texCoords = tc[i];
        }
    }

    glm::vec3 QuadVertices::PositionAt(float x, float y) {
        glm::vec3 a = y * vertices[1].position + (1-y) * vertices[0].position;
        glm::vec3 b = y * vertices[3].position + (1-y) * vertices[2].position;
        return x * b + (1-x) * a;
    }

    void QuadVertices::SetAlphaFromTexture(bool enabled) {
        vertices[0].alphaFromTexture = vertices[1].alphaFromTexture = vertices[2].alphaFromTexture = vertices[3].alphaFromTexture = enabled ? 1.f : 0.f;
    }

    void QuadVertices::SetPaletteIdx(float idx) {
        vertices[0].paletteIdx = vertices[1].paletteIdx = vertices[2].paletteIdx = vertices[3].paletteIdx = idx;
    }

    //======= QuadIndices =======

    QuadIndices::QuadIndices(int idx) {
        for (int i = 0; i < 4; i++)
            indices[i] = (idx * 4) + i;
        indices[4] = (uint32_t)(-1);
    }

    //======= Quad =======

    Quad::Quad(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& v4, const glm::vec4& color, const TextureRef& texture, const TexCoords& tc)
        : Quad(DefaultInfo(), v1, v2, v3, v4, color, texture, tc) {}

    Quad::Quad(const glm::uvec4& info, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& v4, const glm::vec4& color, const TextureRef& texture, const TexCoords& tc) : tex(texture) {
        vertices[0] = Vertex(v1, color, tc[0], info);
        vertices[1] = Vertex(v2, color, tc[1], info);
        vertices[2] = Vertex(v3, color, tc[2], info);
        vertices[3] = Vertex(v4, color, tc[3], info);
    }

    Quad Quad::FromCenter(const glm::vec3& center, const glm::vec2& halfSize, const glm::vec4& color, const TextureRef& texture, const TexCoords& tc) {
        return FromCenter(DefaultInfo(), center, halfSize, color, texture, tc);
    }

    Quad Quad::FromCorner(const glm::vec3& botLeft, const glm::vec2& size, const glm::vec4& color, const TextureRef& texture, const TexCoords& tc) {
        return FromCorner(DefaultInfo(), botLeft, size, color, texture, tc);
    }

    Quad Quad::SpriteQuad(const glm::vec3& botLeft, const glm::vec3& size, const glm::vec4& color, const TextureRef& texture, const TexCoords& tc, int depthMode) {
        return SpriteQuad(DefaultInfo(), botLeft, size, color, texture, tc, depthMode);
    }

    Quad Quad::Tile(const glm::vec3& botLeft, const glm::vec3& right, const glm::vec3& fwd, const glm::vec4& color, const TextureRef& texture, const TexCoords& tc) {
        return Tile(DefaultInfo(), botLeft, right, fwd, color, texture, tc);
    }

    Quad Quad::FromCenter(const glm::uvec4& info, const glm::vec3& center, const glm::vec2& halfSize, const glm::vec4& color, const TextureRef& texture, const TexCoords& tc) {
        Quad q = {};

        q.tex = texture;

        q.vertices[0] = Vertex(center + glm::vec3(-halfSize.x, -halfSize.y, 0.f), color, tc[0], info);
        q.vertices[1] = Vertex(center + glm::vec3(-halfSize.x, +halfSize.y, 0.f), color, tc[1], info);
        q.vertices[2] = Vertex(center + glm::vec3(+halfSize.x, -halfSize.y, 0.f), color, tc[2], info);
        q.vertices[3] = Vertex(center + glm::vec3(+halfSize.x, +halfSize.y, 0.f), color, tc[3], info);

        return q;
    }
    
    Quad Quad::FromCorner(const glm::uvec4& info, const glm::vec3& botLeft, const glm::vec2& size, const glm::vec4& color, const TextureRef& texture, const TexCoords& tc) {
        Quad q = {};

        q.tex = texture;

        q.vertices[0] = Vertex(botLeft + glm::vec3(0.f, 0.f, 0.f),       color, tc[0], info);
        q.vertices[1] = Vertex(botLeft + glm::vec3(0.f, size.y, 0.f),    color, tc[1], info);
        q.vertices[2] = Vertex(botLeft + glm::vec3(size.x, 0.f, 0.f),    color, tc[2], info);
        q.vertices[3] = Vertex(botLeft + glm::vec3(size.x, size.y, 0.f), color, tc[3], info);
        
        return q;
    }

    Quad Quad::SpriteQuad(const glm::uvec4& info, const glm::vec3& botLeft, const glm::vec3& size, const glm::vec4& color, const TextureRef& texture, const TexCoords& tc, int depthMode) {
        Quad q = {};
        q.tex = texture;

        q.vertices[0] = Vertex(botLeft + glm::vec3(0.f, 0.f, size.z),       color, tc[0], info);
        q.vertices[1] = Vertex(botLeft + glm::vec3(0.f, size.y, size.z),    color, tc[1], info);
        q.vertices[2] = Vertex(botLeft + glm::vec3(size.x, 0.f, size.z),    color, tc[2], info);
        q.vertices[3] = Vertex(botLeft + glm::vec3(size.x, size.y, size.z), color, tc[3], info);

        static std::pair<int,int> idx[] = { {0,1}, {0,2}, {0,1} };
        q.vertices[idx[depthMode].first].position.z -= size.z;
        q.vertices[idx[depthMode].second].position.z -= size.z;

        return q;
    }

    Quad Quad::Tile(const glm::uvec4& info, const glm::vec3& botLeft, const glm::vec3& right, const glm::vec3& fwd, const glm::vec4& color, const TextureRef& texture, const TexCoords& tc) {
        Quad q = {};
        q.tex = texture;

        q.vertices[0] = Vertex(botLeft,               color, tc[0], info);
        q.vertices[1] = Vertex(botLeft + fwd,         color, tc[1], info);
        q.vertices[2] = Vertex(botLeft + right,       color, tc[2], info);
        q.vertices[3] = Vertex(botLeft + right + fwd, color, tc[3], info);

        return q;
    }

    Quad Quad::Tile(const glm::uvec4& info, const glm::vec3& botLeft, const glm::vec3& right, const glm::vec3& fwd, const glm::vec3& up, const glm::vec4& h, const glm::vec4& color, const TextureRef& texture, const TexCoords& tc) {
        Quad q = {};
        q.tex = texture;

        // printf("(%.2f, %.2f, %.2f, %.2f)\n", h[0], h[1], h[2], h[3]);
        q.vertices[0] = Vertex(botLeft + up * h[0],               color, tc[0], info);
        q.vertices[1] = Vertex(botLeft + fwd + up * h[1],         color, tc[1], info);
        q.vertices[2] = Vertex(botLeft + right + up * h[2],       color, tc[2], info);
        q.vertices[3] = Vertex(botLeft + right + fwd + up * h[3], color, tc[3], info);

        return q;
    }

    Quad Quad::Char(const glm::uvec4& info, const CharInfo& c, const glm::vec2& pos, const glm::vec2& size_mult, const glm::vec2& texSize_inv, const glm::vec4& color, const TextureRef& texture, float z) {
        Quad q = {};
        q.tex = texture;

        float x = pos.x + c.bearing.x * size_mult.x;
        float y = pos.y - (c.size.y - c.bearing.y) * size_mult.y;

        float w = c.size.x * size_mult.x;
        float h = c.size.y * size_mult.y;

        float tx = c.textureOffset;
        float ow = c.size.x;
        float oh = c.size.y;

        q.vertices[0] = Vertex(glm::vec3(x  , y  , z), color, glm::vec2(tx   , oh ) * texSize_inv, info);
        q.vertices[1] = Vertex(glm::vec3(x  , y+h, z), color, glm::vec2(tx   , 0.f) * texSize_inv, info);
        q.vertices[2] = Vertex(glm::vec3(x+w, y  , z), color, glm::vec2(tx+ow, oh ) * texSize_inv, info);
        q.vertices[3] = Vertex(glm::vec3(x+w, y+h, z), color, glm::vec2(tx+ow, 0.f) * texSize_inv, info);
        q.vertices.SetAlphaFromTexture(true);

        return q;
    }

    Quad Quad::CharClippedTop(const glm::uvec4& info, const CharInfo& c, const glm::vec2& pos, const glm::vec2& size_mult, const glm::vec2& texSize_inv, const glm::vec4& color, const TextureRef& texture, float z, int cp) {
        Quad q = {};
        q.tex = texture;

        float x = pos.x + c.bearing.x * size_mult.x;
        float y = pos.y - (c.size.y - c.bearing.y) * size_mult.y;

        int cut = std::max(c.size.y - c.bearing.y + cp, 0);

        float w = c.size.x * size_mult.x;
        float h = std::min(c.size.y, cut) * size_mult.y;

        float tx = c.textureOffset;
        float ow = c.size.x;
        float oh = c.size.y;

        float ch = std::max(c.size.y - cut, 0);

        q.vertices[0] = Vertex(glm::vec3(x  , y  , z), color, glm::vec2(tx   , oh) * texSize_inv, info);
        q.vertices[1] = Vertex(glm::vec3(x  , y+h, z), color, glm::vec2(tx   , ch) * texSize_inv, info);
        q.vertices[2] = Vertex(glm::vec3(x+w, y  , z), color, glm::vec2(tx+ow, oh) * texSize_inv, info);
        q.vertices[3] = Vertex(glm::vec3(x+w, y+h, z), color, glm::vec2(tx+ow, ch) * texSize_inv, info);
        q.vertices.SetAlphaFromTexture(true);

        return q;
    }

    Quad Quad::CharClippedBot(const glm::uvec4& info, const CharInfo& c, const glm::vec2& pos, const glm::vec2& size_mult, const glm::vec2& texSize_inv, const glm::vec4& color, const TextureRef& texture, float z, int cp) {
        Quad q = {};
        q.tex = texture;

        float floor = pos.y + cp * size_mult.y;

        float x = pos.x + c.bearing.x * size_mult.x;
        float y = pos.y - (c.size.y - c.bearing.y) * size_mult.y;

        float w = c.size.x * size_mult.x;
        float h = c.size.y * size_mult.y;

        float yl = std::max(y, floor);
        float yh = std::max(y+h, floor);

        float tx = c.textureOffset;
        float ow = c.size.x;
        float oh = std::max(c.size.y - c.size.y + c.bearing.y, cp) - std::max(-c.size.y + c.bearing.y, cp);

        q.vertices[0] = Vertex(glm::vec3(x  , yl, z), color, glm::vec2(tx   , oh ) * texSize_inv, info);
        q.vertices[1] = Vertex(glm::vec3(x  , yh, z), color, glm::vec2(tx   , 0.f) * texSize_inv, info);
        q.vertices[2] = Vertex(glm::vec3(x+w, yl, z), color, glm::vec2(tx+ow, oh ) * texSize_inv, info);
        q.vertices[3] = Vertex(glm::vec3(x+w, yh, z), color, glm::vec2(tx+ow, 0.f) * texSize_inv, info);
        q.vertices.SetAlphaFromTexture(true);

        return q;
    }

    Quad& Quad::SetPaletteIdx(float idx) {
        vertices.SetPaletteIdx(idx);
        return *this;
    }

}//namespace eng
