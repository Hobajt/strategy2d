#pragma once

#include <chrono>

namespace eng {
    
    struct Timer {
        using clockType = std::chrono::high_resolution_clock;
        using timePoint = clockType::time_point;

        using s = std::chrono::seconds;
        using ms = std::chrono::milliseconds;
        using us = std::chrono::microseconds;

        Timer() : start(clockType::now()) {}
        void Reset() { start = clockType::now(); }

        template<typename TimeT = us> long long TimeElapsed() { return std::chrono::duration_cast<TimeT>(clockType::now() - start).count(); }
    private:
        timePoint start;
    };

}//namespace eng
