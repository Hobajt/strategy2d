#include "engine/utils/log.h"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace eng {

    std::shared_ptr<spdlog::logger> Log::engineLogger;
    std::shared_ptr<spdlog::logger> Log::appLogger;

    void Log::Initialize() {
        std::vector<spdlog::sink_ptr> logSinks;

        logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
        logSinks[0]->set_pattern("%^[%T] %n: %v%$");

        engineLogger = std::make_shared<spdlog::logger>("ENG", begin(logSinks), end(logSinks));
		spdlog::register_logger(engineLogger);
		engineLogger->set_level(spdlog::level::trace);
		engineLogger->flush_on(spdlog::level::trace);

		appLogger = std::make_shared<spdlog::logger>("APP", begin(logSinks), end(logSinks));
		spdlog::register_logger(appLogger);
		appLogger->set_level(spdlog::level::trace);
		appLogger->flush_on(spdlog::level::trace);
    }

}//namespace eng
