#pragma once

#include <array>

namespace Mock
{
    constexpr int value = 42;
    constexpr int other_value = 73;

    struct MockFooable
    {
        int foo() const
        {
            return value_;
        }

        void set_value(int val)
        {
            value_ = val;
        }

    private:
        int value_ = value;
    };

    struct MockLargeFooable : MockFooable
    {
    private:
        std::array<double,1024> buffer_;
    };
}

