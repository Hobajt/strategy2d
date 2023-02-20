#pragma once

#include "engine/utils/engine_config.h"

#include <memory>

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>

namespace eng {

    class Log {
    public:
        static void Initialize();

        static std::shared_ptr<spdlog::logger>& EngineLogger() { return engineLogger; }
        static std::shared_ptr<spdlog::logger>& AppLogger() { return appLogger; }
    private:
        static std::shared_ptr<spdlog::logger> engineLogger;
        static std::shared_ptr<spdlog::logger> appLogger;
    };

}//namespace eng

#ifdef ENGINE_ENABLE_LOGGING
    #define ENG_LOG_TRACE(...)      eng::Log::EngineLogger()->trace(__VA_ARGS__)
    #define ENG_LOG_INFO(...)       eng::Log::EngineLogger()->info(__VA_ARGS__)
    #define ENG_LOG_WARN(...)       eng::Log::EngineLogger()->warn(__VA_ARGS__)
    #define ENG_LOG_ERROR(...)      eng::Log::EngineLogger()->error(__VA_ARGS__)
    #define ENG_LOG_CRITICAL(...)   eng::Log::EngineLogger()->critical(__VA_ARGS__)

    #define LOG_TRACE(...)      eng::Log::AppLogger()->trace(__VA_ARGS__)
    #define LOG_INFO(...)       eng::Log::AppLogger()->info(__VA_ARGS__)
    #define LOG_WARN(...)       eng::Log::AppLogger()->warn(__VA_ARGS__)
    #define LOG_ERROR(...)      eng::Log::AppLogger()->error(__VA_ARGS__)
    #define LOG_CRITICAL(...)   eng::Log::AppLogger()->critical(__VA_ARGS__)
#else
    #define ENG_LOG_TRACE(...)
    #define ENG_LOG_INFO(...)
    #define ENG_LOG_WARN(...)
    #define ENG_LOG_ERROR(...)
    #define ENG_LOG_CRITICAL(...)

    #define LOG_TRACE(...)
    #define LOG_INFO(...)
    #define LOG_WARN(...)
    #define LOG_ERROR(...)
    #define LOG_CRITICAL(...)
#endif
