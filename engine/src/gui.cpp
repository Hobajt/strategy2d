#include "engine/core/gui.h"

#include "engine/core/window.h"
#include "engine/core/renderer.h"
#include "engine/core/quad.h"
#include "engine/core/input.h"

#define Z_INDEX_BASE -0.9f
#define Z_INDEX_MULT 1e-3f
#define Z_TEXT_OFFSET 1e-4f

namespace eng::GUI {

    constexpr float HEALTH_BAR_HEIGHT_RATIO = 0.14f;

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
        if(enabled) {
            InnerRender();
            for(Element* child : children)
                child->Render();
        }
    }

    void Element::Recalculate() {
        InnerRecalculate(glm::vec2(0.f), glm::vec2(1.f), 0.f);
    }

    Element* Element::ResolveMouseSelection(const glm::vec2& mousePos_n) {
        if(!enabled)
            return nullptr;

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

    void SelectionHandler::Update(Element* gui) {
        Input& input = Input::Get();

        GUI::Element* selection = gui->ResolveMouseSelection(input.mousePos_n);
        if(selection != nullptr) {
            if(input.lmb.down()) {
                lastSelected = clickedElement = selection;
                selection->OnDown();
            }
            else if(input.lmb.pressed()) {
                //nested if in order to prevent OnHover calls while lmb is pressed
                if(selection == clickedElement)
                    selection->OnHold();
            }
            else if(input.lmb.up()) {
                if(selection == clickedElement) selection->OnUp();
                clickedElement = nullptr;
            }
            else {
                selection->OnHover();
            }
        }

        if(lastSelected != nullptr)
            lastSelected->OnHighlight();
    }

    //===== TextLabel =====

    TextLabel::TextLabel(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const StyleRef& style_, const std::string& text_, bool centered_)
        : Element(offset_, size_, zOffset_, style_, nullptr), text(text_), centered(centered_) {}
    
    void TextLabel::InnerRender() {
        Element::InnerRender();
        ASSERT_MSG(style->font != nullptr, "GUI element with text has to have a font assigned.");
        if(centered)
            style->font->RenderTextCentered(text.c_str(), glm::vec2(position.x, -position.y), style->textScale, style->textColor, Z_INDEX_BASE - zIdx * Z_INDEX_MULT - Z_TEXT_OFFSET);
        else
            style->font->RenderTextAlignLeft(text.c_str(), glm::vec2(position.x - size.x, -position.y), style->textScale, style->textColor, Z_INDEX_BASE - zIdx * Z_INDEX_MULT - Z_TEXT_OFFSET);
    }

    void TextLabel::Setup(const std::string& text_, bool enable) {
        text = text_;
        if(enable)
            Enable(true);
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
        ASSERT_MSG(style->font != nullptr, "GUI element with text has to have a font assigned.");

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

    //===== ScrollText =====

    ScrollText::ScrollText(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const StyleRef& style_, const std::string& text_) 
        : Element(offset_, size_, zOffset_, style_, nullptr), lineSize(glm::vec2(0.f)), visibleLinesCount(0.f), text(text_), font(nullptr) {
        
        if(style != nullptr && style->font != nullptr) {
            font = style->font;
            ResizeLines(LineHeight());
            ProcessText(text);
        }
    }

    void ScrollText::SetPosition(float line) {
        pos = line;
    }

    float ScrollText::GetPositionNormalized() const {
        return (pos+visibleLinesCount) / (lines.size() + 0.5f + visibleLinesCount);
    }

    bool ScrollText::IsScrolledUp() const {
        return pos <= -visibleLinesCount;
    }
    
    bool ScrollText::IsScrolledDown() const {
        return pos >= lines.size() + 0.5f;
    }

    bool ScrollText::ScrollUpdate(float t) {
        pos += t;
        return IsScrolledDown();
    }

    void ScrollText::SetPositionPreset(int type) {
        switch(type) {
            case ScrollTextPosition::TOP:               //first line visible at the top border (nothing else hidden above)
                pos = 0.f;
                break;
            case ScrollTextPosition::BOT:               //last line visible at the bottom border (nothing else hidden below)
                pos = lines.size() - visibleLinesCount;
                break;
            case ScrollTextPosition::FIRST_LINE_GONE:   //completely scrolled (first line not yet visible)
                pos = -visibleLinesCount;
                break;
            case ScrollTextPosition::FIRST_LINE_BOT:    //first line visible at the bottom border
                pos = -visibleLinesCount+1;
                break;
            case ScrollTextPosition::LAST_LINE_TOP:     //last line visible at the top border
                pos = lines.size() - 1;
                break;
            case ScrollTextPosition::LAST_LINE_GONE:    //completely scrolled (last line left through the top)
                pos = lines.size() + 0.5f;
                break;
        }
    }

    void ScrollText::SetPositionNormalized(float t) {
        pos = -visibleLinesCount + t * (lines.size() + 0.5f + visibleLinesCount);
    }

    void ScrollText::SetLineGap(float gap) {
        if(lineGapMult != gap) {
            lineGapMult = gap;
            ResizeLines(LineHeight());
            ProcessText(text);
        }
    }

    void ScrollText::UpdateText(const std::string& newText) {
        text = newText;
        ProcessText(text);
    }

    void ScrollText::ProcessText(const std::string& text) {
        lines.clear();
        ENG_LOG_TRACE("ScrollText::ProcessText triggered");

        //minimal word length needed to start splitting the word (% of line width)
        constexpr float longWordThresh = 1.f;

        //convert to pixels, so that character width's don't have to be divided lower in the code
        float lineWidth = lineSize.x * Window::Get().Width();

        if(lineSize.y < 1e-3f || lineWidth < 1e-3f) {
            // ENG_LOG_TRACE("ScrollText - Area too small.");
            return;
        }

        float currentLineLength = 0.f;
        float currentWordLength = 0.f;

        const char* c = text.c_str();
        const char* lastSpace = nullptr;
        const char* lineStart = c;

        // ENG_LOG_INFO("Text to process:");
        // ENG_LOG_INFO("{}", text);
        // ENG_LOG_INFO("===================");

        //TODO: process last line!!

        while(*c != '\0') {
            switch(*c) {
                case ' ':
                case '\n':
                    if(currentLineLength + currentWordLength <= lineWidth) {
                        //line can contain this word
                        currentLineLength += currentWordLength + CharWidth(' ');
                        currentWordLength = 0.f;
                        lastSpace = c;

                        if(*c == '\n') {
                            lines.push_back(text.substr(lineStart - text.c_str(), c - lineStart));
                            currentLineLength = currentWordLength = 0.f;
                            lineStart = lastSpace = c+1;
                        }
                    }
                    else if(currentWordLength >= lineWidth * longWordThresh) {
                        //word that's longer than line - have to split it
                        float hyphenLength = CharWidth('-');
                            float wordLength = hyphenLength;

                        for(const char* c2 = lastSpace; c2 < c; c2++) {
                            int cLength = CharWidth(*c2);
                            if(currentLineLength + wordLength + cLength > lineWidth) {
                                if(c2+1 == c && (currentLineLength + wordLength + cLength - hyphenLength < lineWidth))
                                    lines.push_back(text.substr(lineStart - text.c_str(), c2+1 - lineStart));
                                else
                                    lines.push_back(text.substr(lineStart - text.c_str(), c2 - lineStart) + "-");
                                currentLineLength = 0.f;
                                lineStart = c2;
                                lastSpace = c2-1;
                                currentWordLength -= wordLength - hyphenLength;
                                wordLength = cLength + hyphenLength;
                            }
                            else {
                                wordLength += cLength;
                            }
                        }

                        if(currentLineLength + currentWordLength <= lineWidth) {
                            currentLineLength += currentWordLength + CharWidth(' ');
                            currentWordLength = 0.f;
                            lastSpace = c;

                            if(*c == '\n') {
                                lines.push_back(text.substr(lineStart - text.c_str(), c - lineStart));
                                currentLineLength = currentWordLength = 0.f;
                                lineStart = lastSpace = c+1;
                            }
                        }
                    }
                    else {
                        //line can't contain this word, but word can fit onto a new line
                        lines.push_back(text.substr(lineStart - text.c_str(), lastSpace - lineStart));
                        lineStart = lastSpace+1;

                        //add the word to new line
                        currentLineLength = currentWordLength + CharWidth(' ');
                        currentWordLength = 0.f;
                        lastSpace = c;

                        if(*c == '\n') {
                            lines.push_back(text.substr(lineStart - text.c_str(), c - lineStart));
                            currentLineLength = currentWordLength = 0.f;
                            lineStart = lastSpace = c+1;
                        }
                    }
                    break;
                default:
                    currentWordLength += CharWidth(*c);
                    break;
            }

            c++;
        }

        //last line (if it doesn't end with '\n')
        if(currentLineLength > 0.f || currentWordLength > 0.f) {
            lines.push_back(text.substr(lineStart - text.c_str(), c - lineStart));
            currentLineLength = currentWordLength = 0.f;
            lineStart = lastSpace = c+1;
        }

        // ENG_LOG_INFO("ScrollText::ProcessText:");
        // for(std::string& s : lines) {
        //     ENG_LOG_INFO("{}", s);
        // }
        // ENG_LOG_INFO("===================");

        /*how:
            - each '\n' spawns new line
            - fold line when it reaches certain length
                - don't split words, fold on spaces
                - max length given by width of the scroll area - need to work with character sizes
        */
    }

    float ScrollText::CharWidth(char c) {
        CharInfo& ch = (c >= 32 && c < 128) ? font->GetChar(c) : font->GetChar(' ');
        return ch.advance.x * style->textScale;
    }

    float ScrollText::LineHeight() {
        return (style->textScale * lineGapMult * style->font->GetRowHeight()) / Window::Get().Height();
    }

    void ScrollText::ResizeLines(float lineHeight) {
        lineSize = glm::vec2(size.x * 2.f, lineHeight);
        visibleLinesCount = (lineHeight > 1e-4f) ? (size.y * 2.f) / lineHeight : 0.f;
        // ENG_LOG_TRACE("ScrollText::ResizeLines triggered");
    }

    void ScrollText::InnerRender() {
        Element::InnerRender();
        ASSERT_MSG(style->font != nullptr, "GUI element with text has to have a font assigned.");

        //recompute when font changes
        float lh = LineHeight();
        if(std::abs(lineSize.y - lh) > 1e-3f || font != style->font) {
            ENG_LOG_TRACE("ScrollText::Resize - triggered from InnerRender ({} | {})", lh, lineSize.y);
            font = style->font;
            ResizeLines(lh);
            ProcessText(text);
        }

        //element top-left corner (-lineHeight, bcs text treats it as bottom-left coord)
        glm::vec2 corner = glm::vec2(position.x - size.x, -position.y + size.y - lineSize.y);
        int startIdx = int(pos);
        int endIdx = int(pos) + visibleLinesCount;
        float off = pos - int(pos);

        // render first line + the line above (with clipping) - line above to render letters like y,q,g,...
        // glm::vec4 clr = glm::vec4(1.f, 0.f, 0.f, 1.f);
        for(int i = std::max(startIdx-1, 0); i < std::min(startIdx+1, (int)lines.size()); i++) {
            float off = pos - i;
            float y = corner.y + lineSize.y * off;
            font->RenderTextClippedTop(lines[i].c_str(), glm::vec2(corner.x, y), style->textScale, off, style->textColor, Z_INDEX_BASE - zIdx * Z_INDEX_MULT - Z_TEXT_OFFSET);
        }

        //render whole lines
        for(int i = std::max(startIdx+1, 0); i < std::min(endIdx-1, (int)lines.size()); i++) {
            float y = corner.y - lineSize.y * (i-startIdx-off);
            font->RenderText(lines[i].c_str(), glm::vec2(corner.x, y), style->textScale, style->textColor, Z_INDEX_BASE - zIdx * Z_INDEX_MULT - Z_TEXT_OFFSET);
        }

        //render last 2 lines (with clipping)
        // clr = glm::vec4(0.f, 1.f, 0.f, 1.f);
        for(int i = std::max(endIdx-1, 0); i < std::min(endIdx+1, (int)lines.size()); i++) {
            float y = corner.y - lineSize.y * (i-startIdx-off);
            font->RenderTextClippedBot(lines[i].c_str(), glm::vec2(corner.x, y), style->textScale, pos+visibleLinesCount-i, style->textColor, Z_INDEX_BASE - zIdx * Z_INDEX_MULT - Z_TEXT_OFFSET);
        }
    }

    //===== ImageButton =====

    ImageButton::ImageButton(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const StyleRef& style_, 
        const Sprite& sprite_, const glm::ivec2& idx_,
        ButtonCallbackHandler* handler_, ButtonCallbackType callback_, int buttonID_, float image_scaledown_)
        : Button(offset_, size_, zOffset_, style_, handler_, callback_, buttonID_), sprite(sprite_), idx(idx_), image_scaledown(image_scaledown_) {}

    ImageButton::ImageButton(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const StyleRef& style_,
        const Sprite& sprite_, const glm::ivec2& idx_, 
        ButtonCallbackHandler* handler_, ButtonCallbackType callback_, int buttonID_, int fireType_, float image_scaledown_)
        : Button(offset_, size_, zOffset_, style_, handler_, callback_, buttonID_, fireType_), sprite(sprite_), idx(idx_), image_scaledown(image_scaledown_) {}
    
    ImageButton::ImageButton(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const StyleRef& style_, 
        const Sprite& sprite_, const glm::ivec2& idx_, float image_scaledown_,
        ButtonCallbackHandler* handler_, ButtonCallbackType callback_, int buttonID_)
        : Button(offset_, size_, zOffset_, style_, handler_, callback_, buttonID_), sprite(sprite_), idx(idx_), image_scaledown(image_scaledown_) {}

    ImageButton::ImageButton(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const StyleRef& style_,
        const Sprite& sprite_, const glm::ivec2& idx_, float image_scaledown_,
        ButtonCallbackHandler* handler_, ButtonCallbackType callback_, int buttonID_, int fireType_)
        : Button(offset_, size_, zOffset_, style_, handler_, callback_, buttonID_, fireType_), sprite(sprite_), idx(idx_), image_scaledown(image_scaledown_) {}

    void ImageButton::Setup(const std::string& name_, const glm::ivec2& idx_, float value, bool enable) {
        name = name_;
        idx = idx_;
        if(enable)
            Enable(true);
    }

    void ImageButton::InnerRender() {
        Button::InnerRender();

        glm::vec2 sz = size * image_scaledown;
        sprite.Render(glm::vec3(position.x - sz.x, -position.y - sz.y,  Z_INDEX_BASE - zIdx * Z_INDEX_MULT - Z_TEXT_OFFSET), glm::vec2(sz * 2.f), idx.y, idx.x);
    }

    //===== ImageButtonWithBar =====

    ImageButtonWithBar::ImageButtonWithBar(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, 
        const StyleRef& style_, const StyleRef& bar_style_, const glm::vec2& borders_size_, const Sprite& sprite_, const glm::ivec2& idx_,
        ButtonCallbackHandler* handler_, ButtonCallbackType callback_, int buttonID_, float image_scaledown_)
        : ImageButton(offset_, size_, zOffset_, style_, sprite_, idx_, handler_, callback_, buttonID_, image_scaledown_), bar_style(bar_style_), borders_size(borders_size_) {}

    ImageButtonWithBar::ImageButtonWithBar(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, 
        const StyleRef& style_, const StyleRef& bar_style_, const glm::vec2& borders_size_, const Sprite& sprite_, const glm::ivec2& idx_, 
        ButtonCallbackHandler* handler_, ButtonCallbackType callback_, int buttonID_, int fireType_, float image_scaledown_)
        : ImageButton(offset_, size_, zOffset_, style_, sprite_, idx_, handler_, callback_, buttonID_, image_scaledown_), bar_style(bar_style_), borders_size(borders_size_) {}
    
    ImageButtonWithBar::ImageButtonWithBar(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, 
        const StyleRef& style_, const StyleRef& bar_style_, const glm::vec2& borders_size_, const Sprite& sprite_, const glm::ivec2& idx_, float image_scaledown_,
        ButtonCallbackHandler* handler_, ButtonCallbackType callback_, int buttonID_)
        : ImageButton(offset_, size_, zOffset_, style_, sprite_, idx_, handler_, callback_, buttonID_, image_scaledown_), bar_style(bar_style_), borders_size(borders_size_) {}

    ImageButtonWithBar::ImageButtonWithBar(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, 
        const StyleRef& style_, const StyleRef& bar_style_, const glm::vec2& borders_size_, const Sprite& sprite_, const glm::ivec2& idx_, float image_scaledown_,
        ButtonCallbackHandler* handler_, ButtonCallbackType callback_, int buttonID_, int fireType_)
        : ImageButton(offset_, size_, zOffset_, style_, sprite_, idx_, handler_, callback_, buttonID_, image_scaledown_), bar_style(bar_style_), borders_size(borders_size_) {}

    void ImageButtonWithBar::Setup(const std::string& name_, const glm::ivec2& idx_, float value_, bool enable) {
        value = value_;
        ImageButton::Setup(name_, idx_, value_, enable);
    }

    void ImageButtonWithBar::SetValue(float value_) {
        value = value_;
    }

    AABB ImageButtonWithBar::GetAABB() {
        glm::vec2 sz = glm::vec2(size.x, size.y * (1.f + HEALTH_BAR_HEIGHT_RATIO));
        return AABB(
            (position + 1.f - sz) * 0.5f,
            (position + 1.f + sz) * 0.5f
        );
    }

    float ImageButtonWithBar::BarHeightRatio() {
        return HEALTH_BAR_HEIGHT_RATIO;
    }

    void ImageButtonWithBar::InnerRender() {
        ImageButton::InnerRender();

        glm::vec2 sz = glm::vec2(size.x, size.y * HEALTH_BAR_HEIGHT_RATIO);
        glm::vec2 bs = borders_size * 2.f * sz;
        glm::vec2 pos = glm::vec2(position.x, -position.y - size.y - sz.y);

        Renderer::RenderQuad(Quad::FromCenter(glm::vec3(pos.x, pos.y + bs.y, Z_INDEX_BASE - zIdx * Z_INDEX_MULT), sz, bar_style->color, bar_style->texture));
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(pos.x - sz.x + bs.x, pos.y - sz.y + 2.f*bs.y, Z_INDEX_BASE - zIdx * Z_INDEX_MULT - Z_TEXT_OFFSET), (sz - bs) * 2.f * glm::vec2(value, 1.f), ColorFromValue(), nullptr));
    }

    glm::vec4 ImageButtonWithBar::ColorFromValue() const {
        if(value >= 0.75f)
            return glm::vec4(0.1f, 0.7f, 0.f, 1.f);
        else if(value >= 0.5f)
            return glm::vec4(0.95f, 0.8f, 0.05f, 1.f);
        else
            return glm::vec4(0.9f, 0.f, 0.f, 1.f);
    }

    //===== ValueBar =====

    ValueBar::ValueBar(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, 
        const StyleRef& bar_style_, const glm::vec2& borders_size_, const StyleRef& text_style_, const glm::vec4 filler_clr_, const std::string& text_)
        : Element(offset_, size_, zOffset_, bar_style_, nullptr), text(text_), value(1.f), text_style(text_style_), filler_clr(filler_clr_), borders_size(borders_size_) {}

    void ValueBar::SetValue(float value_) {
        value = value_;
    }

    void ValueBar::SetText(const std::string& text_) {
        text = text_;
    }

    void ValueBar::Setup(float value_, const std::string& text_, bool enable) {
        value = value_;
        text = text_;
        if(enable)
            Enable(true);
    }

    void ValueBar::InnerRender() {
        Element::InnerRender();

        //render filler bar
        glm::vec2 bs = borders_size * 2.f * size;
        Renderer::RenderQuad(Quad::FromCorner(glm::vec3(position.x - size.x + bs.x, -position.y - size.y + bs.y, Z_INDEX_BASE - zIdx * Z_INDEX_MULT - Z_TEXT_OFFSET), (size - bs) * 2.f * glm::vec2(value, 1.f), filler_clr, nullptr));

        //render the text
        text_style->font->RenderTextCentered(text.c_str(), glm::vec2(position.x, -position.y), text_style->textScale, text_style->textColor, Z_INDEX_BASE - zIdx * Z_INDEX_MULT - 2.f*Z_TEXT_OFFSET);
    }

    //===== KeyValue =====

    KeyValue::KeyValue(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, const StyleRef& style_, const std::string& text_)
        : Element(offset_, size_, zOffset_, style_, nullptr), text(text_) {
        
        sep_pos = text.find(':');
        sep_pos = (sep_pos != std::string::npos) ? (sep_pos+1) : text.size();
    }

    void KeyValue::Setup(const std::string& text_, bool enable) {
        text = text_;
        sep_pos = text.find(':');
        sep_pos = (sep_pos != std::string::npos) ? (sep_pos+1) : text.size();

        if(enable)
            Enable(true);
    }
    
    void KeyValue::InnerRender() {
        Element::InnerRender();
        ASSERT_MSG(style->font != nullptr, "GUI element with text has to have a font assigned.");
        style->font->RenderTextKeyValue(text.c_str(), sep_pos, glm::vec2(position.x, -position.y), style->textScale, style->textColor, Z_INDEX_BASE - zIdx * Z_INDEX_MULT - Z_TEXT_OFFSET);
    }

    //===== ImageButtonGrid =====

    ImageButtonGrid::ImageButtonGrid(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, 
            const StyleRef& btn_style_, const Sprite& sprite_, int rows, int cols,  const glm::vec2& btn_size,
            ButtonCallbackHandler* handler_, ButtonCallbackType callback_)
        : Element(offset_, size_, zOffset_, Style::Default(), nullptr), handler(handler_), callback(callback_), grid_size(glm::ivec2(cols, rows)) {

        glm::vec2 btn_step = 1.f / glm::vec2(cols, rows);

        for(int y = 0; y < rows; y++) {
            for(int x = 0; x < cols; x++) {
                int i = x+y*cols;
                ImageButton* btn = new ImageButton((btn_step * glm::vec2(x,y)) * 2.f - 1.f + btn_size, btn_size, 1.f, btn_style_, sprite_, glm::ivec2(0, 0), 0.9f, handler, callback, i);
                // ImageButton* btn = new ImageButton(glm::vec2(1.f, 0.f), btn_size, 0.f, btn_style_, sprite_, glm::ivec2(0, 0), 0.9f, handler, callback, i);
                AddChild(btn, true);
                btns.push_back(btn);
            }
        }
    }

    ImageButtonGrid::ImageButtonGrid(const glm::vec2& offset_, const glm::vec2& size_, float zOffset_, 
            const StyleRef& btn_style_, const StyleRef& bar_style_, const glm::vec2& bar_borders_size_, const Sprite& sprite_, int rows, int cols,  const glm::vec2& btn_size,
            ButtonCallbackHandler* handler_, ButtonCallbackType callback_)
        : Element(offset_, size_, zOffset_, Style::Default(), nullptr), handler(handler_), callback(callback_), grid_size(glm::ivec2(cols, rows)) {

        glm::vec2 btn_step = 1.f / glm::vec2(cols, rows);

        for(int y = 0; y < rows; y++) {
            for(int x = 0; x < cols; x++) {
                int i = x+y*cols;
                ImageButton* btn = new ImageButtonWithBar((btn_step * glm::vec2(x,y)) * 2.f - 1.f + btn_size, btn_size, 1.f, btn_style_, bar_style_, bar_borders_size_, sprite_, glm::ivec2(0, 0), 0.9f, handler, callback, i);
                // ImageButton* btn = new ImageButton(glm::vec2(1.f, 0.f), btn_size, 0.f, btn_style_, sprite_, glm::ivec2(0, 0), 0.9f, handler, callback, i);
                AddChild(btn, true);
                btns.push_back(btn);
            }
        }
    }

}//namespace eng::GUI
