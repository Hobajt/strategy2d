#include "engine/utils/generator.h"

#include "engine/core/window.h"

namespace eng {

    namespace TextureGenerator {

        using rgb = glm::u8vec3;
        using rgba = glm::u8vec4;

        void basicButton(rgb* data, int width, int height, int channel, int bw, int sw, bool flipShading);
        void triangle(rgb* data, int width, int height, int left, int right, int top, int bottom, bool flipShading, bool up);

        //========================================

        TextureRef ButtonTexture_Clear(int width, int height, int bw, int sw, int channel, bool flipShading) {
            if(((unsigned int)channel) > 3) {
                ENG_LOG_ERROR("TextureGenerator - valid channel values are only 0,1,2 (rgb channels)");
                return nullptr;
            }
            
            rgb* data = new rgb[width * height];

            basicButton(data, width, height, channel, bw, sw, flipShading);

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

        TextureRef ButtonTexture_Triangle(int width, int height, int borderWidth, bool flipShading, bool up) {
            rgb* data = new rgb[width * height];

            basicButton(data, width, height, 0, borderWidth, borderWidth*2, flipShading);
            triangle(data, width, height, borderWidth*4, borderWidth*5, borderWidth*4, borderWidth*5, flipShading, up);

            //--------------------------

            char buf[256];
            snprintf(buf, sizeof(buf), "btnTex_triangle_%s%s", up ? "up" : "down", flipShading ? "_flip" : "");

            TextureRef texture = std::make_shared<Texture>(
                TextureParams::CustomData(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE),
                (void*)data,
                std::string(buf)
            );

            delete[] data;
            return texture;
        }

        TextureRef ButtonTexture_Gem(int width, int height, int borderWidth, bool flipShading) {
            rgb* data = new rgb[width * height];

            basicButton(data, width, height, 0, borderWidth, flipShading, flipShading);
            // triangle(data, borderWidth*2, borderWidth*2, width-borderWidth*2, height-borderWidth*3, width, flipShading);

            //--------------------------

            char buf[256];
            snprintf(buf, sizeof(buf), "btnTex_gem%s", flipShading ? "_flip" : "");

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

        //===============================

        void basicButton(rgb* data, int width, int height, int channel, int bw, int sw, bool flipShading) {
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

            //borders - top & bottom
            for(int y = 0; y < bw; y++) {
                for(int x = 0; x < width; x++) {
                    data[y * width + x] = border;
                    data[(height-1-y) * width + x] = border;
                }
            }
            //shading - top & bottom
            for(int y = 0; y < sw; y++) {
                for(int x = 0; x < width; x++) {
                    data[(y+bw) * width + x] = light;
                    data[(height-1-y-bw) * width + x] = shadow;
                }
            }
            //borders - left & right
            for(int y = bw; y < height-bw; y++) {
                for(int x = 0; x < bw; x++) {
                    data[y * width + x] = border;
                    data[y * width + width-1-x] = border;
                }
            }
            for(int y = bw; y < height-bw; y++) {
                for(int x = 0; x < sw; x++) {
                    data[y * width + x+bw] = light;
                    data[y * width + width-1-x-bw] = shadow;
                }
            }
            //3d outlines - corners (botLeft & topRight)
            for(int y = 0; y < sw; y++) {
                for(int x = 0; x < sw; x++) {
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
        }

        void triangle(rgb* data, int width, int height, int left, int right, int top, int bottom, bool flipShading, bool up) {
            rgb fill = flipShading ? rgb(171, 143, 19) : rgb(191, 167, 23);
            rgb light = flipShading ? rgb(212, 196, 31) : rgb(225, 203, 30);
            rgb shadow = flipShading ? rgb(145, 122, 16) : rgb(176, 154, 19);

            if(flipShading) {
                int tmp = left;
                left = right;
                right = tmp;

                tmp = top;
                top = bottom;
                bottom = top;
            }

            //r=range,c=center
            int rx = width - right - left;
            int ry = height - top - bottom;
            int cx = rx / 2;
            float step = float(cx)/ry;      //step in x per one move in y
            int sh = rx / 8;
            int sho = rx / 16;
            
            for(int i = 0; i < ry; i++) {
                int istep = int(step * (i*up + (ry-1-i)*(1-up)));
                int xs = left+cx-istep;
                int xe = left+cx+istep;

                //triangle
                for(int x = xs; x < xe; x++) {
                    data[(top+i)*width + x] = fill;
                }

                //shading
                int se = std::min(xs+sh+sho, xe);
                for(int x = xs + sho; x < se; x++) {
                    data[(top+i)*width + x] = light;
                }
                for(int x = se; x < std::min(se+sho,xe); x++) {
                    data[(top+i)*width + x] = shadow;
                }
            }

        }

    }//namespace TextureGenerator

}//namespace eng
