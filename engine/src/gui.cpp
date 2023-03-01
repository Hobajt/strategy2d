#include "engine/core/gui.h"

#include "engine/core/renderer.h"
#include "engine/core/quad.h"

#define Z_INDEX_BASE -0.9f
#define Z_INDEX_MULT 1e-3f
#define Z_TEXT_OFFSET 1e-4f

namespace eng::GUI {

    //===== Style =====

    StyleRef CreateDefaultStyle() {
        StyleRef s = std::make_shared<Style>();
        s->color = glm::vec4(glm::vec3(1.f), 0.f);
        return s;
    }

    StyleRef Style::Default() {
        static StyleRef defStyle = CreateDefaultStyle();
        return defStyle;
    }

    //===== Element =====

    Element::Element(Element* parent_, bool recalculate_) {
        SetParent(parent_, recalculate_);
        if(recalculate_ && parent == nullptr)
            Recalculate();
        if(style == nullptr)
            style = Style::Default();
    }

    Element::Element(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const StyleRef& style_, Element* parent_, bool recalculate_)
        : l_offset(offset_), l_size(size_), l_zOffset(zOffset_), style(style_) {
        SetParent(parent_, recalculate_);
        if(recalculate_ && parent == nullptr)
            Recalculate();
        if(style == nullptr)
            style = Style::Default();
    }

    Element::~Element() {
        for(Element* child : children)
            delete child;
        children.clear();
    }

    void Element::SetParent(Element* newParent, bool recalculate) {
        if(parent != newParent) {
            if(parent != nullptr)
                parent->RemoveChild(this, recalculate);
            if(newParent != nullptr)
                newParent->AddChild(this, recalculate);
        }
    }

    void Element::RemoveChild(Element* child, bool recalculate) {
        if(child != nullptr) {
            auto pos = std::find(children.begin(), children.end(), child);
            if(pos != children.end()) {
                children.erase(pos);
            }
            child->parent = nullptr;

            if(recalculate)
                child->Recalculate();
        }
    }

    Element* Element::AddChild(Element* child, bool recalculate) {
        if(child != nullptr) {
            auto pos = std::find(children.begin(), children.end(), child);
            if(pos == children.end()) {
                children.push_back(child);
            }
            child->parent = this;

            if(recalculate)
                child->InnerRecalculate(position, size, zIdx);
        }
        return child;
    }

    AABB Element::GetAABB() {
        return AABB(
            (position + 1.f - size) * 0.5f,
            (position + 1.f + size) * 0.5f
        );
    }

    void Element::Render() {
        InnerRender();
        for(Element* child : children)
            child->Render();
    }

    void Element::Recalculate() {
        InnerRecalculate(glm::vec2(0.f), glm::vec2(1.f), 0.f);
    }

    Element* Element::ResolveMouseSelection(const glm::vec2& mousePos_n) {
        //ignoring overlaps -> returns the first found element
        for(Element* child : children) {
            Element* selected = child->ResolveMouseSelection(mousePos_n);
            if(selected != nullptr)
                return selected;
        }

        if(GetAABB().IsInside(mousePos_n))
            return this;
        else
            return nullptr;
    }

    void Element::OnHover() {
        hover = true;
    }

    void Element::OnPressed() {
        pressed = true;
    }

    void Element::Highlight() {
        highlight = true;
    }

    void Element::InnerRender() {
        TextureRef t = pressed ? style->pressedTexture : (hover ? style->hoverTexture : style->texture);
        Renderer::RenderQuad(Quad::FromCenter(glm::vec3(position.x, -position.y, Z_INDEX_BASE - zIdx * Z_INDEX_MULT), size, style->color, t));
        if(highlight) {
            Renderer::RenderQuad(Quad::FromCenter(glm::vec3(position.x, -position.y, Z_INDEX_BASE - zIdx * Z_INDEX_MULT - Z_TEXT_OFFSET), size, glm::vec4(1.f), style->highlightTexture));
        }
        hover = pressed = highlight = false;
    }

    void Element::InnerRecalculate(const glm::vec2& p_position, const glm::vec2& p_size, float p_zIdx) {
        position = p_position + l_offset * p_size;
        size = p_size * l_size;
        zIdx = p_zIdx + l_zOffset;

        for(Element* child : children)
            child->InnerRecalculate(position, size, zIdx);
    }

    Element* Element::ResolveMouseSelection_old(const glm::vec2& mousePos_n) {
        if(GetAABB().IsInside(mousePos_n)) {
            //ignoring overlaps -> returns the first found element
            for(Element* child : children) {
                Element* selected = child->ResolveMouseSelection_old(mousePos_n);
                if(selected != nullptr) return selected;
            }
            return this;
        }
        else {
            return nullptr;
        }
    }

    //===== Button =====

    Button::Button(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const StyleRef& style_,
                    ButtonCallbackHandler* handler_, ButtonCallbackType callback_, int buttonID_)
        : Element(offset_, size_, zOffset_, style_, nullptr), handler(handler_), callback(callback_), id(buttonID_) {}

    void Button::OnUp() {
        // ENG_LOG_FINER("Button CLICK - '{}'", id);
        if(callback && handler) {
            callback(handler, id);
        }
    }

    //===== TextButton =====

    TextButton::TextButton(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const StyleRef& style_,
            const std::string& text_, ButtonCallbackHandler* handler_, ButtonCallbackType callback_, int highlightIdx_, int buttonID_)
        : Button(offset_, size_, zOffset_, style_, handler_, callback_, buttonID_), text(text_), highlightIdx(highlightIdx_) {}

    void TextButton::InnerRender() {
        glm::vec4 clr = (Hover() || Pressed()) ? style->hoverColor : style->textColor;
        glm::ivec2 pxOffset = Pressed() ? style->pressedOffset : glm::ivec2(0);
        Button::InnerRender();
        style->font->RenderTextCentered(text.c_str(), glm::vec2(position.x, -position.y), 1.f, 
            clr, style->hoverColor, highlightIdx, pxOffset, Z_INDEX_BASE - zIdx * Z_INDEX_MULT - Z_TEXT_OFFSET
        );
    }

    //===== Menu =====

    Menu::Menu(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const std::vector<Element*>& content)
        : Menu(offset_, size_, zOffset_, Style::Default(), content) {}

    Menu::Menu(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const StyleRef& style_, const std::vector<Element*>& content)
        : Element(offset_, size_, zOffset_, style_, nullptr) {
        for(Element* el : content) {
            AddChild(el, true);
        }
    }
    
    //===== ScrollMenu =====



}//namespace eng::GUI
