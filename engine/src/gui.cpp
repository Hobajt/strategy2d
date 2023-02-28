#include "engine/core/gui.h"

#include "engine/core/renderer.h"
#include "engine/core/quad.h"

#define Z_INDEX_BASE -0.9f
#define Z_INDEX_MULT 1e-3f
#define Z_TEXT_OFFSET 1e-4f

namespace eng::GUI {

    ElementProperties ElementProperties::Menu() {
        ElementProperties ep = {};
        ep.color = glm::vec4(glm::vec3(1.f), 0.f);
        return ep;
    }

    //===== Element =====

    Element::Element(Element* parent_, bool recalculate_) {
        SetParent(parent_, recalculate_);
        if(recalculate_ && parent == nullptr)
            Recalculate();
    }

    Element::Element(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const ElementProperties& props, Element* parent_, bool recalculate_)
        : l_offset(offset_), l_size(size_), l_zOffset(zOffset_), texture(props.texture), color(props.color), hoverTexture(props.hoverTexture), pressedTexture(props.pressedTexture) {
        SetParent(parent_, recalculate_);
        if(recalculate_ && parent == nullptr)
            Recalculate();
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

    void Element::InnerRender() {
        TextureRef t = pressed ? pressedTexture : (hover ? hoverTexture : texture);
        Renderer::RenderQuad(Quad::FromCenter(glm::vec3(position.x, -position.y, Z_INDEX_BASE - zIdx * Z_INDEX_MULT), size, color, t));
        hover = false;
        pressed = false;
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

    Button::Button(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const ElementProperties& props,
                    ButtonCallbackHandler* handler_, ButtonCallbackType callback_, int buttonID_)
        : Element(offset_, size_, zOffset_, props, nullptr), handler(handler_), callback(callback_), id(buttonID_) {}

    void Button::OnUp() {
        // ENG_LOG_FINER("Button CLICK - '{}'", id);
        if(callback && handler) {
            callback(handler, id);
        }
    }

    //===== TextButton =====

    TextButton::TextButton(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const ElementProperties& props,
            const std::string& text_, ButtonCallbackHandler* handler_, ButtonCallbackType callback_, int highlightIdx_, int buttonID_)
        : Button(offset_, size_, zOffset_, props, handler_, callback_, buttonID_), font(props.font), text(text_), textColor(props.textColor), highlightIdx(highlightIdx_) {}

    void TextButton::InnerRender() {
        glm::vec4 clr = (Hover() || Pressed()) ? hoverColor : textColor;
        Button::InnerRender();
        font->RenderTextCentered(text.c_str(), glm::vec2(position.x, -position.y), 1.f, clr, hoverColor, highlightIdx, Z_INDEX_BASE - zIdx * Z_INDEX_MULT - Z_TEXT_OFFSET);
    }

    //===== Menu =====

    Menu::Menu(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const std::vector<Element*>& content)
        : Menu(offset_, size_, zOffset_, ElementProperties::Menu(), content) {}

    Menu::Menu(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const ElementProperties& props, const std::vector<Element*>& content)
        : Element(offset_, size_, zOffset_, props, nullptr) {
        for(Element* el : content) {
            AddChild(el, true);
        }
    }
    
    //===== ScrollMenu =====



}//namespace eng::GUI
