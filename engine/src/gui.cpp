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

    AABB Element::GetAABB() {
        //need to convert, bcs offset is relative to screen center & anchors the element's center
        glm::vec2 m = offset + 0.5f - size * 0.5f;      
        glm::vec2 M = offset + 0.5f + size * 0.5f;
        return AABB{ m, M };
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
        Renderer::RenderQuad(Quad::FromCenter(glm::vec3(pos.x, -pos.y, Z_INDEX_BASE - zIdx * Z_INDEX_MULT), size, color, texture));
    }

    //===== Button =====

    Button::Button(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const TextureRef& texture_, 
                   const glm::vec4& color_, const FontRef& font_, const std::string& text_, const glm::vec4& textColor_)
        : Element(offset_, size_, zOffset_, texture_, color_), text(text_), textColor(textColor_), font(font_) {}

    void Button::InnerRender(const glm::vec2& pos, const glm::vec2& size, float zIdx) const {
        Element::InnerRender(pos, size, zIdx);
        font->RenderTextCentered(text.c_str(), glm::vec2(pos.x, -pos.y), 1.f, textColor, Z_INDEX_BASE - zIdx * Z_INDEX_MULT - Z_TEXT_OFFSET);
    }

    void Button::OnHover() {
        ENG_LOG_FINE("Button HOVER - '{}'", text);
    }

    void Button::OnClick() {
        ENG_LOG_FINE("Button CLICK - '{}'", text);
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

    ScreenObject* Menu::FetchLeaf(const glm::vec2& mousePos) {
        //warning - this wont work if menu object has a parent node
        AABB bb = GetAABB();
        glm::vec2 pos = (mousePos - bb.min) / (bb.max - bb.min);

        //ingoring overlaps, returns the first detected
        // ENG_LOG_TRACE("========MENU LOOKUP======");
        // ENG_LOG_TRACE("pos = {}", to_string(pos));
        for(Button& btn : buttons) {
            bb = AABB{
                btn.offset * size + 0.5f - btn.size * 0.5f,
                btn.offset * size + 0.5f + btn.size * 0.5f
            };

            // ENG_LOG_TRACE("BTN: off = {}, size = {}; m = {}, M = {}", to_string(btn.offset), to_string(btn.size), to_string(bb.min), to_string(bb.max));
            if(bb.IsInside(pos)) {
                return &btn;
            }
        }

        return this;
    }

}//namespace eng::GUI
