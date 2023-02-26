#pragma once

#include <vector>

#include "engine/utils/mathdefs.h"
#include "engine/core/texture.h"
#include "engine/core/text.h"

#include "engine/core/selection.h"

namespace eng::GUI {

    /*
    coord info:
        - child has relative coords to parent element
        - coordinates are taken from center of the screen, size is half of the actual object
            - meaning that element with position (0,0) and size (0.5,0.5) will render quad at the center, which goes 0.5 to all sides (NDC values)
        - top-left corner of the screen is at coords (-0.5, -0.5)
    z-index:
        - valid range is <-1900, 100> - this maps to <-1,1> depth in NDC
            - 0 maps to -0.9 depth in NDC
            - 100 maps to -1 in NDC -> the most front position (not sure if -1 still gets rendered, maybe 99 is better)
        - text in buttons is rendered slightly in front of the button (+0.1 z-index)
    */

    //===== Element =====

    class Element : public ScreenObject {
    public:
        Element(Element* parent_ = nullptr);
        Element(const glm::vec2& offset, const glm::vec2& size, float zOffset, const TextureRef& texture_, const glm::vec4& color_, Element* parent_ = nullptr);

        //Render this element along with all of it's children.
        void Render() const;

        // void Update();
        
        void LinkParent(Element* parent_) { parent = parent_; }
        void AddChild(Element* child);

        //note: for gui this only works on root elements (children will return wrong values)
        virtual AABB GetAABB() override;
    private:
        void InnerRenderHierarchy(const glm::vec2& parentPos, const glm::vec2& parentSize, float parentZIndex) const;
    protected:
        virtual void InnerRender(const glm::vec2& pos, const glm::vec2& size, float zIdx) const;
    public:
        glm::vec2 offset = glm::vec2(0.f);
        glm::vec2 size = glm::vec2(1.f);
        float zOffset = 0.f;

        glm::vec4 color = glm::vec4(1.f);
        TextureRef texture = nullptr;
    private:
        Element* parent;
        std::vector<Element*> children;
    };


    //Base class for objects that should process GUI button interaction.
    class ButtonCallbackHandler {
    public:
        virtual void OnClick(int buttonID) = 0;
    };

    //===== Button =====

    class Button : public Element {
    public:
        Button(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_,
            const TextureRef& texture_, const glm::vec4& color_, 
            ButtonCallbackHandler* handler_, int buttonID_, 
            const FontRef& font_,  const std::string& text_ = std::string(""), const glm::vec4& textColor_ = glm::vec4(1.f));

        virtual void OnHover() override;
        virtual void OnClick() override;
    protected:
        virtual void InnerRender(const glm::vec2& pos, const glm::vec2& size, float zIdx) const override;
    private:
        FontRef font;
        std::string text;
        glm::vec4 textColor;

        int id;
        ButtonCallbackHandler* handler = nullptr;
    };

    //===== Menu =====

    class Menu : public Element {
    public:
        Menu(const glm::vec2& offset, const glm::vec2& size, float zOffset, const std::vector<Button>& buttons, const TextureRef& texture = nullptr, const glm::vec4& color = glm::vec4(1.f));

        virtual ScreenObject* FetchLeaf(const glm::vec2& mousePos) override;
    public:
        std::vector<Button> buttons;
    };

}//namespace eng::GUI
