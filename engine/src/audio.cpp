#include "engine/core/audio.h"

#include <exception>
#include <vector>
#include <deque>

#include "engine/utils/log.h"
#include "engine/utils/dbg_gui.h"

#include <miniaudio.h>

const char* ma_stringify_error(int err_code);

namespace eng {


    //===== Audio =====

    namespace Audio {

        ma_sound* FetchFreeSoundObject();

        struct InternalAudioData {
            ma_engine engine;

            bool enabled = true;

            //TODO: initialize volume values from some kind of persistent storage (.config file?)
            float volume_master = 100.f;
            float volume_digital = 100.f;
            float volume_music = 100.f;

            //TODO: setup attenuation distances so that sound grows weaker when it goes out of screen
            float minDistance = 10.f;
            float maxDistance = 20.f;

            float outOfScreenRolloff = 0.85f;

            ma_sound_group digital_group;
            ma_sound_group music_group;

            ma_sound music;

            std::vector<ma_sound> preloadedSounds;
            std::deque<ma_sound> soundPool;
        };

        static InternalAudioData data;

        //=====================

        void Initialize() {
            ma_result result = ma_engine_init(NULL, &data.engine);
            if(result != MA_SUCCESS) {
                ENG_LOG_CRITICAL("Failed to initialize audio engine.");
                throw std::exception();
            }

            result = ma_sound_group_init(&data.engine, 0, NULL, &data.digital_group);
            if(result != MA_SUCCESS) {
                ENG_LOG_ERROR("Failed to create sound group for digital sounds.");
                throw std::exception();
            }

            result = ma_sound_group_init(&data.engine, MA_SOUND_FLAG_NO_SPATIALIZATION, NULL, &data.music_group);
            if(result != MA_SUCCESS) {
                ENG_LOG_ERROR("Failed to create sound group for music sounds.");
                throw std::exception();
            }

            //TODO: preload all the sound files (use MA_SOUND_FLAG_NO_SPATIALIZATION for music files)
            // for() {
            //     ma_sound_init_from_file(&data.engine, "", MA_SOUND_FLAG_DECODE, &data.preloadedSounds[i]);
            // }

            //not sure why, but direction needs to be set (both listener and sound), otherwise you'll get extreme panning
            //or increase constant's value at miniaudio.h:49088
            ma_engine_listener_set_world_up(&data.engine, 0, 0.f, 1.f, 0.f);
            ma_engine_listener_set_direction(&data.engine, 0, 0.f, 0.f, 1.f);
            ma_engine_listener_set_cone(&data.engine, 0, 1.f, 1.f, 1.f);

            ENG_LOG_TRACE("[C] Audio");
        }

        void Release() {
            if(ma_sound_is_playing(&data.music)) {
                ma_sound_uninit(&data.music);
            }

            for(ma_sound& sound : data.preloadedSounds)
                ma_sound_uninit(&sound);
            data.preloadedSounds.clear();

            for(ma_sound& sound : data.soundPool)
                ma_sound_uninit(&sound);
            data.soundPool.clear();

            ma_engine_uninit(&data.engine);
            ENG_LOG_TRACE("[D] Audio");
        }

        bool Play(const std::string& path) {
            if(!data.enabled)
                return true;

            // ma_result result = ma_engine_play_sound(&data.engine, path.c_str(), &data.digital_group);

            ma_sound* sound = FetchFreeSoundObject();

            ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION;
            ma_result result = ma_sound_init_from_file(&data.engine, path.c_str(), flags, &data.digital_group, NULL, sound);
            ma_sound_start(sound);

            if(result != MA_SUCCESS) {
                ENG_LOG_WARN("Audio::Play - failed to play audio '{}'; reason: {}", path, ma_stringify_error(result));
                return false;
            }
            return true;
        }

        bool Play(const std::string& path, const glm::vec2& position) {
            if(!data.enabled)
                return true;
            
            ma_sound* sound = FetchFreeSoundObject();

            ma_uint32 flags = MA_SOUND_FLAG_DECODE;
            ma_result result = ma_sound_init_from_file(&data.engine, path.c_str(), flags, &data.digital_group, NULL, sound);

            ma_sound_set_position(sound, position.x, position.y, 0.f);

            //spherical sound source
            ma_sound_set_direction(sound, 0.f, 0.f, 1.f);
            ma_sound_set_cone(sound, 1.f, 1.f, 1.f);

            //distance attenuation setup
            ma_sound_set_rolloff(sound, data.outOfScreenRolloff);
            ma_sound_set_min_distance(sound, data.minDistance);
            ma_sound_set_max_distance(sound, data.maxDistance);
            ma_sound_set_attenuation_model(sound, ma_attenuation_model_linear);

            ma_sound_start(sound);

            if(result != MA_SUCCESS) {
                ENG_LOG_WARN("Audio::Play - failed to play audio '{}'; reason: {}", path, ma_stringify_error(result));
                return false;
            }
            return true;
        }

        bool PlayMusic(const std::string& path) {
            if(!data.enabled)
                return true;

            if(ma_sound_is_playing(&data.music)) {
                ma_sound_uninit(&data.music);
            }
            ma_uint32 flags = MA_SOUND_FLAG_DECODE | MA_SOUND_FLAG_NO_SPATIALIZATION;
            ma_result result = ma_sound_init_from_file(&data.engine, path.c_str(), flags, &data.music_group, NULL, &data.music);
            ma_sound_set_looping(&data.music, true);
            ma_sound_start(&data.music);
            if(result != MA_SUCCESS) {
                ENG_LOG_WARN("Audio::PlayMusic - failed to play music '{}'; reason: {}", path, ma_stringify_error(result));
                return false;
            }

            return true;
        }

        void UpdateListenerPosition(const glm::vec2& position) {
            ma_engine_listener_set_position(&data.engine, 0, position.x, position.y, 0.f);
            // ma_vec3f p = ma_engine_listener_get_position(&data.engine, 0);
            // ENG_LOG_TRACE("LISTENER POS: ({}, {}, {})", p.x, p.y, p.z);
        }

        void Enabled(bool enabled) {
            data.enabled = enabled;
            //TODO: add music start/stop
        }

        void DBG_GUI() {
#ifdef ENGINE_ENABLE_GUI
            ImGui::Begin("Sound");

            ImGui::Checkbox("Enabled", &data.enabled);
            ImGui::Separator();

            ImGui::Text("Volume");
            if(ImGui::SliderFloat("Master", &data.volume_master, 0.f, 100.f, "%.1f")) {
                ma_engine_set_volume(&data.engine, data.volume_master * 1e-2f);
            }

            if(ImGui::SliderFloat("Music", &data.volume_music, 0.f, 100.f, "%.1f")) {
                ma_sound_set_volume(&data.music_group, data.volume_music * 1e-2f);
            }

            if(ImGui::SliderFloat("Digital", &data.volume_digital, 0.f, 100.f, "%.1f")) {
                ma_sound_set_volume(&data.digital_group, data.volume_digital * 1e-2f);
            }
            ImGui::Separator();

            static glm::vec2 pos = glm::vec2(0.f);
            if(ImGui::SliderFloat2("listener pos", (float*)&pos, -100.f, 100.f)) {
                UpdateListenerPosition(pos);
            }

            ImGui::End();
#endif
        }

        ma_sound* FetchFreeSoundObject() {
            //TODO: optimize - maybe track unused sounds & garbage collect, maybe sort by time (when will the sound finish) to make lookup easier

            //iterate through the pool, return first free sound
            for(size_t i = 0; i < data.soundPool.size(); i++) {
                if(ma_sound_at_end(&data.soundPool[i])) {
                    ma_sound_uninit(&data.soundPool[i]);
                    ENG_LOG_FINEST("SoundPool - reusing old sound (idx {})", i);
                    return &data.soundPool[i];
                }
            }
            //warning - only use the pointers to initialize the sound (Play() method); pool reallocation will invalidate the pointer

            //no free sound found -> insert new one and return it
            data.soundPool.push_back(ma_sound());
            ENG_LOG_FINEST("SoundPool - inserting new sound ({})", data.soundPool.size());
            return &data.soundPool.back();
        }

    }//namespace Audio

}//namespace eng

#define MAKE_CASE(code) case code: return #code
const char* ma_stringify_error(int err_code) {
    switch(err_code) {
        MAKE_CASE(MA_SUCCESS);
        MAKE_CASE(MA_ERROR);
        MAKE_CASE(MA_INVALID_ARGS);
        MAKE_CASE(MA_INVALID_OPERATION);
        MAKE_CASE(MA_OUT_OF_MEMORY);
        MAKE_CASE(MA_OUT_OF_RANGE);
        MAKE_CASE(MA_ACCESS_DENIED);
        MAKE_CASE(MA_DOES_NOT_EXIST);
        MAKE_CASE(MA_ALREADY_EXISTS);
        MAKE_CASE(MA_TOO_MANY_OPEN_FILES);
        MAKE_CASE(MA_INVALID_FILE);
        MAKE_CASE(MA_TOO_BIG);
        MAKE_CASE(MA_PATH_TOO_LONG);
        MAKE_CASE(MA_NAME_TOO_LONG);
        MAKE_CASE(MA_NOT_DIRECTORY);
        MAKE_CASE(MA_IS_DIRECTORY);
        MAKE_CASE(MA_DIRECTORY_NOT_EMPTY);
        MAKE_CASE(MA_AT_END);
        MAKE_CASE(MA_NO_SPACE);
        MAKE_CASE(MA_BUSY);
        MAKE_CASE(MA_IO_ERROR);
        MAKE_CASE(MA_INTERRUPT);
        MAKE_CASE(MA_UNAVAILABLE);
        MAKE_CASE(MA_ALREADY_IN_USE);
        MAKE_CASE(MA_BAD_ADDRESS);
        MAKE_CASE(MA_BAD_SEEK);
        MAKE_CASE(MA_BAD_PIPE);
        MAKE_CASE(MA_DEADLOCK);
        MAKE_CASE(MA_TOO_MANY_LINKS);
        MAKE_CASE(MA_NOT_IMPLEMENTED);
        MAKE_CASE(MA_NO_MESSAGE);
        MAKE_CASE(MA_BAD_MESSAGE);
        MAKE_CASE(MA_NO_DATA_AVAILABLE);
        MAKE_CASE(MA_INVALID_DATA);
        MAKE_CASE(MA_TIMEOUT);
        MAKE_CASE(MA_NO_NETWORK);
        MAKE_CASE(MA_NOT_UNIQUE);
        MAKE_CASE(MA_NOT_SOCKET);
        MAKE_CASE(MA_NO_ADDRESS);
        MAKE_CASE(MA_BAD_PROTOCOL);
        MAKE_CASE(MA_PROTOCOL_UNAVAILABLE);
        MAKE_CASE(MA_PROTOCOL_NOT_SUPPORTED);
        MAKE_CASE(MA_PROTOCOL_FAMILY_NOT_SUPPORTED);
        MAKE_CASE(MA_ADDRESS_FAMILY_NOT_SUPPORTED);
        MAKE_CASE(MA_SOCKET_NOT_SUPPORTED);
        MAKE_CASE(MA_CONNECTION_RESET);
        MAKE_CASE(MA_ALREADY_CONNECTED);
        MAKE_CASE(MA_NOT_CONNECTED);
        MAKE_CASE(MA_CONNECTION_REFUSED);
        MAKE_CASE(MA_NO_HOST);
        MAKE_CASE(MA_IN_PROGRESS);
        MAKE_CASE(MA_CANCELLED);
        MAKE_CASE(MA_MEMORY_ALREADY_MAPPED);
        MAKE_CASE(MA_FORMAT_NOT_SUPPORTED);
        MAKE_CASE(MA_DEVICE_TYPE_NOT_SUPPORTED);
        MAKE_CASE(MA_SHARE_MODE_NOT_SUPPORTED);
        MAKE_CASE(MA_NO_BACKEND);
        MAKE_CASE(MA_NO_DEVICE);
        MAKE_CASE(MA_API_NOT_FOUND);
        MAKE_CASE(MA_INVALID_DEVICE_CONFIG);
        MAKE_CASE(MA_LOOP);
        MAKE_CASE(MA_DEVICE_NOT_INITIALIZED);
        MAKE_CASE(MA_DEVICE_ALREADY_INITIALIZED);
        MAKE_CASE(MA_DEVICE_NOT_STARTED);
        MAKE_CASE(MA_DEVICE_NOT_STOPPED);
        MAKE_CASE(MA_FAILED_TO_INIT_BACKEND);
        MAKE_CASE(MA_FAILED_TO_OPEN_BACKEND_DEVICE);
        MAKE_CASE(MA_FAILED_TO_START_BACKEND_DEVICE);
        MAKE_CASE(MA_FAILED_TO_STOP_BACKEND_DEVICE);
        default: return "UNKNOWN_ERROR_CODE";
    }
}
#undef MAKE_CASE
