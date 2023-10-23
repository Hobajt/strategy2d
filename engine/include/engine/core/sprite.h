#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>

#include "engine/utils/mathdefs.h"

#include "engine/core/texture.h"


namespace eng {
    
    //===== SpriteData =====

    //Sprite data container
    struct SpriteData {
        //Locates block of images in the texture atlas that the sprite works with.
        struct FrameBlock {
            int line_length = 1;
            int line_count = 1;
            int start_frame = 0;
            glm::ivec2 offset = glm::ivec2(0);
            bool enable_flip = true;
        };
    public:
        glm::ivec2 offset;
        glm::ivec2 size;
        FrameBlock frames;
        std::vector<glm::ivec2> pts;

        std::string name;
    };

    //===== Sprite =====

    //2D graphics object. Wrapper for a group of images in a texture atlas.
    class Sprite {
    public:
        Sprite() = default;
        Sprite(const TextureRef& texture, const SpriteData& data);

        //Render sprite at given screen coords. frameIdx and orientation are horizontal and vertical indices into the sprite's frames.
        void Render(const glm::uvec4& info, const glm::vec3& screen_pos, const glm::vec2& screen_size, int idxY = 0, int idxX = 0) const;
        void Render(const glm::vec3& screen_pos, const glm::vec2& screen_size, int idxY = 0, int idxX = 0, float paletteIdx = -1.f) const;
        void Render(const glm::vec3& screen_pos, const glm::vec2& screen_size, int idxY, int idxX, const glm::vec4& color, float paletteIdx = -1.f) const;

        void RenderAnim(const glm::vec3& screen_pos, const glm::vec2& screen_size, int frameIdx = 0, int spriteIdx = 0, float paletteIdx = -1.f) const;
        void RenderAnimAlt( const glm::vec4& color, bool noTexture, const glm::vec3& screen_pos, const glm::vec2& screen_size, int frameIdx = 0, int spriteIdx = 0, float paletteIdx = -1.f) const;

        //Alternate render call with additional options (modify color/use texture as alpha).
        void RenderAlt(const glm::uvec4& info, const glm::vec4& color, bool noTexture, const glm::vec3& screen_pos, const glm::vec2& screen_size, int idxY = 0, int idxX = 0) const;

        //Update texture coordinates. Call when spritesheet or sprite's offset/size changes.
        void RecomputeTexCoords();

        std::string Name() const { return data.name; }
        glm::ivec2 Size() const { return data.size; }
        glm::ivec2 Point(int i) const { return data.pts[i]; }
        size_t PointCount() const { return data.pts.size(); }

        //for sprite animation
        int FirstFrame() const { return data.frames.start_frame; }
        int FrameCount() const { return data.frames.line_length; }
        int AnimFrameCount() const { return data.pts.size(); }

        int LineLength() const { return data.frames.line_length; }
        int LineCount() const { return data.frames.line_count; }

        void DBG_GUI();
        TextureRef GetTexture() const { return texture; }
    private:
        //To compute texture coordinates of selected frame of the sprite.
        TexCoords TexOffset(const glm::ivec2& offset) const;
        TexCoords TexOffset(const glm::ivec2& offset, bool flip) const;
    private:
        SpriteData data;
        TextureRef texture = nullptr;
        TexCoords texCoords;
    };
    using SpriteRef = std::shared_ptr<Sprite>;

    //===== SpritesheetData =====

    //Spritesheet data container.
    struct SpritesheetData {
        TextureRef texture = nullptr;
        std::map<std::string, Sprite> sprites;
        std::string name;
    };

    //===== Spritesheet =====

    //Larger image containing multiple sprites, essentially a sprite database.
    class Spritesheet {
        using SpriteMap = std::map<std::string, Sprite>;
    public:
        //Flags parameter is for the texture loading.
        Spritesheet(const SpritesheetData& data);
        Spritesheet(const std::string& config_filepath, int flags = 0);

        Spritesheet() = default;
        ~Spritesheet();

        //copy disabled
        Spritesheet(const Spritesheet&) = delete;
        Spritesheet& operator=(const Spritesheet&) = delete;

        //move enabled
        Spritesheet(Spritesheet&&) noexcept;
        Spritesheet& operator=(Spritesheet&&) noexcept;

        //=== sprite fetch methods ===

        Sprite operator()(const std::string& name) const { return Get(name); }
        Sprite Get(const std::string& name) const;
        bool TryGet(const std::string& name, Sprite& out_sprite) const;

        //=== iterators ===

        SpriteMap::iterator begin() { return data.sprites.begin(); }
        SpriteMap::iterator end() { return data.sprites.end(); }
        SpriteMap::const_iterator begin() const { return data.sprites.begin(); }
        SpriteMap::const_iterator end() const { return data.sprites.end(); }

        size_t Size() const { return data.sprites.size(); }

        //=== other ===

        void DBG_GUI();
    private:
        void Release() noexcept;
        void Move(Spritesheet&&) noexcept;
    private:
        SpritesheetData data;
    };
    using SpritesheetRef = std::shared_ptr<Spritesheet>;

    //===== SpriteGroupData =====

    //SpriteGroup data container.
    struct SpriteGroupData {
        std::string name;
        int id;

        std::vector<Sprite> sprites;

        bool repeat = true;
        float duration = 1.f;

        int frameCount = 1;
        int firstFrame = 0;

        float keyframe = 1.f;
    public:
        SpriteGroupData() = default;
        SpriteGroupData(int id, const std::string& name, bool repeat, float duration, int frameCount, int firstFrame);
        SpriteGroupData(bool repeat, float duration, int frameCount, int firstFrame);
        SpriteGroupData(int id, const Sprite& sprite, bool repeat, float duration, float keyframe = 1.f);
    };

    //===== SpriteGroup =====

    //Groups together multiple sprites, to render them as one object.
    class SpriteGroup {
    public:
        SpriteGroup() = default;
        SpriteGroup(const Sprite& sprite, int id);
        SpriteGroup(const SpriteGroupData& data);

        void Render(const glm::vec3& screen_pos, const glm::vec2& screen_size, const glm::uvec4& info, int idxY = 0, int idxX = 0);
        void RenderAlt(const glm::vec3& screen_pos, const glm::vec2& screen_size, const glm::uvec4& info, const glm::vec4& color, bool noTexture, int idxY = 0, int idxX = 0);

        void RenderAnim(const glm::vec3& screen_pos, const glm::vec2& screen_size, const glm::uvec4& info, int frameIdx = 0, int spriteIdx = 0, float paletteIdx = -1);
        void RenderAnimAlt(const glm::vec3& screen_pos, const glm::vec2& screen_size, const glm::uvec4& info, const glm::vec4& color, bool noTexture, int frameIdx = 0, int spriteIdx = 0, float paletteIdx = -1);

        int ID() const { return data.id; }
        bool Repeat() const { return data.repeat; }
        float Duration() const { return data.duration; }
        int FrameCount() const { return data.frameCount; }
        int FirstFrame() const { return data.firstFrame; }

        int FrameIdx(float f) const { return int(data.frameCount * (f / data.duration)); }
        float FirstFrameF() const { return data.firstFrame / (float)data.frameCount;}

        float Keyframe() const { return data.keyframe; }
    private:
        SpriteGroupData data;
    };

}//namespace eng
