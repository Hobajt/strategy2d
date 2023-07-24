#pragma once

#include <memory>
#include <string>

#include "engine/utils/mathdefs.h"

namespace eng {

   namespace Audio {

        void Initialize();
        void Release();

        //Starts playing given sound effect (takes in it's path). No spatialization.
        bool Play(const std::string& path);

        //Starts playing given sound effect (takes in it's path). Attenuates the sound based on distance from listener.
        bool Play(const std::string& path, const glm::vec2& position);

        //non-spatial, plays in loop until replaced by different music
        bool PlayMusic(const std::string& path);

        void UpdateListenerPosition(const glm::vec2& position);

        void Enabled(bool enabled);

        void DBG_GUI();

   }//namespace Audio

}//namespace eng
