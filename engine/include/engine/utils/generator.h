#pragma once

#include <array>

#include "engine/core/texture.h"

namespace eng {

    namespace TextureGenerator {

        using rgb = glm::u8vec3;
        using rgba = glm::u8vec4;

        namespace GeneratedTextureType {
            enum {
                BUTTON1, BUTTON2, BUTTON3, BUTTON_2BORDERS, BUTTON_3BORDERS, BUTTON_TRIANGLE, BUTTON_GEM1, BUTTON_GEM2,
                BUTTON_HIGHLIGHT,
                SHADOWS, OCCLUSION,
                COUNT
            };
        }//namespace GeneratedTextureType

        namespace GeneratedTextureFlags {
            enum {
                FLIP_SHADING    = BIT(0),
                RGB_ONLY        = BIT(1),
                UP              = BIT(2),
            };
        }//namespace GeneratedTextureFlags

        struct Params {
            using colorArray = std::array<glm::u8vec4, 6>;
        public:
            int type;

            glm::ivec2 size;
            glm::ivec2 borderSize;
            glm::ivec2 shadingSize;
            int channel;
            glm::ivec2 w;
            float ratio;

            int flags = 0;
            colorArray colors;
        public:
            Params() = default;
            Params(int type, const glm::ivec2& size, const glm::ivec2& borderSize, const glm::ivec2& shadingSize, int channel, const glm::ivec2& w, float ratio, int flags, const colorArray& colors);

            static Params ButtonTexture_Clear(int width, int height, int borderWidth, int shadingWidth, int channel, bool flipShading);
            static Params ButtonTexture_Clear(int width, int height, int bw, int sw, const rgb& fill, const rgb& border, const rgb& light, const rgb& shadow);
            static Params ButtonTexture_Clear(int width, int height, int bw, int sw, const rgba& fill, const rgba& border, const rgba& light, const rgba& shadow);
            static Params ButtonTexture_Clear_2borders(int width, int height, int bw, int sw, const rgb& fill, const rgb& border, const rgb& light, const rgb& shadow, const rgb& b2, int b2w);
            static Params ButtonTexture_Clear_3borders(int width, int height, int bw, int sw, const rgb& fill, const rgb& border, const rgb& light, const rgb& shadow, const rgb& b2, int b2w, const rgb& b3, int b3w);
            static Params ButtonTexture_Triangle(int width, int height, int borderWidth, bool flipShading, bool up);
            static Params ButtonTexture_Gem(int width, int height, int borderWidth, float ratio, const rgba& fillColor = rgba(71,0,0,255));
            static Params ButtonTexture_Gem2(int width, int height, int borderWidth, float ratio, const rgba& clr1 = rgba(71,0,0,255), const rgba& clr2 = rgba(222,0,0,255));
            static Params ButtonHighlightTexture(int width, int height, int borderWidth, const rgba& clr = rgba(0));
            static Params ShadowsTexture(int width, int height, int size);
            static Params OcclusionTileset(int tile_size, int block_size);

            int Hash() const;
        };

        size_t Count();
        int InvokationCount();
        void Clear();

        TextureRef GetTexture(const Params& params);


    }//namespace TextureGenerator

}//namespace eng
