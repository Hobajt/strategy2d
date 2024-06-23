#include "engine/utils/generator.h"

#include "engine/core/window.h"

#include <utility>
#include <array>

namespace eng {

    namespace TextureGenerator {

        struct TextureCache {
            std::unordered_map<int, TextureRef> cache;
            int invokation_count = 0;
        public:
            bool Contains(int hash);
            TextureRef GetTexture(int hash);
            TextureRef Add(int hash, const TextureRef& texture);
            //TODO: either trigger merging automatically from Add(), or add explicit function & expose it
        };

        static TextureCache cache = {};

        //========== old API ==========
        
        TextureRef ButtonTexture_Clear1(const Params& params);
        TextureRef ButtonTexture_Clear2(const Params& params);
        TextureRef ButtonTexture_Clear3(const Params& params);
        TextureRef ButtonTexture_Clear_2borders(const Params& params);
        TextureRef ButtonTexture_Clear_3borders(const Params& params);
        TextureRef ButtonTexture_Triangle(const Params& params);
        TextureRef ButtonTexture_Gem(const Params& params);
        TextureRef ButtonTexture_Gem2(const Params& params);
        TextureRef ButtonHighlightTexture(const Params& params);
        TextureRef ShadowsTexture(const Params& params);
        TextureRef OcclusionTileset(const Params& params);

        //==============================

        //Creates a very basic button texture (plain color, borders & simple shading).
        TextureRef ButtonTexture_Clear(int width, int height, int borderWidth, int shadingWidth, int channel, bool flipShading);
        TextureRef ButtonTexture_Clear(int width, int height, int bw, int sw, const rgb& fill, const rgb& border, const rgb& light, const rgb& shadow);
        TextureRef ButtonTexture_Clear(int width, int height, int bw, int sw, const rgba& fill, const rgba& border, const rgba& light, const rgba& shadow);
        TextureRef ButtonTexture_Clear_2borders(int width, int height, int bw, int sw, const rgb& fill, const rgb& border, const rgb& light, const rgb& shadow, const rgb& b2, int b2w);
        TextureRef ButtonTexture_Clear_3borders(int width, int height, int bw, int sw, const rgb& fill, const rgb& border, const rgb& light, const rgb& shadow, const rgb& b2, int b2w, const rgb& b3, int b3w);
        TextureRef ButtonTexture_Triangle(int width, int height, int borderWidth, bool flipShading, bool up);

        //ratio is screen width/height - to make it a circle rather than ellipsoid
        TextureRef ButtonTexture_Gem(int width, int height, int borderWidth, float ratio, const rgba& fillColor = rgba(71,0,0,255));
        TextureRef ButtonTexture_Gem2(int width, int height, int borderWidth, float ratio, const rgba& clr1 = rgba(71,0,0,255), const rgba& clr2 = rgba(222,0,0,255));

        //Yellow outline that marks selected button.
        TextureRef ButtonHighlightTexture(int width, int height, int borderWidth);
        TextureRef ButtonHighlightTexture(int width, int height, int borderWidth, const rgba& clr);

        TextureRef ShadowsTexture(int width, int height, int size);
        TextureRef OcclusionTileset(int tile_size, int block_size);

        //============ Params ==================

        Params::colorArray setupColors(const rgba& c1 = rgba(0), const rgba& c2 = rgba(0), const rgba& c3 = rgba(0), const rgba& c4 = rgba(0), const rgba& c5 = rgba(0), const rgba& c6 = rgba(0)) {
            return Params::colorArray{ c1, c2, c3, c4, c5, c6 };
        }

        Params::colorArray setupColors(const rgb& c1, const rgb& c2 = rgb(0), const rgb& c3 = rgb(0), const rgb& c4 = rgb(0), const rgb& c5 = rgb(0), const rgb& c6 = rgb(0)) {
            return Params::colorArray{ rgba(c1, 0), rgba(c2, 0), rgba(c3, 0), rgba(c4, 0), rgba(c5, 0), rgba(c6, 0) };
        }

        Params::Params(int type_, const glm::ivec2& size_, const glm::ivec2& borderSize_, const glm::ivec2& shadingSize_, int channel_, const glm::ivec2& w_, float ratio_, int flags_, const colorArray& colors_)
            : type(type_), size(size_), borderSize(borderSize_), shadingSize(shadingSize_), channel(channel_), w(w_), ratio(ratio_), flags(flags_), colors(colors_) {}

        Params Params::ButtonTexture_Clear(int width, int height, int borderWidth, int shadingWidth, int channel, bool flipShading) {
            return Params(
                GeneratedTextureType::BUTTON1, glm::ivec2(width, height), glm::ivec2(borderWidth), glm::ivec2(shadingWidth), 
                channel, glm::ivec2(0), 1.f, flipShading * GeneratedTextureFlags::FLIP_SHADING, setupColors()
            );
        }

        Params Params::ButtonTexture_Clear(int width, int height, int bw, int sw, const rgb& fill, const rgb& border, const rgb& light, const rgb& shadow) {
            return Params(
                GeneratedTextureType::BUTTON2, glm::ivec2(width, height), glm::ivec2(bw), glm::ivec2(sw), 
                0, glm::ivec2(0), 1.f, 0, setupColors(fill, border, light, shadow)
            );
        }

        Params Params::ButtonTexture_Clear(int width, int height, int bw, int sw, const rgba& fill, const rgba& border, const rgba& light, const rgba& shadow) {
            return Params(
                GeneratedTextureType::BUTTON3, glm::ivec2(width, height), glm::ivec2(bw), glm::ivec2(sw), 
                0, glm::ivec2(0), 1.f, 0, setupColors(fill, border, light, shadow)
            );
        }

        Params Params::ButtonTexture_Clear_2borders(int width, int height, int bw, int sw, const rgb& fill, const rgb& border, const rgb& light, const rgb& shadow, const rgb& b2, int b2w) {
            return Params(
                GeneratedTextureType::BUTTON2, glm::ivec2(width, height), glm::ivec2(bw), glm::ivec2(sw), 
                0, glm::ivec2(b2w, 0), 1.f, 0, setupColors(fill, border, light, shadow, b2)
            );
        }

        Params Params::ButtonTexture_Clear_3borders(int width, int height, int bw, int sw, const rgb& fill, const rgb& border, const rgb& light, const rgb& shadow, const rgb& b2, int b2w, const rgb& b3, int b3w) {
            return Params(
                GeneratedTextureType::BUTTON2, glm::ivec2(width, height), glm::ivec2(bw), glm::ivec2(sw), 
                0, glm::ivec2(b2w, b3w), 1.f, 0, setupColors(fill, border, light, shadow, b2, b3)
            );
        }
        
        Params Params::ButtonTexture_Triangle(int width, int height, int borderWidth, bool flipShading, bool up) {
            return Params(
                GeneratedTextureType::BUTTON_TRIANGLE, glm::ivec2(width, height), glm::ivec2(borderWidth), glm::ivec2(0), 
                0, glm::ivec2(0), 1.f, (flipShading*GeneratedTextureFlags::FLIP_SHADING) | (up*GeneratedTextureFlags::UP), setupColors()
            );
        }

        Params Params::ButtonTexture_Gem(int width, int height, int borderWidth, float ratio, const rgba& fillColor) {
            return Params(
                GeneratedTextureType::BUTTON_GEM1, glm::ivec2(width, height), glm::ivec2(borderWidth), glm::ivec2(0), 
                0, glm::ivec2(0), ratio, 0, setupColors(fillColor)
            );
        }

        Params Params::ButtonTexture_Gem2(int width, int height, int borderWidth, float ratio, const rgba& clr1, const rgba& clr2) {
            return Params(
                GeneratedTextureType::BUTTON_GEM2, glm::ivec2(width, height), glm::ivec2(borderWidth), glm::ivec2(0), 
                0, glm::ivec2(0), ratio, 0, setupColors(clr1, clr2)
            );
        }

        Params Params::ButtonHighlightTexture(int width, int height, int borderWidth, const rgba& clr) {
            return Params(
                GeneratedTextureType::BUTTON_HIGHLIGHT, glm::ivec2(width, height), glm::ivec2(borderWidth), glm::ivec2(0), 
                0, glm::ivec2(0), 1.f, 0, setupColors(clr)
            );
        }

        Params Params::ShadowsTexture(int width, int height, int size) {
            return Params(
                GeneratedTextureType::SHADOWS, glm::ivec2(width, height), glm::ivec2(0), glm::ivec2(0), 
                size, glm::ivec2(0), 1.f, 0, setupColors()
            );
        }

        Params Params::OcclusionTileset(int tile_size, int block_size) {
            return Params(
                GeneratedTextureType::OCCLUSION, glm::ivec2(tile_size, block_size), glm::ivec2(0), glm::ivec2(0), 
                0, glm::ivec2(0), 1.f, 0, setupColors()
            );
        }

        int Params::Hash() const {
            int h = 0;

            h = type*31;
            h = h*31 + size.x;
            h = h*31 + size.y;
            h = h*31 + borderSize.x;
            h = h*31 + borderSize.y;
            h = h*31 + shadingSize.x;
            h = h*31 + shadingSize.y;
            h = h*31 + channel;
            h = h*31 + w.x;
            h = h*31 + w.y;
            h = h*31 + int(ratio*100);
            h = h*31 + flags;
            
            for(auto& c : colors) {
                h = h*31 + c.r;
                h = h*31 + c.g;
                h = h*31 + c.b;
                h = h*31 + c.a;
            }
            
            return h;
        }

        //==============================

        size_t Count() {
            return cache.cache.size();
        }

        int InvokationCount() {
            return cache.invokation_count;
        }

        void Clear() {
            cache.cache.clear();
        }

        TextureRef GetTexture(const Params& params) {
            cache.invokation_count++;

            int hash = params.Hash();
            if(cache.Contains(hash))
                return cache.GetTexture(hash);

            switch(params.type) {
                case GeneratedTextureType::BUTTON1:
                    return cache.Add(hash, ButtonTexture_Clear1(params));
                case GeneratedTextureType::BUTTON2:
                    return cache.Add(hash, ButtonTexture_Clear2(params));
                case GeneratedTextureType::BUTTON3:
                    return cache.Add(hash, ButtonTexture_Clear3(params));
                case GeneratedTextureType::BUTTON_2BORDERS:
                    return cache.Add(hash, ButtonTexture_Clear_2borders(params));
                case GeneratedTextureType::BUTTON_3BORDERS:
                    return cache.Add(hash, ButtonTexture_Clear_3borders(params));
                case GeneratedTextureType::BUTTON_TRIANGLE:
                    return cache.Add(hash, ButtonTexture_Triangle(params));
                case GeneratedTextureType::BUTTON_GEM1:
                    return cache.Add(hash, ButtonTexture_Gem(params));
                case GeneratedTextureType::BUTTON_GEM2:
                    return cache.Add(hash, ButtonTexture_Gem2(params));
                case GeneratedTextureType::BUTTON_HIGHLIGHT:
                    return cache.Add(hash, ButtonHighlightTexture(params)); 
                case GeneratedTextureType::SHADOWS:
                    return cache.Add(hash, ShadowsTexture(params));
                case GeneratedTextureType::OCCLUSION:
                    return cache.Add(hash, OcclusionTileset(params)); 
                default:
                    ENG_LOG_ERROR("TextureGenerator::GetTexture - invalid texture type ({})", params.type);
                    throw std::runtime_error("");
            }
        }

        bool TextureCache::Contains(int hash) {
            return cache.count(hash);
        }

        TextureRef TextureCache::GetTexture(int hash) {
            return cache.at(hash);
        }

        TextureRef TextureCache::Add(int hash, const TextureRef& texture) {
            if(cache.count(hash))
                ENG_LOG_WARN("TextureGenerator::Add - overriding an existing texture reference.");
            cache.insert({ hash, texture });
            return texture;
        }

        TextureRef ButtonTexture_Clear1(const Params& params) {
            return ButtonTexture_Clear(params.size.x, params.size.y, params.borderSize.x, params.shadingSize.x, params.channel, params.flags & GeneratedTextureFlags::FLIP_SHADING);
        }

        TextureRef ButtonTexture_Clear2(const Params& params) {
            return ButtonTexture_Clear(params.size.x, params.size.y, params.borderSize.x, params.shadingSize.x, (rgb)params.colors[0], (rgb)params.colors[1], (rgb)params.colors[2], (rgb)params.colors[3]);
        }

        TextureRef ButtonTexture_Clear3(const Params& params) {
            return ButtonTexture_Clear(params.size.x, params.size.y, params.borderSize.x, params.shadingSize.x, params.colors[0], params.colors[1], params.colors[2], params.colors[3]);
        }

        TextureRef ButtonTexture_Clear_2borders(const Params& params) {
            return ButtonTexture_Clear_2borders(params.size.x, params.size.y, params.borderSize.x, params.shadingSize.x, params.colors[0], params.colors[1], params.colors[2], params.colors[3], params.colors[4], params.w[0]);
        }

        TextureRef ButtonTexture_Clear_3borders(const Params& params) {
            return ButtonTexture_Clear_3borders(params.size.x, params.size.y, params.borderSize.x, params.shadingSize.x, params.colors[0], params.colors[1], params.colors[2], params.colors[3], params.colors[4], params.w[0], params.colors[5], params.w[1]);
        }

        TextureRef ButtonTexture_Triangle(const Params& params) {
            return ButtonTexture_Triangle(params.size.x, params.size.y, params.borderSize.x, params.flags & GeneratedTextureFlags::FLIP_SHADING, params.flags & GeneratedTextureFlags::UP);
        }

        TextureRef ButtonTexture_Gem(const Params& params) {
            return ButtonTexture_Gem(params.size.x, params.size.y, params.borderSize.x, params.ratio, params.colors[0]);
        }

        TextureRef ButtonTexture_Gem2(const Params& params) {
            return ButtonTexture_Gem2(params.size.x, params.size.y, params.borderSize.x, params.ratio, params.colors[0], params.colors[1]);
        }
        
        TextureRef ButtonHighlightTexture(const Params& params) {
            return ButtonHighlightTexture(params.size.x, params.size.y, params.borderSize.x, params.colors[0]);
        }

        TextureRef ShadowsTexture(const Params& params) {
            return ShadowsTexture(params.size.x, params.size.y, params.channel);
        }

        TextureRef OcclusionTileset(const Params& params) {
            return OcclusionTileset(params.size.x, params.size.y);
        }

        //==============================

        void basicButton(rgb* data, int width, int height, int channel, int bw, int sw, bool flipShading);
        void basicButton2(rgb* data, int width, int height, int bw, int sw, const rgb& fill, const rgb& border, const rgb& light, const rgb& shadow);
        void basicButton2(rgba* data, int width, int height, int bw, int sw, const rgba& fill, const rgba& border, const rgba& light, const rgba& shadow);
        void basicButton3(rgb* data, int width, int height, int bw, int sw, const rgb& fill, const rgb& border, const rgb& light, const rgb& shadow, const rgb& b2, int b2w);
        void basicButton3(rgb* data, int width, int height, int bw, int sw, const rgb& fill, const rgb& border, const rgb& light, const rgb& shadow, const rgb& b2, int b2w, const rgb& b3, int b3w);
        void triangle(rgb* data, int width, int height, int left, int right, int top, int bottom, bool flipShading, bool up);
        template<typename T> void gem(T* data, int width, int height, int offset, int borderWidth, int goldWidth, float ratio, const T centerFillColor);
        template<typename T> void gem2(T* data, int width, int height, int offset, int borderWidth, int goldWidth, float ratio, const T clr1, const T clr2);

        template<typename T> void fillColor(T* data, int width, int height, const T& color);

        float ellipseEQ(float x, float y, float a, float b);
        bool withinCircle(float x, float y, float r_sq);
        glm::vec2 rotate(const glm::vec2& v, float deg);

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

        TextureRef ButtonTexture_Clear(int width, int height, int bw, int sw, const rgb& fill, const rgb& border, const rgb& light, const rgb& shadow) {
            rgb* data = new rgb[width * height];

            basicButton2(data, width, height, bw, sw, fill, border, light, shadow);

            //--------------------------

            char buf[256];
            snprintf(buf, sizeof(buf), "btnTex_clear2_0x%02X%02X%02X", fill.r, fill.g, fill.b);

            TextureRef texture = std::make_shared<Texture>(
                TextureParams::CustomData(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE),
                (void*)data,
                std::string(buf)
            );
            delete[] data;
            return texture;
        }

        TextureRef ButtonTexture_Clear(int width, int height, int bw, int sw, const rgba& fill, const rgba& border, const rgba& light, const rgba& shadow) {
            rgba* data = new rgba[width * height];

            basicButton2(data, width, height, bw, sw, fill, border, light, shadow);

            //--------------------------

            char buf[256];
            snprintf(buf, sizeof(buf), "btnTex_clear2_0x%02X%02X%02X", fill.r, fill.g, fill.b);

            TextureRef texture = std::make_shared<Texture>(
                TextureParams::CustomData(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE),
                (void*)data,
                std::string(buf)
            );
            delete[] data;
            return texture;
        }

        TextureRef ButtonTexture_Clear_2borders(int width, int height, int bw, int sw, const rgb& fill, const rgb& border, const rgb& light, const rgb& shadow, const rgb& b2, int b2w) {
            rgb* data = new rgb[width * height];

            basicButton3(data, width, height, bw, sw, fill, border, light, shadow, b2, b2w);

            //--------------------------

            char buf[256];
            snprintf(buf, sizeof(buf), "btnTex_clear2_0x%02X%02X%02X", fill.r, fill.g, fill.b);

            TextureRef texture = std::make_shared<Texture>(
                TextureParams::CustomData(width, height, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE),
                (void*)data,
                std::string(buf)
            );
            delete[] data;
            return texture;
        }

        TextureRef ButtonTexture_Clear_3borders(int width, int height, int bw, int sw, const rgb& fill, const rgb& border, const rgb& light, const rgb& shadow, const rgb& b2, int b2w, const rgb& b3, int b3w) {
            rgb* data = new rgb[width * height];

            basicButton3(data, width, height, bw, sw, fill, border, light, shadow, b2, b2w, b3, b3w);

            //--------------------------

            char buf[256];
            snprintf(buf, sizeof(buf), "btnTex_clear2_0x%02X%02X%02X", fill.r, fill.g, fill.b);

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

        TextureRef ButtonTexture_Gem(int width, int height, int borderWidth, float ratio, const rgba& clr) {
            rgba* data = new rgba[width * height];

            fillColor<rgba>(data, width, height, rgba(0));
            // basicButton(data, width, height, 0, borderWidth, flipShading, flipShading);
            gem<rgba>(data, width, height, borderWidth, borderWidth*2, borderWidth*2, ratio, clr);

            //--------------------------

            TextureRef texture = std::make_shared<Texture>(
                TextureParams::CustomData(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE),
                (void*)data,
                std::string("btnTex_gem")
            );

            delete[] data;
            return texture;
        }

        TextureRef ButtonTexture_Gem2(int width, int height, int borderWidth, float ratio, const rgba& clr1, const rgba& clr2) {
            rgba* data = new rgba[width * height];

            fillColor<rgba>(data, width, height, rgba(0));
            // basicButton(data, width, height, 0, borderWidth, flipShading, flipShading);
            gem2<rgba>(data, width, height, borderWidth, borderWidth*2, borderWidth*2, ratio, clr1, clr2);

            //--------------------------

            TextureRef texture = std::make_shared<Texture>(
                TextureParams::CustomData(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE),
                (void*)data,
                std::string("btnTex_gem")
            );

            delete[] data;
            return texture;
        }

        TextureRef ButtonHighlightTexture(int width, int height, int bw) {
            return ButtonHighlightTexture(width, height, bw, rgba(255, 255, 0, 255));
        }

        TextureRef ButtonHighlightTexture(int width, int height, int bw, const rgba& highlight) {
            rgba* data = new rgba[width * height];

            rgba fill = rgba(255, 255, 255, 0);

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

        TextureRef ShadowsTexture(int width, int height, int size) {
            rgba* data = new rgba[width * height];
            
            rgba clrs[2] = {
                rgba(255,255,255,255),
                rgba(0,0,0,0)
            };

            //--------------------------
            for(int y = 0; y < height; y++) {
                int py = y / size;
                for(int x = 0; x < width; x++) {
                    int px = x / size;
                    data[y * width + x] = clrs[int((px+py) % 2 == 0)];
                }
            }


            //--------------------------

            char buf[256];
            snprintf(buf, sizeof(buf), "shadowsTex_%dx%d", width, height);

            TextureRef texture = std::make_shared<Texture>(
                TextureParams::CustomData(width, height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE),
                (void*)data,
                std::string(buf)
            );

            delete[] data;
            return texture;
        }

        struct OcclusionVisuals {
            struct Circle {
                bool enabled;
                glm::vec2 c;
                float r_sq;
            public:
                Circle() : enabled(false), c(glm::vec2(0.f)), r_sq(0.f) {}
                Circle(const glm::vec2& c_, float r) : c(c_), r_sq(r * r), enabled(true) {}
            };
        public:
            bool invert;
            std::array<Circle, 2> circles;
        };

        TextureRef OcclusionTileset(int tile_size, int block_size) {
            //16 = possible tile corner combinations; 2 = occlusion & fog of war textures
            int rows = 2;
            int cols = 16;
            int total_width = tile_size * cols;
            int total_height = tile_size * rows;

            int ts = tile_size;

            rgba* data = new rgba[total_width * total_height];

            rgba clrs[2][2] = {
                { rgba(0,0,0,0), rgba(255,255,255,255), },
                { rgba(255,255,255,128), rgba(255,255,255,255), },
            };

            float r = ts*2.f;
            float f = 0.5f;
            float v = std::sqrt(r*r - (ts*f*0.5f)*(ts*f*0.5f));
            std::array<OcclusionVisuals, 16> descriptions = {
                OcclusionVisuals{ true,     { OcclusionVisuals::Circle(), OcclusionVisuals::Circle() } },  //full
                OcclusionVisuals{ true,     { OcclusionVisuals::Circle(glm::vec2(0.f, ts), ts*f), OcclusionVisuals::Circle() } },    //~a
                OcclusionVisuals{ true,     { OcclusionVisuals::Circle(glm::vec2(ts, ts), ts*f), OcclusionVisuals::Circle() } },    //~b
                OcclusionVisuals{ true,     { OcclusionVisuals::Circle(glm::vec2(ts*0.5f, v+ts*f), r), OcclusionVisuals::Circle() } },    //c-d
                OcclusionVisuals{ true,     { OcclusionVisuals::Circle(glm::vec2(0.f, 0.f), ts*f), OcclusionVisuals::Circle() } },    //~c
                OcclusionVisuals{ true,     { OcclusionVisuals::Circle(glm::vec2(-v+ts*f, ts*0.5f), r), OcclusionVisuals::Circle() } },    //b-d
                OcclusionVisuals{ false,    { OcclusionVisuals::Circle(glm::vec2(ts,  0.f), ts*f), OcclusionVisuals::Circle(glm::vec2(0.f, ts), ts*f) } },    //a-d
                OcclusionVisuals{ false,    { OcclusionVisuals::Circle(glm::vec2(ts,  0.f), ts*f), OcclusionVisuals::Circle() } },    //d
                OcclusionVisuals{ true,     { OcclusionVisuals::Circle(glm::vec2(ts,  0.f), ts*f), OcclusionVisuals::Circle() } },    //~d
                OcclusionVisuals{ false,    { OcclusionVisuals::Circle(glm::vec2(0.f, 0.f), ts*f), OcclusionVisuals::Circle(glm::vec2(ts, ts), ts*f) } },    //b-c
                OcclusionVisuals{ true,     { OcclusionVisuals::Circle(glm::vec2(v+ts*f, ts*0.5f), r), OcclusionVisuals::Circle() } },    //a-c
                OcclusionVisuals{ false,    { OcclusionVisuals::Circle(glm::vec2(0.f, 0.f), ts*f), OcclusionVisuals::Circle() } },    //c
                OcclusionVisuals{ true,     { OcclusionVisuals::Circle(glm::vec2(ts*0.5f,-v+ts*f), r), OcclusionVisuals::Circle() } },    //a-b
                OcclusionVisuals{ false,    { OcclusionVisuals::Circle(glm::vec2(ts, ts), ts*f), OcclusionVisuals::Circle() } },    //b
                OcclusionVisuals{ false,    { OcclusionVisuals::Circle(glm::vec2(0.f, ts), ts*f), OcclusionVisuals::Circle() } },    //a
                OcclusionVisuals{ false,    { OcclusionVisuals::Circle(), OcclusionVisuals::Circle() } },  //empty
            };

            //--------------------------
            for(int r = 0; r < rows; r++) {
                for(int y = 0; y < tile_size; y++) {
                    int py = y / block_size;

                    for(int c = 0; c < cols; c++) {
                        for(int x = 0; x < tile_size; x++) {
                            int px = x / block_size;

                            OcclusionVisuals::Circle& c1 = descriptions[c].circles[0];
                            OcclusionVisuals::Circle& c2 = descriptions[c].circles[1];
                            bool invert = descriptions[c].invert;

                            bool occluded = (c1.enabled && withinCircle(c1.c.x-x, c1.c.y-y, c1.r_sq)) || (c2.enabled && withinCircle(c2.c.x-x, c2.c.y-y, c2.r_sq));
                            occluded = occluded ^ invert;

                            int n = (c == 15) ? 0 : r;
                            data[(r*tile_size+y)*total_width + c*tile_size+x] = clrs[n][int(occluded && (r == 0 || (py+px) % 2 == 0))];
                        }
                    }
                }
            }
            //--------------------------

            char buf[256];
            snprintf(buf, sizeof(buf), "occlusionsTex_%dx%d", tile_size * 16, tile_size * 2);
            TextureRef texture = std::make_shared<Texture>(
                TextureParams::CustomData(total_width, total_height, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE),
                (void*)data,
                std::string(buf)
            );

            delete[] data;
            return texture;
        }

        //===============================

        template<typename T> inline void fillColor(T* data, int width, int height, const T& color) {
            for(int y = 0; y < height; y++) {
                for(int x = 0; x < width; x++) {
                    data[y * width + x] = color;
                }
            }
        }

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

        void basicButton2(rgb* data, int width, int height, int bw, int sw, const rgb& fill, const rgb& border, const rgb& light, const rgb& shadow) {

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

        void basicButton2(rgba* data, int width, int height, int bw, int sw, const rgba& fill, const rgba& border, const rgba& light, const rgba& shadow) {

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

        void basicButton3(rgb* data, int width, int height, int bw, int sw, const rgb& fill, const rgb& border, const rgb& light, const rgb& shadow, const rgb& b2, int b2w) {

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


            //2nd border
            for(int y = 0; y < b2w; y++) {
                for(int x = 0; x < width; x++) {
                    data[y * width + x] = b2;
                    data[(height-1-y) * width + x] = b2;
                }
            }
            for(int y = b2w; y < height-b2w; y++) {
                for(int x = 0; x < b2w; x++) {
                    data[y * width + x] = b2;
                    data[y * width + width-1-x] = b2;
                }
            }
        }

        void basicButton3(rgb* data, int width, int height, int bw, int sw, const rgb& fill, const rgb& border, const rgb& light, const rgb& shadow, const rgb& b2, int b2w, const rgb& b3, int b3w) {

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


            //2nd border
            for(int y = 0; y < b2w; y++) {
                for(int x = 0; x < width; x++) {
                    data[y * width + x] = b2;
                    data[(height-1-y) * width + x] = b2;
                }
            }
            for(int y = b2w; y < height-b2w; y++) {
                for(int x = 0; x < b2w; x++) {
                    data[y * width + x] = b2;
                    data[y * width + width-1-x] = b2;
                }
            }

            //3rd border
            for(int y = 0; y < b3w; y++) {
                for(int x = 0; x < width; x++) {
                    data[y * width + x] = b3;
                    data[(height-1-y) * width + x] = b3;
                }
            }
            for(int y = b3w; y < height-b3w; y++) {
                for(int x = 0; x < b3w; x++) {
                    data[y * width + x] = b3;
                    data[y * width + width-1-x] = b3;
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
                bottom = tmp;
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

        template<typename T> 
        inline void gem(T* data, int width, int height, int offset, int bw, int gw, float ratio, const T centerFillColor) {
            //best variable initialization ever
            // T fillGem = T(255); fillGem[0] = 71; fillGem[1] = fillGem[2] = 0;
            T fillGem = centerFillColor;
            T fillGold = T(255); fillGold[0] = 210; fillGold[1] = 190; fillGold[2] = 28;
            T fillBorder = T(255); fillBorder[0] = 30; fillBorder[1] = fillBorder[2] = 0;
            //=====

            glm::vec2 rat = glm::vec2(1.f, ratio);

            glm::vec2 c = glm::vec2(width, height) * 0.5f * rat;
            glm::vec2 r1 = c - float(offset+bw+gw);
            glm::vec2 r2 = c - float(offset+bw);
            glm::vec2 r3 = c - float(offset);
            
            //circles rendering
            for(int y = 0; y < height; y++) {
                for(int x = 0; x < width; x++) {
                    T& pixel = data[y * width + x];
                    
                    glm::vec2 p = glm::vec2(x, y) * rat;
                    
                    //gem (inner circle)
                    if(ellipseEQ(p.x-c.x, p.y-c.y, r1.x, r1.y) < 1.f) {
                        pixel = fillGem;
                    }
                    //gold fittings
                    else if(ellipseEQ(p.x-c.x, p.y-c.y, r2.x, r2.y) < 1.f) {
                        pixel = fillGold;
                    }
                    //borders
                    else if(ellipseEQ(p.x-c.x, p.y-c.y, r3.x, r3.y) < 1.f) {
                        pixel = fillBorder;
                    }
                }
            }

            //shading
        }

        template<typename T> 
        inline void gem2(T* data, int width, int height, int offset, int bw, int gw, float ratio, const T clr1, const T clr2) {
            //best variable initialization ever
            // T fillGem = T(255); fillGem[0] = 71; fillGem[1] = fillGem[2] = 0;
            T fillGem = clr1;
            T fillGold = T(255); fillGold[0] = 210; fillGold[1] = 190; fillGold[2] = 28;
            T fillBorder = T(255); fillBorder[0] = 30; fillBorder[1] = fillBorder[2] = 0;
            //=====

            glm::vec2 rat = glm::vec2(1.f, ratio);

            glm::vec2 c = glm::vec2(width, height) * 0.5f * rat;
            glm::vec2 r1 = c - float(offset+bw+gw);
            glm::vec2 r2 = c - float(offset+bw);
            glm::vec2 r3 = c - float(offset);
            glm::vec2 r4 = c * 1.25f;
            glm::vec2 r5 = c * 1.5f;

            glm::vec2 p1 = c * 0.4f;
            glm::vec2 p2 = glm::vec2(0);
            glm::vec2 p3 = c * 0.7f;
            glm::vec2 p4 = c + rotate(c*0.5f, -20.f);

            //circles rendering
            for(int y = 0; y < height; y++) {
                for(int x = 0; x < width; x++) {
                    T& pixel = data[y * width + x];
                    
                    glm::vec2 p = glm::vec2(x, y) * rat;
                    
                    //gem (inner circle)
                    if(ellipseEQ(p.x-c.x, p.y-c.y, r1.x, r1.y) < 1.f) {
                        pixel = fillGem;
                        if(ellipseEQ(p.x-p2.x, p.y-p2.y, r5.x, r5.y) > 1.f) {
                            pixel = rgba(110, 0, 0, 255);
                        }
                        if(ellipseEQ(p.x-p1.x, p.y-p1.y, r4.x, r4.y) > 1.f) {
                            pixel = clr2;
                        }
                    }
                    //gold fittings
                    else if(ellipseEQ(p.x-c.x, p.y-c.y, r2.x, r2.y) < 1.f) {
                        pixel = fillGold;
                    }
                    //borders
                    else if(ellipseEQ(p.x-c.x, p.y-c.y, r3.x, r3.y) < 1.f) {
                        pixel = fillBorder;
                        
                    }

                    glm::vec2 v = rotate(p - p3, 45.f);
                    if(ellipseEQ(v.x, v.y, 16.f, 4.f) < 1.f)
                        pixel = rgba(220,220,220,255);
                    
                    v = rotate(p - p4, 60.f);
                    if(ellipseEQ(v.x, v.y, 32.f, 2.f) < 1.f)
                        pixel = rgba(150,150,150,255);
                }
            }
        }

        float ellipseEQ(float x, float y, float a, float b) {
            return (x*x)/(a*a) + (y*y)/(b*b);
        }

        bool withinCircle(float x, float y, float r_sq) {
            return x*x + y*y < r_sq;
        }

        glm::vec2 rotate(const glm::vec2& v, float deg) {
            float rad = deg * glm::pi<float>() / 180.f;
            float c = std::cos(rad);
            float s = std::sin(rad);
            return glm::vec2(v.x*c - v.y*s, v.x*s + v.y*c);
        }

    }//namespace TextureGenerator

}//namespace eng
