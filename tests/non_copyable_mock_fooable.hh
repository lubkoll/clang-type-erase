#pragma once

#include <array>

namespace Mock
{
    constexpr int value = 42;
    constexpr int other_value = 73;

    struct NonCopyableMockFooable
    {
        NonCopyableMockFooable() = default;
        NonCopyableMockFooable(const NonCopyableMockFooable&) = delete;
        NonCopyableMockFooable& operator=(const NonCopyableMockFooable&) = delete;
        NonCopyableMockFooable(NonCopyableMockFooable&&) = default;
        NonCopyableMockFooable& operator=(NonCopyableMockFooable&&) = default;

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

    struct NonCopyableMockLargeFooable : NonCopyableMockFooable
    {
    private:
        std::array<double,1024> buffer_;
    };
}

