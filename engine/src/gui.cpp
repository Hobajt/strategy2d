#include "engine/core/gui.h"

#include "engine/core/window.h"
#include "engine/core/renderer.h"
#include "engine/core/quad.h"
#include "engine/core/input.h"

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

    Element* Element::GetChild(int idx) {
        return children.at(idx);
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

        if(GetAABB().IsInside(mousePos_n) && interactable)
            return this;
        else
            return nullptr;
    }

    void Element::OnHover() {
        hover = true;
    }

    void Element::OnHold() {
        hold = true;
    }

    void Element::OnHighlight() {
        highlight = (style->highlightMode != HighlightMode::NONE);
    }

    void Element::UpdateOffset(const glm::vec2& newOffset, bool recalculate) {
        l_offset = newOffset;
        if(recalculate) {
            if(parent)
                InnerRecalculate(parent->position, parent->size, parent->zIdx);
            else
                Recalculate();
        }
    }

    void Element::InnerRender() {
        TextureRef t = hold ? style->holdTexture : (hover ? style->hoverTexture : style->texture);
        Renderer::RenderQuad(Quad::FromCenter(glm::vec3(position.x, -position.y, Z_INDEX_BASE - zIdx * Z_INDEX_MULT), size, style->color, t));
        if(highlight && style->highlightMode == HighlightMode::TEXTURE) {
            Renderer::RenderQuad(Quad::FromCenter(glm::vec3(position.x, -position.y, Z_INDEX_BASE - zIdx * Z_INDEX_MULT - Z_TEXT_OFFSET), size, style->highlightColor, style->highlightTexture));
        }
        hover = hold = highlight = false;
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

    //===== TextLabel =====

    TextLabel::TextLabel(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const StyleRef& style_, const std::string& text_)
        : Element(offset_, size_, zOffset_, style_, nullptr), text(text_) {}
    
    void TextLabel::InnerRender() {
        Element::InnerRender();
        style->font->RenderTextCentered(text.c_str(), glm::vec2(position.x, -position.y), style->textScale, style->textColor, Z_INDEX_BASE - zIdx * Z_INDEX_MULT - Z_TEXT_OFFSET);
    }

    //===== Button =====

    Button::Button(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const StyleRef& style_,
                    ButtonCallbackHandler* handler_, ButtonCallbackType callback_, int buttonID_)
        : Button(offset_, size_, zOffset_, style_, handler_, callback_, buttonID_, 0) {}
    
    Button::Button(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const StyleRef& style_,
                    ButtonCallbackHandler* handler_, ButtonCallbackType callback_, int buttonID_, int firingType)
        : Element(offset_, size_, zOffset_, style_, nullptr), handler(handler_), callback(callback_), id(buttonID_),
        fireOnDown(HAS_FLAG(firingType, ButtonFlags::FIRE_ON_DOWN)), fireOnHold(HAS_FLAG(firingType, ButtonFlags::FIRE_ON_HOLD)) {}

    void Button::OnDown() {
        // ENG_LOG_FINER("Button DOWN - '{}'", id);
        if(callback && handler && fireOnDown && !fireOnHold) {
            callback(handler, id);
        }
    }

    void Button::OnHold() {
        // ENG_LOG_FINEST("Button HOLD - '{}'", id);
        Element::OnHold();
        if(callback && handler && fireOnHold) {
            callback(handler, id);
        }
    }

    void Button::OnUp() {
        // ENG_LOG_FINER("Button UP - '{}'", id);
        if(callback && handler && !fireOnDown && !fireOnHold) {
            callback(handler, id);
        }
    }

    //===== TextButton =====

    TextButton::TextButton(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const StyleRef& style_,
            const std::string& text_, ButtonCallbackHandler* handler_, ButtonCallbackType callback_, int highlightIdx_, int buttonID_)
        : Button(offset_, size_, zOffset_, style_, handler_, callback_, buttonID_), text(text_), highlightIdx(highlightIdx_) {}
    
    TextButton::TextButton(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const StyleRef& style_,
            const std::string& text_, ButtonCallbackHandler* handler_, ButtonCallbackType callback_, int highlightIdx_, int buttonID_, int firingType)
        : Button(offset_, size_, zOffset_, style_, handler_, callback_, buttonID_, firingType), text(text_), highlightIdx(highlightIdx_) {}

    void TextButton::Text(const std::string& newText) {
        text = newText;
    }

    void TextButton::InnerRender() {
        glm::vec4 clr = (Hover() || Hold()) ? style->hoverColor : style->textColor;
        clr = (Highlight() && style->highlightMode == HighlightMode::TEXT) ? style->highlightColor : clr;
        glm::ivec2 pxOffset = Hold() ? style->holdOffset : glm::ivec2(0);
        Button::InnerRender();

        style->font->RenderTextCentered(text.c_str(), glm::vec2(position.x, -position.y), style->textScale, 
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

    //===== ScrollBar =====

    ScrollBar::ScrollBar(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, float btnHeight,
            ScrollBarHandler* handler, const StyleRef& upStyle, const StyleRef& downStyle, const StyleRef& sliderStyle, const StyleRef& sliderGripStyle) 
        : Element(offset_, size_, zOffset_, Style::Default(), nullptr) {

        float bh = btnHeight / size_.y;
        
        //scroll up button
        AddChild(new Button(glm::vec2(0.f, -1.f + bh), glm::vec2(1.f, bh), 0.0f, upStyle, 
            handler, [](GUI::ButtonCallbackHandler* handler, int id) {
                static_cast<ScrollBarHandler*>(handler)->SignalUp();
            }, 69, ButtonFlags::FIRE_ON_DOWN), true
        );

        //scroll down button
        AddChild(new Button(glm::vec2(0.f, 1.f - bh), glm::vec2(1.f, bh), 0.0f, downStyle, 
            handler, [](GUI::ButtonCallbackHandler* handler, int id) {
                static_cast<ScrollBarHandler*>(handler)->SignalDown();
            }, -1, ButtonFlags::FIRE_ON_DOWN), true
        );
        
        //slider rails
        rails = AddChild(new Button(glm::vec2(0.f, 0.f), glm::vec2(1.f, 1.f - 2.f*bh), 0.0f, sliderStyle, 
            handler, [](GUI::ButtonCallbackHandler* handler, int id) {
                static_cast<ScrollBarHandler*>(handler)->SignalSlider();
            }, -1, ButtonFlags::FIRE_ON_HOLD), true
        );

        //slider grip
        slider = AddChild(new Button(glm::vec2(0.f, 0.f), glm::vec2(1.f, bh), 0.1f, sliderGripStyle, nullptr, nullptr, true), true);
        slider->Interactable(false);     //disable interactions so that it doesn't interfere with slider handler

        //values for easier grip positioning
        sMin = -1.f + 3.f * bh;
        float sMax = 1.f - 3.f * bh;
        sRange = sMax - sMin;
    }

    float ScrollBar::GetClickPos() {
        AABB bb = rails->GetAABB();
        float y = (Input::Get().mousePos_n.y - bb.min.y) / (bb.max.y - bb.min.y);
        return y;
    }

    void ScrollBar::UpdateSliderPos(float pos) {
        slider->UpdateOffset(glm::vec2(0.f, sMin + pos * sRange));
    }
    
    //===== ScrollMenu =====

    ScrollMenu::ScrollMenu(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, int rowCount_, float barWidth, const std::vector<StyleRef>& styles)
        : Menu(offset_, size_, zOffset_, Style::Default(), {}), rowCount(rowCount_) {
        ASSERT_MSG(styles.size() >= 5, "Scroll menu requires 5 gui styles (item, up button, down button, slider button & slider grip button)")

        //generate children based on item count & add them to the hierarchy
        float btnHeight = size_.y / rowCount;
        float off = -1.f + btnHeight / size_.y;
        float step = (2.f * btnHeight) / size_.y;
        float bh = btnHeight / size_.y;
        char buf [256];
        TextButton* btn;
        for(int i = 0; i < rowCount; i++) {
            snprintf(buf, sizeof(buf), "item_%d", i);
            btn = (TextButton*)AddChild(new TextButton(glm::vec2(0.f, off+step*i), glm::vec2(1.f, bh), 0.1f, styles[0], std::string(buf), 
                this, [](GUI::ButtonCallbackHandler* handler, int id) {
                    static_cast<ScrollMenu*>(handler)->UpdateSelection(id);
                }, -1, i, ButtonFlags::FIRE_ON_DOWN), true
            );
            menuBtns.push_back(btn);
        }

        //generate scrollbar
        float gap = 0.0f;
        glm::vec2 bar = glm::vec2(barWidth / size_.x, barWidth / size_.y);
        scrollBar = (ScrollBar*)AddChild(
            new ScrollBar(glm::vec2(1.f + gap + bar.x, 0.f), glm::vec2(bar.x, 1.f), 0.f, bar.y, this, 
                styles[1], styles[2], styles[3], styles[4]
            ), true
        );
    }

    void ScrollMenu::SignalUp() {
        if((--pos) < 0)
            pos = 0;
        MenuUpdate();
    }

    void ScrollMenu::SignalDown() {
        if((pos++) >= items.size() - rowCount)
            pos = std::max(int(items.size() - rowCount), 0);
        MenuUpdate();
    }

    void ScrollMenu::SignalSlider() {
        float yPos = scrollBar->GetClickPos();
        pos = std::max((int)roundf(yPos * (items.size() - rowCount)), 0);
        MenuUpdate();
    }

    void ScrollMenu::UpdateContent(const std::vector<std::string>& items_, bool resetPosition) {
        items = items_;
        if(resetPosition)
            ResetPosition();
        MenuUpdate();
    }

    void ScrollMenu::ResetPosition() {
        pos = 0;
        selectedItem = 0;
    }

    void ScrollMenu::UpdateSelection(int btnIdx) {
        selectedItem = pos + btnIdx;
    }

    void ScrollMenu::InnerRender() {
        Element::InnerRender();

        //override highlights to only highlight the selected item
        int selectedBtn = selectedItem - pos;
        for(TextButton* btn : menuBtns)
            btn->SetHighlight(false);
        if(selectedBtn >= 0 && selectedBtn < menuBtns.size()) {
            menuBtns[selectedBtn]->SetHighlight(true);
        }
    }

    void ScrollMenu::MenuUpdate() {
        //update visible text values
        for(int i = 0; i < rowCount; i++) {
            if(pos+i < items.size())
                menuBtns[i]->Text(items[pos+i]);
            else
                menuBtns[i]->Text("");
        }

        //update scrollbar slider position
        int scrollSize = items.size() - rowCount;
        float sliderPos = (scrollSize > 0) ? (pos / float(scrollSize)) : 0.f;
        scrollBar->UpdateSliderPos(sliderPos);
    }

}//namespace eng::GUI
