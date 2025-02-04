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
        float minDistance = 15.f;
        float maxDistance = 25.f;
        //TODO: maybe move these back to audio.cpp - probably shouldn't be modifiable

        float outOfScreenRolloff = 0.7f;
    public:
        void UpdateSounds(float master, float digital, float music, bool save_changes = true);
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

    bool CameraPanning();

    void UpdateSpeeds(float game, float mouse, float keys, bool save_changes = true);
    void UpdatePreferences(bool fog, bool save_changes = true);

    void UpdateCameraPanning(bool enabled);
    void ToggleCameraPanning();

    bool Hack_NoPrices();
    bool Hack_NoPopLimit();
    
    bool Hack_MapReveal();
    void Hack_MapReveal_Clear();

    namespace Saves {

        std::string FullPath(const std::string& name, bool append_extension = true);
        std::string CustomGames_FullPath(const std::string& name, bool append_extension = true);

        std::string DirPath();
        std::string CustomGames_DirPath();

        std::vector<std::string> Scan(bool extract_names = true);
        std::vector<std::string> ScanCustomGames(bool extract_names = true);

        bool Delete(const std::string& filename);

    }//namespace Saves

}//namespace eng
