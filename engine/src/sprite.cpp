#include "engine/core/sprite.h"

#include "engine/utils/log.h"
#include "engine/utils/utils.h"
#include "engine/utils/dbg_gui.h"

#include "engine/core/quad.h"
#include "engine/core/renderer.h"

namespace eng {

    //===== Sprite =====

    Sprite::Sprite(const TextureRef& texture_, const SpriteData& data_) : texture(texture_), data(data_) {
        RecomputeTexCoords();
    }

    void Sprite::Render(const glm::uvec4& info, const glm::vec3& screen_pos, const glm::vec2& screen_size, int orientation, int frameIdx) {
        glm::vec2 texOffset = glm::vec2((data.size.x + data.frames.offset) * (frameIdx % data.frames.line_length), data.size.y * (orientation % data.frames.line_count));
        Quad quad = Quad::FromCorner(info, screen_pos, screen_size, glm::vec4(1.f), texture, TexOffset(texOffset));
        Renderer::RenderQuad(quad);
    }

    void Sprite::RenderAlt(const glm::uvec4& info, const glm::vec4& color, bool noTexture, const glm::vec3& screen_pos, const glm::vec2& screen_size, int orientation, int frameIdx) {
        glm::vec2 texOffset = glm::vec2((data.size.x + data.frames.offset) * (frameIdx % data.frames.line_length), data.size.y * (orientation % data.frames.line_count));
        Quad quad = Quad::FromCorner(info, screen_pos, screen_size, color, texture, TexOffset(texOffset));
        quad.vertices.SetAlphaFromTexture(noTexture);
        Renderer::RenderQuad(quad);
    }

    void Sprite::RecomputeTexCoords() {
        const glm::ivec2& of = data.offset;
        const glm::ivec2& sz = data.size;
        const glm::vec2  tsz = glm::vec2(texture->Size());

        texCoords[0] = glm::vec2(of + glm::ivec2(   0, sz.y)) / tsz;
        texCoords[1] = glm::vec2(of + glm::ivec2(   0,    0)) / tsz;
        texCoords[2] = glm::vec2(of + glm::ivec2(sz.x, sz.y)) / tsz;
        texCoords[3] = glm::vec2(of + glm::ivec2(sz.x,    0)) / tsz;
    }

    void Sprite::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        if(ImGui::CollapsingHeader(data.name.c_str())) {
            ImGui::PushID(this);

            if(ImGui::DragInt2("offset", (int*)&data.offset))
                RecomputeTexCoords();
            
            if(ImGui::DragInt2("size", (int*)&data.size))
                RecomputeTexCoords();

            if(ImGui::CollapsingHeader("pts")) {
                char buf[256];
                for(int i = 0; i < (int)data.pts.size(); i++) {
                    snprintf(buf, sizeof(buf), "pt[%d]", i);
                    ImGui::DragInt2(buf, (int*)&data.pts[i]);
                }
            }
            ImGui::PopID();
        }
#endif
    }

    TexCoords Sprite::TexOffset(const glm::ivec2& added_offset) {
        const glm::ivec2& of = data.offset + added_offset;
        const glm::ivec2& sz = data.size;
        const glm::vec2  tsz = glm::vec2(texture->Size());

        return TexCoords(
            glm::vec2(of + glm::ivec2(   0, sz.y)) / tsz,
            glm::vec2(of + glm::ivec2(   0,    0)) / tsz,
            glm::vec2(of + glm::ivec2(sz.x, sz.y)) / tsz,
            glm::vec2(of + glm::ivec2(sz.x,    0)) / tsz
        );
    }

    //===== Spritesheet =====

    Spritesheet::Spritesheet(const SpritesheetData& data_) : data(data_) {
        ENG_LOG_TRACE("[C] Spritesheet '{}' ({})", data.name, (int)data.sprites.size());
    }

    Spritesheet::~Spritesheet() {
        Release();
    }

    Spritesheet::Spritesheet(Spritesheet&& s) noexcept {
        Move(std::move(s));
    }

    Spritesheet& Spritesheet::operator=(Spritesheet&& s) noexcept {
        Release();
        Move(std::move(s));
        return *this;
    }

    Sprite Spritesheet::Get(const std::string& name) const {
        return data.sprites.at(name);
    }

    bool Spritesheet::TryGet(const std::string& name, Sprite& out_sprite) const {
        auto it = data.sprites.find(name);
        if(it != data.sprites.end()) {
            out_sprite = it->second;
            return true;
        }
        return false;
    }

    void Spritesheet::DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        if (ImGui::CollapsingHeader(data.name.c_str())) {
			ImGui::PushID(this);

            for(auto&[key, sprite] : data.sprites) {
                sprite.DBG_GUI();
            }

            ImGui::PopID();
        }
#endif
    }

    void Spritesheet::Release() noexcept {
        if(data.texture != nullptr) {
            ENG_LOG_TRACE("[D] Spritesheet '{}'", data.name);
            data.texture = nullptr;
            data.sprites.clear();
        }
    }

    void Spritesheet::Move(Spritesheet&& s) noexcept {
        data = std::move(s.data);
        s.data = {};
    }

    //===== SpriteGroup =====

    SpriteGroup::SpriteGroup(const Sprite& sprite) {
        data.sprites = { sprite };
    }
    
    SpriteGroup::SpriteGroup(const SpriteGroupData& data_) : data(data_) {
        ENG_LOG_TRACE("[C] SpriteGraphics '{}' ({})", data.name, (int)data.sprites.size());
    }

    void SpriteGroup::Render(const glm::vec3& screen_pos, const glm::vec2& screen_size, const glm::uvec4& info, int orientation, int frameIdx) {
        size_t i = 0;
        glm::uvec3 inf = glm::uvec3(info);
        for (Sprite& sprite : data.sprites) {
            sprite.Render(glm::uvec4(inf, i++), screen_pos, screen_size, orientation, frameIdx);
        }
    }

    void SpriteGroup::RenderAlt(const glm::vec3& screen_pos, const glm::vec2& screen_size, const glm::uvec4& info, const glm::vec4& color, bool noTexture, int orientation, int frameIdx) {
        size_t i = 0;
        glm::uvec3 inf = glm::uvec3(info);
        for (Sprite& sprite : data.sprites) {
            sprite.RenderAlt(glm::uvec4(inf, i++), color, noTexture, screen_pos, screen_size, orientation, frameIdx);
        }
    }

}//namespace eng
