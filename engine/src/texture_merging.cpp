#include "engine/core/texture.h"
#include "engine/core/renderer.h"
#include "engine/utils/timer.h"

#include <rectpack2D/rectpack2D.h>

namespace eng {

    namespace r2d = rectpack2D;

    constexpr bool allow_flip = false;
    constexpr auto runtime_flipping_mode = r2d::flipping_option::DISABLED;
    constexpr auto discard_step = 1;

    using spaces_type = r2d::empty_spaces<allow_flip, r2d::default_empty_spaces>;
    using rect_type = r2d::output_rect_t<spaces_type>;

    r2d::rect_wh BinPacking(std::vector<rect_type>& rectangles) {
        int max_side = Renderer::MaxTextureSize();

        auto report_successful = [](rect_type&) { return r2d::callback_result::CONTINUE_PACKING; };
        auto report_unsuccessful = [](rect_type&) { return r2d::callback_result::ABORT_PACKING; };

        auto report_result = [&rectangles](const r2d::rect_wh& result_size) {
            // ENG_LOG_FINER("Resultant bin: {} {}", result_size.w, result_size.h);
            for (const auto& r : rectangles) {
                // ENG_LOG_FINER("{} {} {} {}", r.x, r.y, r.w, r.h);
            }
        };

        const auto result_size = r2d::find_best_packing<spaces_type>(
			rectangles,
			r2d::make_finder_input(
				max_side,
				discard_step,
				report_successful,
				report_unsuccessful,
				runtime_flipping_mode
			)
		);

		report_result(result_size);
        return result_size;
    }

    void Texture::MergeTextures(std::vector<TextureRef>& textures, GLenum filteringMode, bool rgba) {
        Timer t1, t2;

        //TODO: case where the algorithm fails is not handled (should split into more than 1 texture)

        t1.Reset();
        t2.Reset();

        std::vector<rect_type> rectangles = {};
        for(TextureRef& tex : textures) {
            rectangles.push_back(r2d::rect_xywh(0, 0, tex->Width(), tex->Height()));
        }
        auto time_packing = t2.TimeElapsed() * 1e-3f;

        //find an optimal layout, taking into account all the textures (rectangle packing problem)
        auto size = BinPacking(rectangles);

        //create new texture (temporary, only the handle will survive)
        GLenum internalFormat = rgba ? GL_RGBA : GL_RGB;
        TextureRef tex = std::make_shared<Texture>(TextureParams::EmptyTexture(size.w, size.h, internalFormat, filteringMode), "merge_texture");
        glm::vec2 tex_size = glm::vec2(tex->Size());

        t2.Reset();
        //copy over all the textures to their new respective location
        for(size_t i = 0; i < rectangles.size(); i++) {
            glm::ivec2 offset = glm::ivec2(rectangles[i].x, rectangles[i].y);
            textures[i]->Merge_CopyTo(tex, offset);
            textures[i]->Merge_UpdateData(tex->handle_data, offset, tex_size);
        }
        auto time_merging = t2.TimeElapsed() * 1e-3f;
        auto time_total = t1.TimeElapsed() * 1e-3f;

        ENG_LOG_TRACE("Texture::MergeTextures - merged {} textures into {} ({}x{}).", textures.size(), 1, tex_size.x, tex_size.y);
        ENG_LOG_TRACE("Texture::MergeTextures - {}ms (packing: {}ms, merging: {}ms)", time_total, time_packing, time_merging);
    }

}//namespace eng
