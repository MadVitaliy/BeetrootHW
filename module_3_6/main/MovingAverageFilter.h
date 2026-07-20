#pragma once
#include <cstddef>

namespace Filters
{

    template <size_t N>
    class MovingAverage
    {
    private:
        size_t index = 0;
        size_t count = 0;
        uint32_t sum = 0;
        uint16_t buffer[N] = {0};

    public:
        uint16_t add(uint16_t value)
        {
            // Subtract the value being overwritten
            sum -= buffer[index];
            // Insert the new value
            buffer[index] = value;
            sum += value;
            // Move index forward (circularly)
            index = (index + 1) % N;
            return getAverage();
        }

        uint16_t getAverage() const
        {
            return sum / N;
        }
    };
}
