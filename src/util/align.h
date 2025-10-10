#pragma once
#include <concepts>

namespace d3d11sw
{
    template<typename T>
    constexpr T DivideAndRoundUp(T nominator, T denominator)
    {
        return (nominator + denominator - 1) / denominator;
    }

    template<typename T>
    constexpr Bool IsAligned(T value, Uint64 alignment)
    {
        return (value % static_cast<T>(alignment)) == 0;
    }

    template<typename T>
    constexpr T AlignUp(T value, Uint64 alignment)
    {
        return (value + static_cast<T>(alignment - 1)) / static_cast<T>(alignment) * static_cast<T>(alignment);
    }

    template<typename T>
    constexpr T AlignDown(T value, Uint64 alignment)
    {
        return value / static_cast<T>(alignment) * static_cast<T>(alignment);
    }

    template<typename T>
    constexpr Bool IsAlignedPow2(T value, Uint64 alignment)
    {
        return (value & static_cast<T>(alignment - 1)) == 0;
    }

    template<typename T>
    constexpr T AlignUpPow2(T value, Uint64 alignment)
    {
        return (value + static_cast<T>(alignment - 1)) & ~static_cast<T>(alignment - 1);
    }

    template<typename T>
    constexpr T AlignDownPow2(T value, Uint64 alignment)
    {
        return value & ~static_cast<T>(alignment - 1);
    }

    template<typename T>
    constexpr Bool IsPow2(T value)
    {
        return (value & (value - 1)) == 0;
    }

    template<typename T>
    constexpr T PreviousPow2(T value)
    {
        if (value == static_cast<T>(0)) return 0;

        value |= (value >> 1);
        value |= (value >> 2);
        value |= (value >> 4);
        if constexpr (sizeof(T) >= 2) value |= (value >> 8);
        if constexpr (sizeof(T) >= 4) value |= (value >> 16);
        if constexpr (sizeof(T) >= 8) value |= (value >> 32);
        return value - (value >> 1);
    }

    template<typename T>
    constexpr T NextPow2(T value)
    {
        if (value == static_cast<T>(0)) return 0;

        value--;
        value |= (value >> 1);
        value |= (value >> 2);
        value |= (value >> 4);
        if constexpr (sizeof(T) >= 2) value |= (value >> 8);
        if constexpr (sizeof(T) >= 4) value |= (value >> 16);
        if constexpr (sizeof(T) >= 8) value |= (value >> 32);
        value++;
        return value;
    }
}
