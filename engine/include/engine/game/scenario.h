#pragma once

#include <memory>
#include <vector>

namespace eng {

    class Level;

    class ScenarioController;
    using ScenarioControllerRef = std::shared_ptr<ScenarioController>;

    class ScenarioController {
    public:
        //Create and initalize controller instance for provided scenario.
        static ScenarioControllerRef Initialize(int campaignIdx, int race, const std::vector<int>& data);

        virtual ~ScenarioController() = default;

        virtual std::vector<int> Export() { return {}; }

        virtual void Update(Level& level) = 0;
    private:

    };

    //=============================================

    class Sc00_ScenarioController : public ScenarioController {
    public:
        Sc00_ScenarioController(const std::vector<int>& data, int race);

        virtual void Update(Level& level) override;
    private:
        bool EndConditionsUpdate(Level& level);
    private:
        float t = 0.f;
    };

    //=============================================

    class Sc01_ScenarioController : public ScenarioController {
    public:
        Sc01_ScenarioController(const std::vector<int>& data, int race);

        virtual void Update(Level& level) override;
    private:
        bool EndConditionsUpdate(Level& level);
    private:
        float t = 0.f;
    };

}//namespace eng
