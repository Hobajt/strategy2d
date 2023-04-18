#pragma once

#include "engine/utils/engine_config.h"

#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

namespace eng {

    class Log {
    public:
        static void Initialize();

        static std::shared_ptr<spdlog::logger>& EngineFineLogger() { return engineFineLogger; }
        static std::shared_ptr<spdlog::logger>& EngineLogger() { return engineLogger; }
        static std::shared_ptr<spdlog::logger>& AppLogger() { return appLogger; }
    private:
        static std::shared_ptr<spdlog::logger> engineFineLogger;
        static std::shared_ptr<spdlog::logger> engineLogger;
        static std::shared_ptr<spdlog::logger> appLogger;
    };

}//namespace eng

#ifdef ENGINE_ENABLE_LOGGING
    #define ENG_LOG_FINEST(...)     eng::Log::EngineFineLogger()->trace(__VA_ARGS__)
    #define ENG_LOG_FINER(...)      eng::Log::EngineFineLogger()->debug(__VA_ARGS__)
    #define ENG_LOG_FINE(...)       eng::Log::EngineFineLogger()->info(__VA_ARGS__)

    #define ENG_LOG_TRACE(...)      eng::Log::EngineLogger()->trace(__VA_ARGS__)
    #define ENG_LOG_DEBUG(...)      eng::Log::EngineLogger()->debug(__VA_ARGS__)
    #define ENG_LOG_INFO(...)       eng::Log::EngineLogger()->info(__VA_ARGS__)
    #define ENG_LOG_WARN(...)       eng::Log::EngineLogger()->warn(__VA_ARGS__)
    #define ENG_LOG_ERROR(...)      eng::Log::EngineLogger()->error(__VA_ARGS__)
    #define ENG_LOG_CRITICAL(...)   eng::Log::EngineLogger()->critical(__VA_ARGS__)

    #define LOG_TRACE(...)      eng::Log::AppLogger()->trace(__VA_ARGS__)
    #define LOG_DEBUG(...)      eng::Log::AppLogger()->debug(__VA_ARGS__)
    #define LOG_INFO(...)       eng::Log::AppLogger()->info(__VA_ARGS__)
    #define LOG_WARN(...)       eng::Log::AppLogger()->warn(__VA_ARGS__)
    #define LOG_ERROR(...)      eng::Log::AppLogger()->error(__VA_ARGS__)
    #define LOG_CRITICAL(...)   eng::Log::AppLogger()->critical(__VA_ARGS__)
#else
    #define ENG_LOG_FINEST(...)
    #define ENG_LOG_FINER(...)
    #define ENG_LOG_FINE(...)

    #define ENG_LOG_TRACE(...)
    #define ENG_LOG_DEBUG(...)
    #define ENG_LOG_INFO(...)
    #define ENG_LOG_WARN(...)
    #define ENG_LOG_ERROR(...)
    #define ENG_LOG_CRITICAL(...)

    #define LOG_TRACE(...)
    #define LOG_DEBUG(...)
    #define LOG_INFO(...)
    #define LOG_WARN(...)
    #define LOG_ERROR(...)
    #define LOG_CRITICAL(...)
#endif

//=== logging for custom types ===

template<typename OStream, glm::length_t L, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::vec<L, T, Q>& v) {
    return os << glm::to_string(v);
}

template<typename OStream, glm::length_t C, glm::length_t R, typename T, glm::qualifier Q>
inline OStream& operator<<(OStream& os, const glm::mat<C, R, T, Q>& m) {
    return os << glm::to_string(m);
}
