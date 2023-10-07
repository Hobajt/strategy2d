#pragma once

#include <memory>
#include <vector>
#include <string>
#include <array>
#include <unordered_map>

#include "engine/utils/mathdefs.h"
#include "engine/core/texture.h"
#include "engine/core/text.h"

#include "engine/core/selection.h"
#include "engine/core/sprite.h"

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

    //===== Style =====

    struct Style;
    using StyleRef = std::shared_ptr<Style>;
    using StyleMap = std::unordered_map<std::string, GUI::StyleRef>;

    namespace TextAlignment { enum { LEFT, CENTER, RIGHT }; }
    namespace HighlightMode { enum { NONE, TEXTURE, TEXT }; }

    //Container for all the style properties of GUI elements.
    struct Style {
        TextureRef texture = nullptr;
        glm::vec4 color = glm::vec4(1.f);

        TextureRef hoverTexture = nullptr;
        glm::vec4 hoverColor = glm::vec4(1.f);

        TextureRef holdTexture = nullptr;
        glm::ivec2 holdOffset = glm::ivec2(0);

        TextureRef highlightTexture = nullptr;
        glm::vec4 highlightColor = glm::vec4(1.f);
        int highlightMode = HighlightMode::TEXTURE;

        FontRef font = nullptr;
        glm::vec4 textColor = glm::vec4(1.f);

        int textAlignment = TextAlignment::CENTER;
        float textScale = 1.f;
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

        Element* GetChild(int idx);

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
        virtual void OnHold() override;
        virtual void OnHighlight() override;

        void Interactable(bool state) { interactable = state; }

        void UpdateOffset(const glm::vec2& offset, bool recalculate = true);

        void SetHighlight(bool active) { highlight = active; }
    protected:
        virtual void InnerRender();

        bool Hover() const { return hover; }
        bool Hold() const { return hold; }
        bool Highlight() const { return highlight; }
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
        bool hold = false;
        bool highlight = false;

        bool interactable = true;
    };

    //===== SelectionHandler =====

    //Helper class for GUI updates & event signaling.
    struct SelectionHandler {
        Element* lastSelected;
        Element* clickedElement;
    public:
        //Uses current mouse pos to detect & signal clicks/hovers/holds in given gui subtree.
        void Update(Element* gui);
    };

    //===== TextLabel =====

    //Uneditable block of text.
    class TextLabel : public Element {
    public:
        TextLabel() = default;
        TextLabel(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style, const std::string& text, bool centered = true);

        virtual void OnHover() override {}
        virtual void OnHold() override {}
        virtual void OnHighlight() override {}
    protected:
        virtual void InnerRender() override;
    private:
        std::string text;
        bool centered;
    };

    //===== Button =====

    //Base class for objects that can be acted upon by button callbacks.
    class ButtonCallbackHandler {};

    typedef void(*ButtonCallbackType)(ButtonCallbackHandler* handler, int buttonID);

    namespace ButtonFlags {
        enum {
            FIRE_ON_DOWN = BIT(0),
            FIRE_ON_HOLD = BIT(1)
        };
    }//namespace ButtonFlags

    class Button : public Element {
    public:
        Button() = default;
        Button(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style,
            ButtonCallbackHandler* handler, ButtonCallbackType callback, int buttonID = -1
        );
        Button(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style,
            ButtonCallbackHandler* handler, ButtonCallbackType callback, int buttonID, int fireType
        );

        virtual void OnDown() override;
        virtual void OnHold() override;
        virtual void OnUp() override;
    private:
        int id = -1;
        ButtonCallbackType callback = nullptr;
        ButtonCallbackHandler* handler = nullptr;
        bool fireOnDown = false;
        bool fireOnHold = false;
    };

    //===== TextButton =====

    class TextButton : public Button {
    public:
        TextButton() = default;
        TextButton(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style, const std::string& text,
            ButtonCallbackHandler* handler, ButtonCallbackType callback, int highlightIdx = -1, int buttonID = -1
        );
        TextButton(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style, const std::string& text,
            ButtonCallbackHandler* handler, ButtonCallbackType callback, int highlightIdx, int buttonID, int firingType
        );

        void Text(const std::string& newText);
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

        virtual void OnHover() override {}
        virtual void OnHold() override {}
        virtual void OnHighlight() override {}
    };

    //===== ScrollBar =====

    class ScrollBarHandler : public ButtonCallbackHandler {
    public:
        virtual void SignalUp() = 0;
        virtual void SignalDown() = 0;
        virtual void SignalSlider() = 0;
    };

    class ScrollBar : public Element {
    public:
        ScrollBar(const glm::vec2& offset, const glm::vec2& size, float zOffset, float btnHeight, 
            ScrollBarHandler* handler, const StyleRef& upStyle, const StyleRef& downStyle, const StyleRef& sliderStyle, const StyleRef& sliderGripStyle);

        virtual void OnHover() override {}
        virtual void OnHold() override {}
        virtual void OnHighlight() override {}

        float GetClickPos();
        void UpdateSliderPos(float pos);
    protected:
        virtual void InnerRender() override {}
    private:
        Element* rails = nullptr;
        Element* slider = nullptr;
        float sMin, sRange;
    };

    //===== ScrollMenu =====

    class ScrollMenu : public Menu, public ScrollBarHandler {
    public:
        ScrollMenu(const glm::vec2& offset, const glm::vec2& size, float zOffset, int rowCount, float barWidth, const std::vector<StyleRef>& styles);

        virtual void SignalUp() override;
        virtual void SignalDown() override;
        virtual void SignalSlider() override;

        void UpdateContent(const std::vector<std::string>& items, bool resetPosition = true);
        void ResetPosition();

        void UpdateSelection(int btnIdx);
    protected:
        virtual void InnerRender() override;
    private:
        void MenuUpdate();
    private:
        int rowCount = -1;
        int pos = 0;
        int selectedItem = 0;

        std::vector<std::string> items;

        ScrollBar* scrollBar;
        std::vector<TextButton*> menuBtns;
    };

    //===== ScrollText =====

    namespace ScrollTextPosition { enum { TOP, BOT, LAST_LINE_TOP, LAST_LINE_GONE, FIRST_LINE_BOT, FIRST_LINE_GONE }; }//namespace ScrollTextPosition

    //A text area with scrolling option (no scroll bar element).
    class ScrollText : public Element {
    public:
        ScrollText() = default;
        ScrollText(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style, const std::string& text);

        virtual void OnHover() override {}
        virtual void OnHold() override {}
        virtual void OnHighlight() override {}

        size_t NumLines() const { return lines.size(); }
        float NumVisibleLines() const { return visibleLinesCount; }
        
        float GetPosition() const { return pos; }
        float GetPositionNormalized() const;

        //Returns true when FIRST_LINE_GONE.
        bool IsScrolledUp() const;
        //Returns true when LAST_LINE_GONE.
        bool IsScrolledDown() const;

        //Update position (scrolling text down). Returns true once IsScrolledDown.
        bool ScrollUpdate(float increment);

        //Given line will be at the top of the element (lines indexed from the top).
        void SetPosition(float line);

        //Use ScrollTextPosition enum values.
        void SetPositionPreset(int type);

        //t=<0,1>, 0 - FIRST_LINE_GONE, 1 - LAST_LINE_GONE (from empty to empty)
        void SetPositionNormalized(float t);

        void SetLineGap(float gap);

        void UpdateText(const std::string& text);
    protected:
        virtual void InnerRender() override;
    private:
        void ProcessText(const std::string& text);
        void ResizeLines(float lineHeight);

        float CharWidth(char c);
        float LineHeight();
    private:
        std::vector<std::string> lines;
        std::string text;
        FontRef font = nullptr;

        float lineGapMult = 1.1f;

        glm::vec2 lineSize = glm::vec2(0.f);
        float visibleLinesCount = 0.f;
        float pos = 0.f;
    };

    //===== ImageButton =====

    class ImageButton : public Button {
    public:
        ImageButton(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style, const Sprite& sprite, const glm::ivec2& idx,
            ButtonCallbackHandler* handler, ButtonCallbackType callback, int buttonID = -1, float image_scaledown = 0.95f
        );
        ImageButton(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style, const Sprite& sprite, const glm::ivec2& idx,
            ButtonCallbackHandler* handler, ButtonCallbackType callback, int buttonID, int fireType, float image_scaledown = 0.95f
        );
        ImageButton(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style, const Sprite& sprite, const glm::ivec2& idx, float image_scaledown,
            ButtonCallbackHandler* handler, ButtonCallbackType callback, int buttonID = -1
        );
        ImageButton(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style, const Sprite& sprite, const glm::ivec2& idx, float image_scaledown,
            ButtonCallbackHandler* handler, ButtonCallbackType callback, int buttonID, int fireType
        );
    protected:
        virtual void InnerRender() override;
    private:
        Sprite sprite;
        glm::ivec2 idx;
        float image_scaledown;
    };

    //===== ImageButtonGrid =====

    //9x9 grid of image buttons with modifiable image indices.
    class ImageButtonGrid : public Element {
    public:
        ImageButtonGrid(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& btn_style, const Sprite& sprite, int rows, int cols, ButtonCallbackHandler* handler, ButtonCallbackType callback);

        virtual void OnHover() override {}
        virtual void OnHold() override {}
        virtual void OnHighlight() override {}
    private:
        std::vector<ImageButton*> btns;
        glm::ivec2 grid_size;

        ButtonCallbackType callback = nullptr;
        ButtonCallbackHandler* handler = nullptr;
    };

}//namespace eng::GUI
