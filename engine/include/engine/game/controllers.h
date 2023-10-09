#pragma once

#include <memory>
#include <vector>

#include "engine/game/faction.h"
#include "engine/core/gui.h"

namespace eng {

    //can setup enum for the IDs as well (PLAYER_LOCAL, PLAYER_REMOTE1, PLAYER_REMOTEn, AI_EASY, ...)

    namespace FactionControllerID {
        enum { INVALID = 0, LOCAL_PLAYER, };
    }

    struct PlayerSelection {
        std::array<ObjectID, 9> selection;
        int selected_count = 0;
        int selection_type = 0;
    public:
        //Picks new objects from selected area in the map.
        void Select(Level& level, const glm::vec2& start, const glm::vec2& end, int playerFactionID);
    };

    //===== GUI::SelectionTab =====

    namespace GUI {
        class SelectionTab : public Element {
        public:
            SelectionTab(const glm::vec2& offset, const glm::vec2& size, float zOffset, 
                const StyleRef& borders_style, const StyleRef& text_style, const StyleRef& text_style_small,
                const StyleRef& mana_bar_style, const glm::vec2& mana_borders_size, const StyleRef& progress_bar_style, const glm::vec2& progress_borders_size, const StyleRef& passive_btn_style,
                const StyleRef& btn_style, const StyleRef& bar_style, const glm::vec2& bar_borders_size, const Sprite& sprite, ButtonCallbackHandler* handler, ButtonCallbackType callback);
            
            void Update(const Level& level, const PlayerSelection& selection);
        private:
            ImageButtonGrid* btns;
            Element* borders;

            TextLabel* name;
            TextLabel* level;
            TextLabel* health;
            std::array<KeyValue*, 6> stats;
            TextLabel* headline;
            ValueBar* mana_bar;

            ValueBar* production_bar;
            ImageButton* production_icon;
            TextLabel* production_headline;
        };
    }

    //===== PlayerFactionController =====

    class PlayerFactionController;
    using PlayerFactionControllerRef = std::shared_ptr<PlayerFactionController>;

    namespace PlayerControllerState { enum { IDLE, SELECTION, CAMERA_CENTERING }; }

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

        bool CursorInGameView(const glm::vec2& pos) const;
        bool CursorInMapView(const glm::vec2& pos) const;

        void RenderSelectionRectangle();

        void InitializeGUI();
    private:
        GUIRequestHandler* handler = nullptr;

        GUI::Menu game_panel;
        GUI::Menu menu_panel;
        GUI::SelectionHandler gui_handler;
        GUI::TextLabel text_prompt;

        GUI::ImageButtonGrid* actionButtons;
        GUI::SelectionTab* selectionTab;

        PlayerSelection selection;

        bool is_menu_active = false;
        int state = PlayerControllerState::IDLE;

        glm::vec2 coords_start;
        glm::vec2 coords_end;
    };

}//namespace eng
