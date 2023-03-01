#pragma once

#include <memory>
#include <vector>
#include <string>

#include "engine/utils/mathdefs.h"
#include "engine/core/texture.h"
#include "engine/core/text.h"

#include "engine/core/selection.h"

namespace eng::GUI {

   /* GUI values description:
        - coordinates - values are relative to the parent (no parent = entire screen)
                      - coord range isn't in pixels, it's normalized
                      - coords go from top-left to bottom-right - (-1,-1) is top-left, (0,0) is center
        - offset    - defines center of given element; is relative to parent's center
        - size      - defines quarter size of element's bounding rectangle - units from it's center to any corner
                    - is also relative to parent (it multiplies) -> (0.5, 0.5) = half the size of parent element
        - z-index   - defines order of rendering; also relative to parent element
                    - value range is <-1900, 100> - this maps to <-1,1> depth in NDC
                        - z=0 maps to -0.9 in NDC
                        - z=100 maps to -1 in NDC (most front position)
                    - any text in GUI is rendered slightly in front of its element (z += 0.1 ... -1e-3 in NDC)
   */

    //TODO: consider remaking GUI's transform system - it's kinda unintuitive to define sizes relative to parent

    //===== Style =====

    struct Style;
    using StyleRef = std::shared_ptr<Style>;

    //Container for all the style properties of GUI elements.
    struct Style {
        TextureRef texture = nullptr;
        glm::vec4 color = glm::vec4(1.f);

        TextureRef hoverTexture = nullptr;
        glm::vec4 hoverColor = glm::vec4(1.f);

        TextureRef pressedTexture = nullptr;
        glm::ivec2 pressedOffset = glm::ivec2(0);

        FontRef font = nullptr;
        glm::vec4 textColor = glm::vec4(1.f);
    public:
        static StyleRef Default();
    };

    //===== Element =====

    //Base class for any kind of GUI object.
    //Assigning a child shifts its ownership onto the parent element.
    class Element : public ScreenObject {
    public:
        Element(Element* parent = nullptr, bool recalculate = true);
        Element(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style, Element* parent, bool recalculate = true);
        virtual ~Element();

        //copy disabled
        Element(const Element&) = delete;
        Element& operator=(const Element&) = delete;

        //move enabled
        Element(Element&&) noexcept = default;
        Element& operator=(Element&&) noexcept = default;
        
        void SetParent(Element* parent, bool recalculate = true);
        void RemoveChild(Element* child, bool recalculate = true);
        Element* AddChild(Element* child, bool recalculate = true);

        virtual AABB GetAABB() override;

        //Renders the element along with all of its children.
        void Render();

        //Recomputes global transform values in the entire hierarchy.
        //Call only on the root element.
        void Recalculate();

        //Returns element from the hierarchy which is being hovered over.
        //Ignores overlaps among children - returns the first found.
        Element* ResolveMouseSelection(const glm::vec2& mousePos_n);

        virtual void OnHover() override;
        virtual void OnPressed() override;
    protected:
        virtual void InnerRender();

        bool Hover() const { return hover; }
        bool Pressed() const { return pressed; }
    private:
        void InnerRecalculate(const glm::vec2& parent_position, const glm::vec2& parent_size, float parent_zIdx);

        //Returns element from the hierarchy which is being hovered over.
        //Ignores overlaps among children - returns the first found.
        //Only works properly if children elements are contained within parent's bounding box.
        Element* ResolveMouseSelection_old(const glm::vec2& mousePos_n);
    protected:
        //transform values, relative to parent
        glm::vec2 l_offset = glm::vec2(0.f);
        glm::vec2 l_size = glm::vec2(1.f);
        float l_zOffset = 0.f;

        //transform values, given globally
        glm::vec2 position;
        glm::vec2 size;
        float zIdx;

        StyleRef style = nullptr;
    private:
        Element* parent = nullptr;
        std::vector<Element*> children;

        bool hover = false;
        bool pressed = false;
    };

    //===== Button =====

    //Base class for objects that can be acted upon by button callbacks.
    class ButtonCallbackHandler {};

    typedef void(*ButtonCallbackType)(ButtonCallbackHandler* handler, int buttonID);

    class Button : public Element {
    public:
        Button(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style,
            ButtonCallbackHandler* handler, ButtonCallbackType callback, int buttonID = -1
        );

        // virtual void OnHover() override;
        virtual void OnUp() override;
    private:
        int id = -1;
        ButtonCallbackType callback = nullptr;
        ButtonCallbackHandler* handler = nullptr;
    };

    //===== TextButton =====

    class TextButton : public Button {
    public:
        TextButton(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style, const std::string& text,
            ButtonCallbackHandler* handler, ButtonCallbackType callback, int highlightIdx = -1, int buttonID = -1
        );
    protected:
        virtual void InnerRender() override;
    private:
        std::string text;
        int highlightIdx = -1;
    };

    //===== Menu =====

    class Menu : public Element {
    public:
        Menu() = default;
        Menu(const glm::vec2& offset, const glm::vec2& size, float zOffset, const std::vector<Element*>& content);
        Menu(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style, const std::vector<Element*>& content);
    };

    //===== ScrollMenu =====

    // public ScrollMenu : public Menu {};

}//namespace eng::GUI
