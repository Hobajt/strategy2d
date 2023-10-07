#pragma once

#include "engine/core/texture.h"

namespace eng {

    namespace TextureGenerator {

        //Creates a very basic button texture (plain color, borders & simple shading).
        TextureRef ButtonTexture_Clear(int width, int height, int borderWidth, int shadingWidth, int channel, bool flipShading);
        TextureRef ButtonTexture_Clear(int width, int height, int bw, int sw, const glm::u8vec3& fill, const glm::u8vec3& border, const glm::u8vec3& light, const glm::u8vec3& shadow);
        TextureRef ButtonTexture_Clear(int width, int height, int bw, int sw, const glm::u8vec4& fill, const glm::u8vec4& border, const glm::u8vec4& light, const glm::u8vec4& shadow);
        TextureRef ButtonTexture_Clear_2borders(int width, int height, int bw, int sw, const glm::u8vec3& fill, const glm::u8vec3& border, const glm::u8vec3& light, const glm::u8vec3& shadow, const glm::u8vec3& b2, int b2w);
        TextureRef ButtonTexture_Triangle(int width, int height, int borderWidth, bool flipShading, bool up);

        //ratio is screen width/height - to make it a circle rather than ellipsoid
        TextureRef ButtonTexture_Gem(int width, int height, int borderWidth, float ratio);

        //Yellow outline that marks selected button.
        TextureRef ButtonHighlightTexture(int width, int height, int borderWidth);
        TextureRef ButtonHighlightTexture(int width, int height, int borderWidth, const glm::u8vec4& clr);


    }//namespace TextureGenerator

}//namespace eng
