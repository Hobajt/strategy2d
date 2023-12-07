#pragma once

#include "engine/core/texture.h"

namespace eng {

    namespace TextureGenerator {

        //Creates a very basic button texture (plain color, borders & simple shading).
        TextureRef ButtonTexture_Clear(int width, int height, int borderWidth, int shadingWidth, int channel, bool flipShading);
        TextureRef ButtonTexture_Clear(int width, int height, int bw, int sw, const glm::u8vec3& fill, const glm::u8vec3& border, const glm::u8vec3& light, const glm::u8vec3& shadow);
        TextureRef ButtonTexture_Clear(int width, int height, int bw, int sw, const glm::u8vec4& fill, const glm::u8vec4& border, const glm::u8vec4& light, const glm::u8vec4& shadow);
        TextureRef ButtonTexture_Clear_2borders(int width, int height, int bw, int sw, const glm::u8vec3& fill, const glm::u8vec3& border, const glm::u8vec3& light, const glm::u8vec3& shadow, const glm::u8vec3& b2, int b2w);
        TextureRef ButtonTexture_Clear_3borders(int width, int height, int bw, int sw, const glm::u8vec3& fill, const glm::u8vec3& border, const glm::u8vec3& light, const glm::u8vec3& shadow, const glm::u8vec3& b2, int b2w, const glm::u8vec3& b3, int b3w);
        TextureRef ButtonTexture_Triangle(int width, int height, int borderWidth, bool flipShading, bool up);

        //ratio is screen width/height - to make it a circle rather than ellipsoid
        TextureRef ButtonTexture_Gem(int width, int height, int borderWidth, float ratio, const glm::u8vec4& fillColor = glm::u8vec4(71,0,0,255));
        TextureRef ButtonTexture_Gem2(int width, int height, int borderWidth, float ratio, const glm::u8vec4& clr1 = glm::u8vec4(71,0,0,255), const glm::u8vec4& clr2 = glm::u8vec4(222,0,0,255));

        //Yellow outline that marks selected button.
        TextureRef ButtonHighlightTexture(int width, int height, int borderWidth);
        TextureRef ButtonHighlightTexture(int width, int height, int borderWidth, const glm::u8vec4& clr);

        TextureRef ShadowsTexture(int width, int height, int size);

        TextureRef OcclusionTileset(int tile_size, int block_size);


    }//namespace TextureGenerator

}//namespace eng
