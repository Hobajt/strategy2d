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

    class ButtonCallbackHandler;

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

        std::vector<Element*>::iterator begin() { return children.begin(); }
        std::vector<Element*>::iterator end() { return children.end(); }
        std::vector<Element*>::const_iterator cbegin() { return children.cbegin(); }
        std::vector<Element*>::const_iterator cend() { return children.cend(); }

        virtual void HandlerPtrMove(ButtonCallbackHandler* oldPtr, ButtonCallbackHandler* newPtr) {}

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
        void Enable(bool enabled_) { enabled = enabled_; }
        bool IsEnabled() const { return enabled; }

        void UpdateOffset(const glm::vec2& offset, bool recalculate = true);

        void SetHighlight(bool active) { highlight = active; }

        void ChangeStyle(const StyleRef& style_) { style = style_; }
        GUI::StyleRef Style() const { return style; }
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
        bool enabled = true;
    };

    //===== SelectionHandler =====

    //Helper class for GUI updates & event signaling.
    struct SelectionHandler {
        Element* lastSelected;
        Element* clickedElement;
        Element* hovering;
    public:
        //Uses current mouse pos to detect & signal clicks/hovers/holds in given gui subtree.
        void Update(Element* gui);

        Element* HoveringElement() { return hovering; }
    };

    //===== TextLabel =====

    //Uneditable block of text.
    class TextLabel : public Element {
    public:
        TextLabel() = default;
        TextLabel(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style, const std::string& text, bool centered = true);
        TextLabel(const glm::vec2& offset, const glm::vec2& size, float zOffset, int highlightIdx, const StyleRef& style, const std::string& text, bool centered = true);

        virtual void OnHover() override {}
        virtual void OnHold() override {}
        virtual void OnHighlight() override {}

        void Setup(const std::string& text, int highlightIdx = -1, bool enable = true);
    protected:
        virtual void InnerRender() override;
    private:
        std::string text;
        bool centered;
        int highlightIdx = -1;
    };

    //===== TextInput =====

    class TextInput : public Element {
    public:
        TextInput() = default;
        ~TextInput();

        TextInput(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style, int max_length);

        TextInput(const TextInput&) = delete;
        TextInput& operator=(const TextInput&) = delete;

        virtual void OnHover() override {}
        virtual void OnHold() override {}
        virtual void OnHighlight() override {}

        void Reset();
        void Backspace();
        void AddChar(char c);

        std::string Text();
        void SetText(const std::string& text);
    protected:
        virtual void InnerRender() override;
    private:
        char* data = nullptr;
        size_t max_length = 0;
        size_t pos = 0;
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

        void Invoke();

        virtual void HandlerPtrMove(ButtonCallbackHandler* oldPtr, ButtonCallbackHandler* newPtr) override { handler = (handler == oldPtr) ? newPtr : handler; }
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
        TextButton(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style, const std::string& text,
            ButtonCallbackHandler* handler, ButtonCallbackType callback, const glm::ivec2& highlightRange, int buttonID = -1, int firingType = 0
        );

        void Text(const std::string& newText, const glm::ivec2& highlightRange = glm::ivec2(-1));
    protected:
        virtual void InnerRender() override;

        //Call before parent::InnerRender() because of the values reset.
        void RenderText(bool centered = true, const glm::vec2& offset = glm::vec2(0.f));
    private:
        std::string text;
        glm::ivec2 highlight_range;
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
            ScrollBarHandler* handler, const StyleRef& upStyle, const StyleRef& downStyle, const StyleRef& sliderStyle, const StyleRef& sliderGripStyle,
            bool horizontal = false
        );

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
        bool horizontal = false;
    };

    //===== ScrollMenu =====

    class ScrollMenu : public Menu, public ScrollBarHandler {
    public:
        ScrollMenu(const glm::vec2& offset, const glm::vec2& size, float zOffset, int rowCount, float barWidth, const std::vector<StyleRef>& styles, ButtonCallbackHandler* handler = nullptr, ButtonCallbackType callback = nullptr);

        virtual void SignalUp() override;
        virtual void SignalDown() override;
        virtual void SignalSlider() override;

        void UpdateContent(const std::vector<std::string>& items, bool resetPosition = true);
        void ResetPosition();

        void UpdateSelection(int btnIdx);

        std::string CurrentSelection() const { return items[selectedItem]; }
        int CurrentSelectionIdx() const { return selectedItem; }
        int ItemCount() const { return items.size(); }

        virtual void HandlerPtrMove(ButtonCallbackHandler* oldPtr, ButtonCallbackHandler* newPtr) override { menu_handler = (menu_handler == oldPtr) ? newPtr : menu_handler; }
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

        ButtonCallbackType menu_callback = nullptr;
        ButtonCallbackHandler* menu_handler = nullptr;
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

        virtual void Setup(const std::string& name, int highlightIdx, const glm::ivec2& idx, float value, bool enable = true);
        virtual void Setup(const std::string& name, int highlightIdx, const glm::ivec2& idx, const glm::ivec4& price, bool enable = true);
        virtual void SetValue(float value) {}

        std::string Name() const { return name; }
        int HighlightIdx() const { return highlightIdx; }
        glm::ivec4 Price() const { return price; }
    protected:
        virtual void InnerRender() override;
    private:
        Sprite sprite;
        float image_scaledown;

        glm::ivec2 idx;
        std::string name;
        glm::ivec4 price;
        int highlightIdx;
    };

    //===== ImageButtonWithBar =====

    class ImageButtonWithBar : public ImageButton {
    public:
        ImageButtonWithBar(const glm::vec2& offset, const glm::vec2& size, float zOffset, 
            const StyleRef& style, const StyleRef& bar_style, const glm::vec2& borders_size, 
            const Sprite& sprite, const glm::ivec2& idx,
            ButtonCallbackHandler* handler, ButtonCallbackType callback, int buttonID = -1, float image_scaledown = 0.95f
        );
        ImageButtonWithBar(const glm::vec2& offset, const glm::vec2& size, float zOffset, 
            const StyleRef& style, const StyleRef& bar_style, const glm::vec2& borders_size, 
            const Sprite& sprite, const glm::ivec2& idx,
            ButtonCallbackHandler* handler, ButtonCallbackType callback, int buttonID, int fireType, float image_scaledown = 0.95f
        );
        ImageButtonWithBar(const glm::vec2& offset, const glm::vec2& size, float zOffset, 
            const StyleRef& style, const StyleRef& bar_style, const glm::vec2& borders_size, 
            const Sprite& sprite, const glm::ivec2& idx, float image_scaledown,
            ButtonCallbackHandler* handler, ButtonCallbackType callback, int buttonID = -1
        );
        ImageButtonWithBar(const glm::vec2& offset, const glm::vec2& size, float zOffset, 
            const StyleRef& style, const StyleRef& bar_style, const glm::vec2& borders_size, 
            const Sprite& sprite, const glm::ivec2& idx, float image_scaledown,
            ButtonCallbackHandler* handler, ButtonCallbackType callback, int buttonID, int fireType
        );

        virtual void Setup(const std::string& name, int highlightIdx, const glm::ivec2& idx, float value, bool enable = true) override;
        virtual void SetValue(float value) override;

        //override in order for the button to work even when hovering over the bar
        virtual AABB GetAABB() override;

        static float BarHeightRatio();
    protected:
        virtual void InnerRender() override;
        glm::vec4 ColorFromValue() const;
    private:
        float value = 1.f;
        StyleRef bar_style;
        glm::vec2 borders_size;
    };

    //===== ValueBar =====

    class ValueBar : public Element {
    public:
        ValueBar() = default;
        ValueBar(const glm::vec2& offset, const glm::vec2& size, float zOffset, 
            const StyleRef& bar_style, const glm::vec2& borders_size, const StyleRef& text_style, const glm::vec4 filler_clr, const std::string& text);
        
        void SetValue(float value);
        void SetText(const std::string& text);

        void Setup(float value, const std::string& text, bool enable = true);
    protected:
        virtual void InnerRender() override;
    private:
        float value;
        std::string text;
        glm::vec4 filler_clr;
        StyleRef text_style;
        glm::vec2 borders_size;
    };

    //===== KeyValue =====

    class KeyValue : public Element {
    public:
        //Position identifies where the separator is going to be located. If there's no separator, the whole text is aligned to the right.
        KeyValue(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style, const std::string& text);

        virtual void OnHover() override {}
        virtual void OnHold() override {}
        virtual void OnHighlight() override {}

        void Setup(const std::string& text, bool enable = true);
        void Setup(const std::string& text, const glm::ivec2& highlight_idx, bool enable = true);
    protected:
        virtual void InnerRender() override;
    private:
        std::string text;
        glm::ivec2 highlight_idx;
        size_t sep_pos;
    };


    //===== ImageAndLabel =====

    class ImageAndLabel : public Element {
    public:
        ImageAndLabel(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style, const Sprite& sprite, const glm::ivec2& icon, const std::string& text, const glm::ivec2& highlight_range = glm::ivec2(-1));

        virtual void OnHover() override {}
        virtual void OnHold() override {}
        virtual void OnHighlight() override {}

        void Setup(const glm::ivec2& icon, const std::string& text, const glm::ivec2& highlight_range = glm::ivec2(-1), bool enable = true);
    protected:
        virtual void InnerRender() override;
    private:
        glm::ivec2 icon;
        std::string text;
        glm::ivec2 highlight_range;
        Sprite sprite;
    };

    //===== ImageButtonGrid =====

    //9x9 grid of image buttons with modifiable image indices.
    class ImageButtonGrid : public Element {
    public:
        ImageButtonGrid(const glm::vec2& offset, const glm::vec2& size, float zOffset, 
            const StyleRef& btn_style, const Sprite& sprite, int rows, int cols, const glm::vec2& btn_size,
            ButtonCallbackHandler* handler, ButtonCallbackType callback
        );

        ImageButtonGrid(const glm::vec2& offset, const glm::vec2& size, float zOffset, 
            const StyleRef& btn_style, const StyleRef& bar_style, const glm::vec2& bar_borders_size, const Sprite& sprite, int rows, int cols, const glm::vec2& btn_size,
            ButtonCallbackHandler* handler, ButtonCallbackType callback
        );

        virtual void OnHover() override {}
        virtual void OnHold() override {}
        virtual void OnHighlight() override {}

        virtual void HandlerPtrMove(ButtonCallbackHandler* oldPtr, ButtonCallbackHandler* newPtr) override { handler = (handler == oldPtr) ? newPtr : handler; }

        ImageButton* GetButton(int idx) { return btns.at(idx); }
    private:
        std::vector<ImageButton*> btns;
        glm::ivec2 grid_size;

        ButtonCallbackType callback = nullptr;
        ButtonCallbackHandler* handler = nullptr;
    };

    //===== ValueSlider =====

    class ValueSlider : public ScrollBar, ScrollBarHandler {
    public:
        ValueSlider(const glm::vec2& offset, const glm::vec2& size, float zOffset, float btnHeight, const std::vector<StyleRef>& styles, const glm::vec2& val_range, int step_count, bool horizontal = true);

        virtual void SignalUp() override;
        virtual void SignalDown() override;
        virtual void SignalSlider() override;

        void SetValue(float value);
        float Value() const;
    private:
        int pos = 0;

        glm::vec2 range;        
        float step;
        int step_count;
    };

    //===== SelectMenu =====

    class SelectMenu : public TextButton, ButtonCallbackHandler {
    public:
        SelectMenu(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& btn_style, const std::vector<std::string>& items, int defaultItem = 0);

        void UpdateSelection(int idx);
        void Unroll(bool unrolled);

        int CurrentSelection() const { return selection; }
    private:
        std::vector<TextButton*> menuBtns;
        std::vector<std::string> items;

        bool opened = false;
        int selection = 0;
    };

    //===== Radio =====

    class RadioButton : public TextButton {
    public:
        RadioButton(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style, const std::string& text,
            ButtonCallbackHandler* handler, ButtonCallbackType callback, int highlightIdx = -1, int buttonID = -1
        );
    protected:
        virtual void InnerRender() override;
    };

    class Radio : public Element, ButtonCallbackHandler {
    public:
        Radio(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& gem_style, const StyleRef& selected_style, const std::vector<std::string>& items, int selectionIdx = 0);

        void Select(int idx);

        int SelectionIdx() { return selection; }
        bool IsSelected(int idx) { return selection == idx; }
    private:
        std::vector<RadioButton*> btns;
        int selection;

        StyleRef gemStyle;
        StyleRef selectionStyle;
    };

}//namespace eng::GUI
