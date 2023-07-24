#pragma once

namespace eng::Random {

    //Random uniform float in range <0,1>.
    float Uniform();

    int UniformInt(int max);
    int UniformInt(int min, int max);

}//namespace eng::Random
