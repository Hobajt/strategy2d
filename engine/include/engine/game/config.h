#pragma once

#include <string>
#include <vector>

namespace eng::Config {

    struct AudioConfig {
        bool enabled = true;
        
        float volume_master = 100.f;
        float volume_digital = 100.f;
        float volume_music = 100.f;

        //TODO: setup attenuation distances so that sound grows weaker when it goes out of screen
        float minDistance = 10.f;
        float maxDistance = 20.f;
        //TODO: maybe move these back to audio.cpp - probably shouldn't be modifiable

        float outOfScreenRolloff = 0.85f;
    };

    //==========
    
    void Reload();
    void SaveChanges();

    void DBG_GUI();

    AudioConfig& Audio();

    float GameSpeed();

    float Map_MouseSpeed();
    float Map_KeySpeed();
    bool FogOfWar();

    namespace Saves {

        std::string DirPath();
        std::vector<std::string> Scan();
        std::string FullPath(const std::string& name);

    }//namespace Saves

}//namespace eng
