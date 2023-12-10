#pragma once

#include <memory>
#include <vector>
#include <tuple>
#include <unordered_map>

#include "engine/game/faction.h"
#include "engine/core/gui.h"
#include "engine/core/gui_action_buttons.h"
#include "engine/game/command.h"

namespace eng {
    
#define STD_MSG_TIME 5.f

    struct PlayerSelection;
    class PlayerFactionController;

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

            bool progress_bar_flag = false;
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

            bool BuildingActionCancelled() const { return buildingAction_inProgress && !IsAtCancelPage(); }
        private:
            void ResetPage(int idx);
        private:
            ImageButtonGrid* btns = nullptr;
            std::array<PageData, 4> pages;
            int page = 0;

            int clicked_btn_idx = -1;
            bool buildingAction_inProgress = false;
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

        //===== GUI::PopupMessage =====

        class PopupMessage : public TextLabel {
        public:
            PopupMessage() = default;
            PopupMessage(const glm::vec2& offset, const glm::vec2& size, float zOffset, const StyleRef& style);

            virtual void OnHover() override {}
            virtual void OnHold() override {}
            virtual void OnHighlight() override {}

            void DisplayMessage(const std::string& msg, float duration = STD_MSG_TIME);
            void Update();
        private:
            float time_limit = 0.f;
        };

        //===== GUI::IngameMenu =====

        namespace IngameMenuTab { enum { MAIN, HELP, OPTIONS, OPTIONS_SOUND, OPTIONS_SPEED, OPTIONS_PREFERENCES, SAVE, LOAD, OBJECTIVES, END_SCENARIO, CONFIRM_SCREEN }; }
        namespace MenuAction { enum { NONE, SAVE, LOAD, DELETE, QUIT_MISSION }; }

        class IngameMenu : public ButtonCallbackHandler {
        public:
            IngameMenu() = default;
            IngameMenu(const glm::vec2& offset, const glm::vec2& size, float zOffset, PlayerFactionController* ctrl, 
                const StyleRef& bg_style, const StyleRef& btn_style, const StyleRef& text_style);

            //copy disabled
            IngameMenu(const IngameMenu&) = delete;
            IngameMenu& operator=(const IngameMenu&) = delete;

            //move enabled
            IngameMenu(IngameMenu&&) noexcept;
            IngameMenu& operator=(IngameMenu&&) noexcept;
                

            void Render();
            void Update(Level& level, PlayerFactionController& ctrl, SelectionHandler& gui_handler);

            void OnKeyPressed(int keycode, int modifiers, bool single_press);

            void OpenTab(int tabID);
        private:
            void Move(IngameMenu&&) noexcept;

            void EnableDeleteButton(bool enabled);
            void SetupConfirmScreen(const std::string& label, const std::string& btn, const glm::ivec2& highlightIdx, int keycode);
            void EndGame();
        private:
            std::unordered_map<int, GUI::Menu> menus;
            int active_menu = IngameMenuTab::MAIN;
            int action = MenuAction::NONE;
            PlayerFactionController* ctrl = nullptr;

            ScrollText* list_objectives = nullptr;
            ScrollMenu* list_load = nullptr;
            ScrollMenu* list_save = nullptr;
            TextInput* textInput = nullptr;
            TextButton* delet_btn = nullptr;
            std::array<StyleRef, 2> btn_styles;

            ValueSlider* sound_master = nullptr;
            ValueSlider* sound_digital = nullptr;
            ValueSlider* sound_music = nullptr;

            ValueSlider* speed_game = nullptr;
            ValueSlider* speed_mouse = nullptr;
            ValueSlider* speed_keys = nullptr;

            Radio* fog_of_war = nullptr;

            ScrollText* confirm_label = nullptr;
            TextButton* confirm_btn = nullptr;
            int confirm_keycode = 0;
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

        bool IssueTargetedCommand(Level& level, const glm::ivec2& target_pos, const ObjectID& target_id, const glm::ivec2& cmd_data, GUI::PopupMessage& msg_bar);
        void IssueAdaptiveCommand(Level& level, const glm::ivec2& target_pos, const ObjectID& target_id);
        void IssueTargetlessCommand(Level& level, const glm::ivec2& cmd_data);

        void CancelBuildingAction(Level& level);

        void DBG_GUI();
    private:
        bool AlreadySelected(const ObjectID& target_id, int currentSelectionSize);
    };

    //===== OcclusionMask =====

    class OcclusionMask {
    public:
        OcclusionMask() = default;
        OcclusionMask(const std::vector<uint32_t>& occlusionData, const glm::ivec2& size);
        ~OcclusionMask();

        OcclusionMask(const OcclusionMask&) = delete;
        OcclusionMask& operator=(const OcclusionMask&) = delete;

        OcclusionMask(OcclusionMask&&) noexcept;
        OcclusionMask& operator=(OcclusionMask&&) noexcept;

        int& operator()(const glm::ivec2& coords);
        const int& operator()(const glm::ivec2& coords) const;
        int& operator()(int y, int x);
        const int& operator()(int y, int x) const;

        std::vector<uint32_t> Export();
        void Import(const std::vector<uint32_t>& data);

        glm::ivec2 Size() const { return size; }
    private:
        glm::ivec2 size;
        int* data = nullptr;
    };

    //===== MapView =====

    //Manages a texture with map view.
    class MapView {
        using rgba = glm::u8vec4;
    public:
        MapView() = default;
        MapView(const glm::ivec2& size, int scale = 4);
        
        void Update(const Map& map,  bool forceRedraw = false);

        TextureRef GetTexture() const { return tex; }
    private:
        rgba* Redraw(const Map& map) const;
    private:
        int redraw_interval = 30;
        TextureRef tex = nullptr;
        int counter = 0;
        int scale = 1;
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

            virtual void ChangeLevel(const std::string& filename) {}
            virtual void QuitMission() {}

            void LinkController(const PlayerFactionControllerRef& ctrl);
        protected:
            void SignalKeyPress(int keycode, int modifiers, bool single_press);
        private:
            PlayerFactionControllerRef playerController = nullptr;
        };
    public:
        PlayerFactionController(FactionsFile::FactionEntry&& entry, const glm::ivec2& mapSize);

        const OcclusionMask& Occlusion() const { return occlusion; }

        //Render player's GUI.
        void Render();

        virtual void Update(Level& level) override;

        void Update_Paused(Level& level);

        void SwitchMenu(bool active);
        void ChangeLevel(const std::string& filename);
        void QuitMission();

        void UpdateOcclusions(Level& level);
    private:
        void OnKeyPressed(int keycode, int modifiers, bool single_press);

        void UpdateState(Level& level, int& cursor_idx);

        void ResolveActionButtonClick();

        void CameraPanning(const glm::vec2& pos);
        bool CursorInGameView(const glm::vec2& pos) const;
        bool CursorInMapView(const glm::vec2& pos) const;
        void BackToIdle();
        glm::ivec2 MapView2MapCoords(const glm::vec2& pos, const glm::ivec2& mapSize);

        void RenderSelectionRectangle();
        void RenderBuildingViz(const Map& map, const glm::ivec2& worker_pos);
        void RenderMapView();

        void InitializeGUI();
        
        virtual void Inner_DBG_GUI() override;

        virtual std::vector<uint32_t> ExportOcclusion() override;
    private:
        GUIRequestHandler* handler = nullptr;

        GUI::Menu game_panel;
        GUI::SelectionHandler gui_handler;
        GUI::TextLabel text_prompt;
        GUI::PopupMessage msg_bar;
        GUI::IngameMenu menu;

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

        OcclusionMask occlusion;
        MapView mapview;
    };

}//namespace eng
