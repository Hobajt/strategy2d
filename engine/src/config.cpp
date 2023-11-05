#include "engine/game/config.h"

#include "engine/core/audio.h"

#include "engine/utils/dbg_gui.h"
#include "engine/utils/json.h"
#include "engine/utils/utils.h"

#define CONFIG_FILEPATH "res/game.config"

namespace eng::Config {

    struct ConfigData {
        AudioConfig audio = {};
        float game_speed = 1.f;

        float map_mouse_speed = 1.f;
        float map_key_speed = 1.f;

        bool fog_of_war = true;
    };
    static ConfigData data = {};

    ConfigData Load(const char* filepath);
    bool Save(const char* filepath);

    void ActivateValues();

    //==================

    void Reload() {
        try {
            data = Load(CONFIG_FILEPATH);
            ActivateValues();
        }
        catch(std::exception&) {
            ENG_LOG_WARN("Failed to load game config, using deafult values instead.");
            data = {};
        }
    }

    void SaveChanges() {
        if(!Save(CONFIG_FILEPATH)) {
            ENG_LOG_WARN("Game config failed to save.");
        }
    }

    void DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
        ImGui::Begin("Config");

        ImGui::Separator();
        ImGui::Text("Speeds");
        if(ImGui::SliderFloat("Game speed", &data.game_speed, 0.6f, 6.f, "%.1f", ImGuiSliderFlags_Logarithmic)) {
            //....;
        }
        if(ImGui::SliderFloat("Mouse scroll", &data.map_mouse_speed, 0.1f, 10.f, "%.1f", ImGuiSliderFlags_Logarithmic)) {
            //....;
        }
        if(ImGui::SliderFloat("Key scroll", &data.map_key_speed, 0.1f, 10.f, "%.1f", ImGuiSliderFlags_Logarithmic)) {
            //....;
        }

        ImGui::Separator();
        ImGui::Text("Sound Config");

        if(ImGui::Checkbox("Enabled", &data.audio.enabled)) {
            Audio::Enabled(data.audio.enabled);
        }
        ImGui::Separator();

        ImGui::Text("Volume");
        if(ImGui::SliderFloat("Master", &data.audio.volume_master, 0.f, 100.f, "%.1f")) {
            Audio::SetVolume_Master(data.audio.volume_master);
        }

        if(ImGui::SliderFloat("Music", &data.audio.volume_music, 0.f, 100.f, "%.1f")) {
            Audio::SetVolume_Music(data.audio.volume_music);
        }

        if(ImGui::SliderFloat("Digital", &data.audio.volume_digital, 0.f, 100.f, "%.1f")) {
            Audio::SetVolume_Digital(data.audio.volume_digital);
        }

        ImGui::Separator();
        ImGui::Text("Preferences");

        ImGui::Checkbox("Fog of War", &data.fog_of_war);

        ImGui::Separator();
        if(ImGui::Button("Reload config")) {
            Reload();
        }
        ImGui::SameLine();
        if(ImGui::Button("Save changes")) {
            SaveChanges();
        }

        ImGui::End();
#endif
    }

    AudioConfig& Audio() {
        return data.audio;
    }

    float GameSpeed() {
        return data.game_speed;
    }

    float Map_MouseSpeed() {
        return data.map_mouse_speed;
    }

    float Map_KeySpeed() {
        return data.map_key_speed;
    }

    bool FogOfWar() {
        return data.fog_of_war;
    }

    namespace Saves {

        std::string DirPath() {
            return "NO_PATH_JUST_YET/";
        }

        std::vector<std::string> Scan() {
            return { "File1", "File2", "File3" };
            // return { "File1", "File2", "File3", "File1", "File2", "File3", "File1", "File2", "File3", "File1", "File2", "File3" };
        }

        std::string FullPath(const std::string& name) {
            return DirPath() + name;
        }

    }//namespace Saves

    //============================

    ConfigData Load(const char* filepath) {
        ConfigData data = {};

        using json = nlohmann::json;

        auto config = json::parse(ReadFile(filepath));

        if(config.count("audio_enabled"))   data.audio.enabled = config.at("audio_enabled");
        if(config.count("volume_master"))   data.audio.volume_master = config.at("volume_master");
        if(config.count("volume_music"))    data.audio.volume_music = config.at("volume_music");
        if(config.count("volume_digital"))  data.audio.volume_digital = config.at("volume_digital");

        if(config.count("game_speed"))          data.game_speed = config.at("game_speed");
        if(config.count("map_mouse_speed"))     data.map_mouse_speed = config.at("map_mouse_speed");
        if(config.count("map_key_speed"))       data.map_key_speed = config.at("map_key_speed");

        if(config.count("fog_of_war"))      data.fog_of_war = config.at("fog_of_war");

        return data;
    }

    bool Save(const char* filepath) {
        using json = nlohmann::json;

        json out = {};

        out["audio_enabled"] = data.audio.enabled;
        out["volume_master"] = data.audio.volume_master;
        out["volume_music"] = data.audio.volume_music;
        out["volume_digital"] = data.audio.volume_digital;

        out["game_speed"] = data.game_speed;
        out["map_mouse_speed"] = data.map_mouse_speed;
        out["map_key_speed"] = data.map_key_speed;

        out["fog_of_war"] = data.fog_of_war;

        return TryWriteFile(filepath, out.dump());
    }

    void ActivateValues() {
        Audio::Enabled(data.audio.enabled);
        Audio::SetVolume_Master(data.audio.volume_master);
        Audio::SetVolume_Music(data.audio.volume_music);
        Audio::SetVolume_Digital(data.audio.volume_digital);
    }

}//namespace eng::Config
