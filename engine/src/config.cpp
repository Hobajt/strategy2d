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
        bool camera_panning = true;
        bool hack_map_reveal = false;
        bool hack_map_reveal_flag = false;
        bool hack_no_prices = false;
        bool hack_no_pop_limit = false;
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
        ImGui::Text("Volume - spatial");
        if(ImGui::SliderFloat("Min distance", &data.audio.minDistance, 0.f, 100.f, "%.1f")) {
        }

        if(ImGui::SliderFloat("Max distance", &data.audio.maxDistance, 0.f, 100.f, "%.1f")) {
        }

        if(ImGui::SliderFloat("Rolloff", &data.audio.outOfScreenRolloff, 0.f, 1.f, "%.1f")) {
        }

        ImGui::Separator();
        ImGui::Text("Preferences");

        ImGui::Checkbox("Fog of War", &data.fog_of_war);

        ImGui::Separator();
        ImGui::Text("Hacks");

        ImGui::Checkbox("No Prices", &data.hack_no_prices);
        ImGui::SameLine();
        ImGui::Checkbox("No Pop Limit", &data.hack_no_pop_limit);
        ImGui::SameLine();
        if(ImGui::Checkbox("Reveal map", &data.hack_map_reveal)) {
            data.hack_map_reveal = true;
        }

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

    void AudioConfig::UpdateSounds(float master, float digital, float music, bool save_changes) {
        volume_digital = digital;
        volume_master = master;
        volume_music = music;
        if(save_changes)
            SaveChanges();
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

    bool CameraPanning() {
        return data.camera_panning;
    }

    void UpdateSpeeds(float game, float mouse, float keys, bool save_changes) {
        data.game_speed = game;
        data.map_mouse_speed = mouse;
        data.map_key_speed = keys;
        if(save_changes)
            SaveChanges();
    }

    void UpdatePreferences(bool fog, bool save_changes) {
        data.fog_of_war = fog;
        if(save_changes)
            SaveChanges();
    }

    void UpdateCameraPanning(bool enabled) {
        data.camera_panning = enabled;
    }

    void ToggleCameraPanning() {
        data.camera_panning = !data.camera_panning;
    }

    bool Hack_NoPrices() {
        return data.hack_no_prices;
    }

    bool Hack_NoPopLimit() {
        return data.hack_no_pop_limit;
    }

    bool Hack_MapReveal() {
        bool res = data.hack_map_reveal && !data.hack_map_reveal_flag;
        data.hack_map_reveal_flag |= data.hack_map_reveal;
        return res;
    }

    void Hack_MapReveal_Clear() {
        data.hack_map_reveal = false;
        data.hack_map_reveal_flag = false;
    }

    namespace Saves {

        bool string_ends_with(const std::string& fullString, const std::string& suffix) {
            if (fullString.length() >= suffix.length()) {
                return (fullString.compare(fullString.length() - suffix.length(), suffix.length(), suffix) == 0);
            }
            return false;
        }

        std::string FullPath(const std::string& name, bool append_extension) {
            std::string res = DirPath() + name;
            if(append_extension && !string_ends_with(name, ".json")) {
                res += ".json";
            }
            return res;
        }

        std::string CustomGames_FullPath(const std::string& name, bool append_extension) {
            std::string res = CustomGames_DirPath() + name;
            if(append_extension && !string_ends_with(name, ".json")) {
                res += ".json";
            }
            return res;
        }

        std::string DirPath() {
            return "res/saves/";
        }

        std::string CustomGames_DirPath() {
            return "res/maps/";
        }

        std::vector<std::string> DirectoryScan(const std::string& dir, bool extract_names = false) {
            std::vector<std::string> files;
            try {
                for (const auto& entry : std::filesystem::directory_iterator(dir)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".json") {
                        if(extract_names)
                            files.push_back(entry.path().stem().string());
                        else
                            files.push_back(entry.path().filename().string());
                    }
                }
                } catch (const std::filesystem::filesystem_error& e) {
                    ENG_LOG_ERROR("Failed to scan directory '{}' for files.", dir);
                    return {};
                }
                
            return files;
        }

        std::vector<std::string> Scan(bool extract_names) {
            return DirectoryScan(DirPath(), extract_names);
        }

        std::vector<std::string> ScanCustomGames(bool extract_names) {
            return DirectoryScan(CustomGames_DirPath(), extract_names);
        }

        bool Delete(const std::string& filename) {
            std::string filepath = FullPath(filename, true);

            if(!std::filesystem::exists(filepath)) {
                ENG_LOG_WARN("[R] Config::Saves::Delete - failed to delete '{}' - file not found.", filepath.c_str());
                return false;
            }

            try {
                std::filesystem::remove(filepath);
            }
            catch(std::filesystem::filesystem_error&) {
                ENG_LOG_WARN("[R] Config::Saves::Delete - failed to delete '{}' - filesystem error.", filepath.c_str());
                return false;
            }

            ENG_LOG_TRACE("[R] Config::Saves::Delete - successfully deleted savefile '{}'.", filepath.c_str());
            return true;
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

        if(config.count("audio_dmin"))          data.audio.minDistance = config.at("audio_dmin");
        if(config.count("audio_dmax"))          data.audio.maxDistance = config.at("audio_dmax");
        if(config.count("audio_rolloff"))       data.audio.outOfScreenRolloff = config.at("audio_rolloff");

        if(config.count("fog_of_war"))      data.fog_of_war = config.at("fog_of_war");
        if(config.count("camera_panning"))  data.camera_panning = config.at("camera_panning");

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

        out["audio_dmin"] = data.audio.minDistance;
        out["audio_dmax"] = data.audio.maxDistance;
        out["audio_rolloff"] = data.audio.outOfScreenRolloff;

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
