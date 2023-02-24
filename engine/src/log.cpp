#include "engine/utils/log.h"

#include <spdlog/sinks/stdout_color_sinks.h>

#ifdef _WIN32
    #define COLOR_CODE_FINEST 8
    #define COLOR_CODE_FINER 8
    #define COLOR_CODE_FINE 8
#else
    #define COLOR_CODE_FINEST "\033[40m\033[90m"
    #define COLOR_CODE_FINER "\033[40m\033[90m"
    #define COLOR_CODE_FINE "\033[40m\033[37m"
#endif

namespace eng {

    std::shared_ptr<spdlog::logger> Log::engineFineLogger;
    std::shared_ptr<spdlog::logger> Log::engineLogger;
    std::shared_ptr<spdlog::logger> Log::appLogger;

    void Log::Initialize() {
        std::vector<spdlog::sink_ptr> logSinks;

        logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        logSinks[0]->set_pattern("%^[%T] %n: %v%$");
        
        spdlog::level::level_enum logLevel = spdlog::level::trace;

        //=== engine logger ===
        engineLogger = std::make_shared<spdlog::logger>("ENG", begin(logSinks), end(logSinks));
		spdlog::register_logger(engineLogger);
		engineLogger->set_level(logLevel);
		engineLogger->flush_on(logLevel);

        //=== application logger ===
		appLogger = std::make_shared<spdlog::logger>("APP", begin(logSinks), end(logSinks));
		spdlog::register_logger(appLogger);
		appLogger->set_level(logLevel);
		appLogger->flush_on(logLevel);

        //=== engine logger (detailed trace) ===
        std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> dontPeeInThe_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        dontPeeInThe_sink->set_pattern("%^[%T] %n: %v%$");
        dontPeeInThe_sink->set_color(spdlog::level::trace, COLOR_CODE_FINEST);
        dontPeeInThe_sink->set_color(spdlog::level::debug, COLOR_CODE_FINER);
        dontPeeInThe_sink->set_color(spdlog::level::info, COLOR_CODE_FINE);
        logSinks[0] = dontPeeInThe_sink;

        engineFineLogger = std::make_shared<spdlog::logger>("DBG", begin(logSinks), end(logSinks));
		spdlog::register_logger(engineFineLogger);
		engineFineLogger->set_level(logLevel);
		engineFineLogger->flush_on(logLevel);
    }

}//namespace eng
