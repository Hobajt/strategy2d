#include "engine/utils/randomness.h"
#include <random>

namespace eng::Random {

    struct Data {
        std::mt19937 gen;
        std::uniform_real_distribution<float> dist;
    public:
        Data() : gen(std::mt19937(std::random_device{}())), dist(std::uniform_real_distribution<float>(0.f, 1.f)) {}
    };

    static Data data = {};

    //==================

    float Uniform() {
        return data.dist(data.gen);
    }

    int UniformInt(int max) {
        std::uniform_int_distribution<int> dist = std::uniform_int_distribution<int>(0, max);
        return dist(data.gen);
    }
    
    int UniformInt(int min, int max) {
        std::uniform_int_distribution<int> dist = std::uniform_int_distribution<int>(min, max);
        return dist(data.gen);
    }

}//namespace eng::Random
