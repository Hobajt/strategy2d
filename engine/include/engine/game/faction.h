#pragma once

#include <memory>

namespace eng {

    class Diplomacy {};

    class FactionController {
    public:
        int GetColorIdx() const;
    private:

    };
    using FactionControllerRef = std::shared_ptr<FactionController>;
    
}//namespace eng
