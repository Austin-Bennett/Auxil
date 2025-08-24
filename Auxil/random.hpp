#ifndef RANDOM_HPP
#define RANDOM_HPP

#include "math.hpp"

namespace Auxil {
    class Random {
        std::mt19937_64 engine;
    public:

        Random() : engine(std::random_device{}()) {}

        // returns a random number in [min, max]
        template<std::integral T>
        T random(T min, T max) {
            std::uniform_int_distribution<T> dist(min, max);
            return dist(engine);
        }

        template<std::floating_point T>
        T random(T min, T max) {
            std::uniform_real_distribution<T> dist(min, max);
            return dist(engine);
        }

        // returns a random number across the entire range of T
        template<std::integral T>
        T random() {
            std::uniform_int_distribution<T> dist(
                std::numeric_limits<T>::lowest(),
                std::numeric_limits<T>::max()
            );
            return dist(engine);
        }

        template<std::floating_point T>
        T random() {
            std::uniform_real_distribution<T> dist(
                std::numeric_limits<T>::lowest(),
                std::numeric_limits<T>::max()
            );
            return dist(engine);
        }

        // returns a float in [0,1)
        template<std::floating_point T>
        T random_percent() {
            std::uniform_real_distribution<T> dist(0.0, 1.0);
            return dist(engine);
        }

    };
}

#endif