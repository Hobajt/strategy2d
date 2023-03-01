#pragma once

#include "engine/core/texture.h"

namespace eng {

    namespace TextureGenerator {

        //Creates a very basic button texture (plain color, borders & simple shading).
        TextureRef ButtonTexture_Clear(int width, int height, int borderWidth, int channel, bool flipShading);

        //Yellow outline that marks selected button.
        TextureRef ButtonHighlightTexture(int width, int height, int borderWidth);

    }//namespace TextureGenerator

}//namespace eng
