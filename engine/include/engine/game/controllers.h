#pragma once

#include <memory>

#include "engine/game/faction.h"
#include "engine/core/gui.h"

namespace eng {

    //can setup enum for the IDs as well (PLAYER_LOCAL, PLAYER_REMOTE1, PLAYER_REMOTEn, AI_EASY, ...)

    namespace FactionControllerID {
        enum { INVALID = 0, LOCAL_PLAYER, };
    }

    //===== PlayerFactionController =====

    class PlayerFactionController : public FactionController, public GUI::ButtonCallbackHandler {
    public:
        //Class, where the controller dispatches it's requests from the GUI.
        class GUIRequestHandler {
        public:
            virtual void PauseRequest(bool pause) {}
            virtual void PauseToggleRequest() {}
        };
    public:
        PlayerFactionController(FactionsFile::FactionEntry&& entry);

        //Render player's GUI.
        void Render();

        virtual void Update() override;

        void SwitchMenu(bool active);
    private:
        GUIRequestHandler* handler = nullptr;

        GUI::Menu game_panel;
        GUI::Menu menu_panel;
        GUI::SelectionHandler gui_handler;

        std::array<ObjectID, 6> selection;
        int selected_count = 0;

        bool is_menu_active = false;
    };
    using PlayerFactionControllerRef = std::shared_ptr<PlayerFactionController>;

}//namespace eng
