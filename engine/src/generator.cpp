#include "engine/utils/generator.h"

#include "engine/core/window.h"

namespace eng {

    namespace TextureGenerator {

        using rgb = glm::u8vec3;
        using rgba = glm::u8vec4;

        TextureRef ButtonTexture_Clear(int width, int height, int bw, int channel, bool flipShading) {
            if(((unsigned int)channel) > 3) {
                ENG_LOG_ERROR("TextureGenerator - valid channel values are only 0,1,2 (rgb channels)");
                return nullptr;
            }
            
            rgb* data = new rgb[width * height];

            rgb fill = rgb(0);
            rgb border = rgb(0);
            rgb light = rgb(0);
            rgb shadow = rgb(0);
            rgb darkShadow = rgb(0);
            rgb darkFill = rgb(0);

            fill[channel] = flipShading ? 110 : 150;
            light[channel] = flipShading ? 80 : 200;
            shadow[channel] = flipShading ? 150 : 100;
            border[channel] = 30;

            //--------------------------

            //fill
            for(int y = 0; y < height; y++) {
                for(int x = 0; x < width; x++) {
                    data[y * width + x] = fill;
                }
            }

            //borders + shading outlines - top & bottom
            for(int y = 0; y < bw; y++) {
                for(int x = 0; x < width; x++) {
                    data[y * width + x] = border;
                    data[(height-1-y) * width + x] = border;

                    data[(y+bw) * width + x] = light;
                    data[(height-1-y-bw) * width + x] = shadow;
                }
            }
            //borders + 3d outlines - left & right
            for(int y = bw; y < height-bw; y++) {
                for(int x = 0; x < bw; x++) {
                    data[y * width + x] = border;
                    data[y * width + width-1-x] = border;

                    data[y * width + x+bw] = light;
                    data[y * width + width-1-x-bw] = shadow;
                }
            }
            //3d outlines - corners (botLeft & topRight)
            for(int y = 0; y < bw; y++) {
                for(int x = 0; x < bw; x++) {
                    if(atan2(y, x) > glm::pi<double>() * 0.25) {
                        data[(y+bw) * width + width-1-x-bw] = shadow;
                        data[(height-1-y-bw) * width + x+bw] = light;
                    }
                    else {
                        data[(y+bw) * width + width-1-x-bw] = light;
                        data[(height-1-y-bw) * width + x+bw] = shadow;
                    }
                }
            }


            //--------------------------

            char buf[256];
            const char* c = "rgb";
            snprintf(buf, sizeof(buf), "btnTex_clear_%c%s", c[channel], flipShading ? "_flip" : "");

            TextureRef texture = std::make_shared<Texture>(
                TextureParams::CustomData(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE),
                (void*)data,
                std::string(buf)
            );
            delete[] data;
            return texture;
        }

        TextureRef ButtonHighlightTexture(int width, int height, int bw) {
            rgba* data = new rgba[width * height];

            rgba fill = rgba(255, 255, 255, 0);
            rgba highlight = rgba(255, 255, 0, 255);

            //--------------------------
            //fill
            for(int y = 0; y < height; y++) {
                for(int x = 0; x < width; x++) {
                    data[y * width + x] = fill;
                }
            }

            //borders - top & bottom
            for(int y = 0; y < bw; y++) {
                for(int x = 0; x < width; x++) {
                    data[y * width + x] = highlight;
                    data[(height-1-y) * width + x] = highlight;
                }
            }
            //borders - left & right
            for(int y = bw; y < height-bw; y++) {
                for(int x = 0; x < bw; x++) {
                    data[y * width + x] = highlight;
                    data[y * width + width-1-x] = highlight;
                }
            }
            //--------------------------

            char buf[256];
            snprintf(buf, sizeof(buf), "btnHighlightTex_%dx%d", width, height);

            TextureRef texture = std::make_shared<Texture>(
                TextureParams::CustomData(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE),
                (void*)data,
                std::string(buf)
            );
            delete[] data;
            return texture;
        }

    }//namespace TextureGenerator

}//namespace eng
