#include "engine/core/sprite.h"

#include "engine/utils/log.h"
#include "engine/utils/utils.h"
#include "engine/utils/dbg_gui.h"
#include "engine/utils/json.h"

#include "engine/core/quad.h"
#include "engine/core/renderer.h"

namespace eng {

    SpritesheetData ParseConfig_Spritesheet(const std::string& config_filepath, int flags);
    SpriteData ParseConfig_Sprite(const nlohmann::json& config);

    //===== Sprite =====

    Sprite::Sprite(const TextureRef& texture_, const SpriteData& data_) : texture(texture_), data(data_) {
        RecomputeTexCoords();
    }

    void Sprite::Render(const glm::uvec4& info, const glm::vec3& screen_pos, const glm::vec2& screen_size, int idxY, int idxX) const {
        ENG_LOG_TRACE("[{},{}] - {}, {}, {}", idxY, idxX, data.size, data.frames.offset, data.offset);
        glm::vec2 texOffset = glm::vec2((data.size.x + data.frames.offset.x) * (idxX % data.frames.line_length), (data.size.y + data.frames.offset.y) * (idxY % data.frames.line_count));
        Quad quad = Quad::FromCorner(info, screen_pos, screen_size, glm::vec4(1.f), texture, TexOffset(texOffset));
        Renderer::RenderQuad(quad);
    }

    void Sprite::Render(const glm::vec3& screen_pos, const glm::vec2& screen_size, int idxY, int idxX, float paletteIdx) const {
        glm::vec2 texOffset = glm::vec2((data.size.x + data.frames.offset.x) * (idxX % data.frames.line_length), (data.size.y + data.frames.offset.y) * (idxY % data.frames.line_count));
        Quad quad = Quad::FromCorner(screen_pos, screen_size, glm::vec4(1.f), texture, TexOffset(texOffset)).SetPaletteIdx(paletteIdx);
        Renderer::RenderQuad(quad);
    }

    void Sprite::RenderAlt(const glm::uvec4& info, const glm::vec4& color, bool noTexture, const glm::vec3& screen_pos, const glm::vec2& screen_size, int idxY, int idxX) const {
        glm::vec2 texOffset = glm::vec2((data.size.x + data.frames.offset.x) * (idxX % data.frames.line_length), (data.size.y + data.frames.offset.y) * (idxY % data.frames.line_count));
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

    TexCoords Sprite::TexOffset(const glm::ivec2& added_offset) const {
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

    Spritesheet::Spritesheet(const std::string& config_filepath, int flags) : data(ParseConfig_Spritesheet(config_filepath, flags)) {
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
        data.firstFrame = sprite.FirstFrame();
        data.frameCount = sprite.FrameCount();
    }
    
    SpriteGroup::SpriteGroup(const SpriteGroupData& data_) : data(data_) {
        ENG_LOG_TRACE("[C] SpriteGraphics '{}' ({})", data.name, (int)data.sprites.size());
    }

    void SpriteGroup::Render(const glm::vec3& screen_pos, const glm::vec2& screen_size, const glm::uvec4& info, int idxY, int idxX) {
        size_t i = 0;
        glm::uvec3 inf = glm::uvec3(info);
        for (Sprite& sprite : data.sprites) {
            sprite.Render(glm::uvec4(inf, i++), screen_pos, screen_size, idxY, idxX);
        }
    }

    void SpriteGroup::RenderAlt(const glm::vec3& screen_pos, const glm::vec2& screen_size, const glm::uvec4& info, const glm::vec4& color, bool noTexture, int idxY, int idxX) {
        size_t i = 0;
        glm::uvec3 inf = glm::uvec3(info);
        for (Sprite& sprite : data.sprites) {
            sprite.RenderAlt(glm::uvec4(inf, i++), color, noTexture, screen_pos, screen_size, idxY, idxX);
        }
    }

    //====================================================================

    SpritesheetData ParseConfig_Spritesheet(const std::string& config_filepath, int flags) {
        using json = nlohmann::json;

        SpritesheetData data = {};

        //load the config and parse it as json file
        json config = json::parse(ReadFile(config_filepath.c_str()));

        try {
            //get spritesheet texture path & load the texture
            std::string texture_filepath = config.at("texture_filepath");
            data.texture = std::make_shared<Texture>(texture_filepath, flags);

            //iterate & parse 'sprites' array
            auto& sprites_json = config.at("sprites");
            for(auto& sprite_json : sprites_json) {
                SpriteData spriteData = ParseConfig_Sprite(sprite_json);
                data.sprites.insert({ spriteData.name, Sprite(data.texture, spriteData) });
            }
        }
        catch(json::exception& e) {
            ENG_LOG_WARN("Failed to parse '{}' config file - {}", config_filepath.c_str(), e.what());
            throw e;
        }

        //extract spritesheet name (from configfile name)
        data.name = GetFilename(config_filepath, true);

        return data;
    }

    SpriteData ParseConfig_Sprite(const nlohmann::json& config) {
        using json = nlohmann::json;

        SpriteData data = {};

        //parse name, size & offset fields
        data.name = config.at("name");
        data.size = eng::json::parse_ivec2(config.at("size"));
        data.offset = eng::json::parse_ivec2(config.at("offset"));

        //parse the array of control points ('pts')
        if(config.count("pts")) {
            auto& pts_json = config.at("pts");
            for(auto& pt_json : pts_json) {
                glm::ivec2 pt = eng::json::parse_ivec2(pt_json);
                pt.y = data.size.y - pt.y;
                data.pts.push_back(pt);
            }
        }
        else {
            //auto-generate 2 control points
            data.pts.push_back(glm::ivec2(0, 0));
            data.pts.push_back(glm::ivec2(data.size.x, data.size.y));
        }

        //parse frames data
        if(config.count("frames")) {
            auto& frames_json = config.at("frames");

            if(frames_json.count("line_length")) data.frames.line_length   = frames_json.at("line_length");
            if(frames_json.count("line_count"))  data.frames.line_count    = frames_json.at("line_count");
            if(frames_json.count("start_frame")) data.frames.start_frame   = frames_json.at("start_frame");
            if(config.count("offset")) {
                if(config.at("offset").size() > 1) data.frames.offset = eng::json::parse_ivec2(frames_json.at("offset"));
                else                               data.frames.offset = glm::ivec2(frames_json.at("offset"));
            }
        }

        return data;
    }

}//namespace eng
