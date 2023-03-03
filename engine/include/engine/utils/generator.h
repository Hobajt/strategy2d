#pragma once

#include "engine/core/texture.h"

namespace eng {

    namespace TextureGenerator {

        //Creates a very basic button texture (plain color, borders & simple shading).
        TextureRef ButtonTexture_Clear(int width, int height, int borderWidth, int shadingWidth, int channel, bool flipShading);
        TextureRef ButtonTexture_Triangle(int width, int height, int borderWidth, bool flipShading, bool up);
        TextureRef ButtonTexture_Gem(int width, int height, int borderWidth, bool flipShading);

        //Yellow outline that marks selected button.
        TextureRef ButtonHighlightTexture(int width, int height, int borderWidth);


    }//namespace TextureGenerator

}//namespace eng
