#pragma once

#include "engine/utils/mathdefs.h"
#include "engine/core/texture.h"
#include "engine/core/text.h"

namespace eng {

    //======= Vertex =======

    struct Vertex {
        glm::vec3 position;
        glm::vec4 color;
        glm::vec2 texCoords;

        uint32_t textureID = 0;
        float alphaFromTexture = 0.f;

        glm::uvec4 info;
    public:
        Vertex() = default;
        Vertex(const glm::vec3& pos, const glm::vec2& tc, const glm::uvec4& info);
        Vertex(const glm::vec3& pos, const glm::vec4& clr, const glm::vec2& tc, const glm::uvec4& info);
    };

    //======= QuadVertices =======

    //Helper struct to represent quad in the vertex buffer.
    struct QuadVertices {
        Vertex vertices[4];
    public:
        Vertex& operator[](int i);
        const Vertex& operator[](int i) const;

        void UpdateTextureIdx(uint32_t texIdx);
        void UpdateTextureCoords(const TexCoords& tc);

        //Position interpolation over the quad. Parameters x,y should be in <0,1>.
        glm::vec3 PositionAt(float x, float y);

        void SetAlphaFromTexture(bool enabled);
    };

    //======= QuadIndices =======

    //Helper struct to represent quad in the element buffer.
    //5th entry is used as a separator (primitive restart).
    struct QuadIndices {
        uint32_t indices[5];
    public:
        QuadIndices() = default;
        QuadIndices(int idx);
    };

    //======= QuadType =======

    //Classifies a broader group of quad's purpose (describes what it's function on the screen is).
    namespace QuadType {
        enum { None = 0, Terrain, GameObject, GUI, Animator };
    }//namespace QuadType

    //======= Quad =======

    //Description of a screen quad (2 triangles, which share common edge).
    //Lengths should be in screen coordinates for rendering (range <-1,1>).
    struct Quad {
        QuadVertices vertices;
        TextureRef tex = nullptr;

        /* Quad vertices indexing:
            1----3
            |\   |
            | \  |
            |  \ |    
            |   \|
            0----2
        */
    public:
        Quad() = default;
        Quad(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& v4, const glm::vec4& color, const TextureRef& texture = nullptr, const TexCoords& tc = TexCoords::Default());
        Quad(const glm::uvec4& info, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& v4, const glm::vec4& color, const TextureRef& texture = nullptr, const TexCoords& tc = TexCoords::Default());

        static Quad FromCenter(const glm::vec3& center, const glm::vec2& halfSize, const glm::vec4& color, const TextureRef& texture = nullptr, const TexCoords& tc = TexCoords::Default());
        static Quad FromCorner(const glm::vec3& botLeft, const glm::vec2& size, const glm::vec4& color, const TextureRef& texture = nullptr, const TexCoords& tc = TexCoords::Default());
        static Quad SpriteQuad(const glm::vec3& botLeft, const glm::vec3& size, const glm::vec4& color, const TextureRef& texture, const TexCoords& tc, int depthMode);
        static Quad Tile(const glm::vec3& botLeft, const glm::vec3& right, const glm::vec3& fwd, const glm::vec4& color, const TextureRef& texture = nullptr, const TexCoords& tc = TexCoords::Default());

        static Quad FromCenter(const glm::uvec4& info, const glm::vec3& center, const glm::vec2& halfSize, const glm::vec4& color, const TextureRef& texture = nullptr, const TexCoords& tc = TexCoords::Default());
        static Quad FromCorner(const glm::uvec4& info, const glm::vec3& botLeft, const glm::vec2& size, const glm::vec4& color, const TextureRef& texture = nullptr, const TexCoords& tc = TexCoords::Default());
        static Quad SpriteQuad(const glm::uvec4& info, const glm::vec3& botLeft, const glm::vec3& size, const glm::vec4& color, const TextureRef& texture, const TexCoords& tc, int depthMode);
        static Quad Tile(const glm::uvec4& info, const glm::vec3& botLeft, const glm::vec3& right, const glm::vec3& fwd, const glm::vec4& color, const TextureRef& texture = nullptr, const TexCoords& tc = TexCoords::Default());

        static Quad Tile(const glm::uvec4& info, const glm::vec3& botLeft, const glm::vec3& right, const glm::vec3& fwd, const glm::vec3& up, const glm::vec4& h, const glm::vec4& color, const TextureRef& texture = nullptr, const TexCoords& tc = TexCoords::Default());

        static Quad Char(const glm::uvec4& info, const CharInfo& c, const glm::vec2& pos, const glm::vec2& size_mult, const glm::vec2& texSize_inv, const glm::vec4& color, const TextureRef& texture, float z = 0.f);

        static glm::uvec4 DefaultInfo() { return glm::uvec4(0); }
    };

}//namespace eng
