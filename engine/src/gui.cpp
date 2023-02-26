#include "engine/core/gui.h"

#include "engine/core/renderer.h"
#include "engine/core/quad.h"

namespace eng::GUI {

#define Z_INDEX_BASE -0.9f
#define Z_INDEX_MULT 1e-3f
#define Z_TEXT_OFFSET 1e-4f

    //===== Element =====
    
    Element::Element(Element* parent_) : parent(parent_) {}
    
    Element::Element(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const TextureRef& texture_, const glm::vec4& color_, Element* parent_)
        : parent(parent_), texture(texture_), color(color_), offset(offset_), size(size_), zOffset(zOffset_) {}

    void Element::Render() const {
        InnerRenderHierarchy(glm::vec2(0.f), glm::vec2(1.f), 0.f);
    }

    void Element::AddChild(Element* child) {
        children.push_back(child);
    }

    void Element::InnerRenderHierarchy(const glm::vec2& parentPos, const glm::vec2& parentSize, float parentZIndex) const {
        glm::vec2 realPos = parentPos + offset * parentSize;
        glm::vec2 realSize = parentSize * size;
        float zIdx = parentZIndex + zOffset;

        //render this element
        InnerRender(realPos, realSize, zIdx);

        //render all the children
        for(Element* element : children) {
            element->InnerRenderHierarchy(realPos, realSize, zIdx);
        }
    }

    void Element::InnerRender(const glm::vec2& pos, const glm::vec2& size, float zIdx) const {
        // ENG_LOG_INFO("pos = {}, size = {}, zIdx = {}", to_string(pos), to_string(size), zIdx);
        Renderer::RenderQuad(Quad::FromCenter(glm::vec3(pos, Z_INDEX_BASE - zIdx * Z_INDEX_MULT), size, color, texture));
    }

    //===== Button =====

    Button::Button(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const TextureRef& texture_, 
                   const glm::vec4& color_, const FontRef& font_, const std::string& text_, const glm::vec4& textColor_)
        : Element(offset_, size_, zOffset_, texture_, color_), text(text_), textColor(textColor_), font(font_) {}

    void Button::InnerRender(const glm::vec2& pos, const glm::vec2& size, float zIdx) const {
        Element::InnerRender(pos, size, zIdx);
        font->RenderTextCentered(text.c_str(), pos, 1.f, textColor, Z_INDEX_BASE - zIdx * Z_INDEX_MULT - Z_TEXT_OFFSET);
    }

    //===== Menu =====

    Menu::Menu(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const std::vector<Button>& buttons_,
               const TextureRef& texture_, const glm::vec4& color_)
        : Element(offset_, size_, zOffset_, texture_, color_), buttons(buttons_) {
        for(Button& b : buttons) {
            b.LinkParent(this);
            AddChild(&b);
        }
    }

}//namespace eng::GUI
