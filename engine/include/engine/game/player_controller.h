#pragma once

#include <memory>
#include <vector>
#include <tuple>

#include "engine/game/faction.h"
#include "engine/core/gui.h"
#include "engine/core/gui_action_buttons.h"
#include "engine/game/command.h"

namespace eng {

    //can setup enum for the IDs as well (PLAYER_LOCAL, PLAYER_REMOTE1, PLAYER_REMOTEn, AI_EASY, ...)

    namespace FactionControllerID {
        enum { INVALID = 0, LOCAL_PLAYER, };
    }

    struct PlayerSelection;

    namespace GUI {

        //===== GUI::SelectionTab =====

        class SelectionTab : public Element {
        public:
            SelectionTab(const glm::vec2& offset, const glm::vec2& size, float zOffset, 
                const StyleRef& borders_style, const StyleRef& text_style, const StyleRef& text_style_small,
                const StyleRef& mana_bar_style, const glm::vec2& mana_borders_size, const StyleRef& progress_bar_style, const glm::vec2& progress_borders_size, const StyleRef& passive_btn_style,
                const StyleRef& btn_style, const StyleRef& bar_style, const glm::vec2& bar_borders_size, const Sprite& sprite, ButtonCallbackHandler* handler, ButtonCallbackType callback);

            //Update when selection changes.            
            void Update(Level& level, const PlayerSelection& selection);
            //Values update, no real GUI changes tho.
            void ValuesUpdate(Level& level, const PlayerSelection& selection);
        private:
            //Reverts elements into their default state (as if nothing is selected).
            void Reset();
        private:
            ImageButtonGrid* btns;
            Element* borders;

            TextLabel* name;
            TextLabel* level_text;
            TextLabel* health;
            std::array<KeyValue*, 6> stats;
            TextLabel* headline;
            ValueBar* mana_bar;

            ValueBar* production_bar;
            ImageButton* production_icon;
        };

        //===== GUI::ActionButtons =====

        class ActionButtons {
            using PageData = std::array<ActionButtonDescription, 9>;
        public:
            ActionButtons() = default;
            ActionButtons(ImageButtonGrid* btns);

            void Update(Level& level, const PlayerSelection& selection);
            void ValuesUpdate(Level& level, const PlayerSelection& selection);

            ImageButtonGrid* Buttons() const { return btns; }
            const ActionButtonDescription& ButtonData(int idx) const { return pages[page][idx]; }

            void DispatchHotkey(char hotkey);
            void ClearClick() { clicked_btn_idx = -1; }
            int ClickIdx() const { return clicked_btn_idx; }
            void SetClickIdx(int idx) { clicked_btn_idx = idx; }

            void ChangePage(int idx);

            void ShowCancelPage() { ChangePage(3); }
            bool IsAtCancelPage() const { return page == 3; }
        private:
            void ResetPage(int idx);
        private:
            ImageButtonGrid* btns = nullptr;
            std::array<PageData, 4> pages;
            int page = 0;

            int clicked_btn_idx = -1;
        };

        //===== GUI::ResourceBar =====

        class ResourceBar : public Element {
        public:
            ResourceBar() = default;
            ResourceBar(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style, const Sprite& sprite, bool alt = false);

            void Update(const glm::ivec3& resources, const glm::ivec2& population);
            void UpdateAlt(const glm::ivec4& price);
            void ClearAlt();

            static glm::ivec2 IconIdx(int resource_idx);

            virtual void OnHover() override {}
            virtual void OnHold() override {}
            virtual void OnHighlight() override {}
        private:
            std::array<ImageAndLabel*, 4> text;
            bool alt;
        };
    }

    //===== PlayerSelection =====

    namespace SelectionType { enum { ENEMY_BUILDING = 0, ENEMY_UNIT, PLAYER_BUILDING, PLAYER_UNIT }; }

    struct PlayerSelection {
        using SelectionData = std::array<ObjectID, 9>;
        using SelectionGroup = std::tuple<SelectionData, int, int>;
    public:
        SelectionData selection;
        std::array<std::pair<glm::vec2, glm::vec2>, 9> location;
        int selected_count = 0;
        int selection_type = 0;
        int clr_idx = 0;

        std::array<SelectionGroup, 9> groups;

        bool update_flag = false;

        int group_update_flag = 0;
        int group_update_idx = 0;
    public:
        //Picks new objects from selected area in the map.
        void Select(Level& level, const glm::vec2& start, const glm::vec2& end, int playerFactionID);

        void SelectionGroupsHotkey(int keycode, int modifiers);

        void SelectFromSelection(int id);

        void Render();
        void Update(Level& level, GUI::SelectionTab* selectionTab, GUI::ActionButtons* actionButtons, int playerFactionID);
        void GroupsUpdate(Level& level);

        int ObjectSelectionType(const ObjectID& id, int factionID, int playerFactionID);

        void IssueTargetedCommand(Level& level, const glm::ivec2& target_pos, const ObjectID& target_id, const glm::ivec2& cmd_data);
        void IssueAdaptiveCommand(Level& level, const glm::ivec2& target_pos, const ObjectID& target_id);
        void IssueTargetlessCommand(Level& level, const glm::ivec2& cmd_data);
    };

    //===== PlayerFactionController =====

    class PlayerFactionController;
    using PlayerFactionControllerRef = std::shared_ptr<PlayerFactionController>;

    namespace PlayerControllerState { enum { IDLE, OBJECT_SELECTION, CAMERA_CENTERING, COMMAND_TARGET_SELECTION }; }

    class PlayerFactionController : public FactionController, public GUI::ButtonCallbackHandler {
    public:
        //Class, where the controller dispatches it's requests from the GUI.
        class GUIRequestHandler {
        public:
            virtual void PauseRequest(bool pause) {}
            virtual void PauseToggleRequest() {}

            void LinkController(const PlayerFactionControllerRef& ctrl);
        protected:
            void SignalKeyPress(int keycode, int modifiers);
        private:
            PlayerFactionControllerRef playerController = nullptr;
        };
    public:
        PlayerFactionController(FactionsFile::FactionEntry&& entry);

        //Render player's GUI.
        void Render();

        virtual void Update(Level& level) override;

        void SwitchMenu(bool active);
    private:
        void OnKeyPressed(int keycode, int modifiers);

        void ResolveActionButtonClick();

        bool CursorInGameView(const glm::vec2& pos) const;
        bool CursorInMapView(const glm::vec2& pos) const;
        void BackToIdle();

        void RenderSelectionRectangle();
        void RenderBuildingViz(const Map& map, const glm::ivec2& worker_pos);

        void InitializeGUI();
        
        virtual void Inner_DBG_GUI() override;
    private:
        GUIRequestHandler* handler = nullptr;

        GUI::Menu game_panel;
        GUI::Menu menu_panel;
        GUI::SelectionHandler gui_handler;
        GUI::TextLabel text_prompt;

        GUI::ActionButtons actionButtons;
        GUI::SelectionTab* selectionTab;
        GUI::ResourceBar resources;
        GUI::ResourceBar price;

        PlayerSelection selection;
        TextureRef shadows;

        bool is_menu_active = false;
        int state = PlayerControllerState::IDLE;

        glm::vec2 coords_start;
        glm::vec2 coords_end;
        glm::ivec2 command_data;
        bool nontarget_cmd_issued = false;
    };

}//namespace eng
