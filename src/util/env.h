#pragma once
#include <cstdlib>
#include <cstring>

namespace d3d11sw {

template <typename T>
Bool FindEnvVar(const Char* name, T& val, T defaultValue = T{})
{
    val = defaultValue;
    const Char* env = std::getenv(name);
    if (!env)
    {
        return false;
    }

    if constexpr (std::is_same_v<T, Bool>)
    {
        val = (std::strcmp(env, "0") != 0 && std::strcmp(env, "false") != 0);
    }
    else if constexpr (std::is_same_v<T, Int> || std::is_same_v<T, Int32>)
    {
        val = static_cast<T>(std::atoi(env));
    }
    else if constexpr (std::is_same_v<T, Uint32>)
    {
        val = static_cast<T>(std::strtoul(env, nullptr, 10));
    }
    else if constexpr (std::is_same_v<T, Float>)
    {
        val = static_cast<T>(std::atof(env));
    }

    return true;
}

inline Bool GetEnvBool(const char* name, Bool defaultValue = false)
{
    Bool val;
    FindEnvVar(name, val, defaultValue);
    return val;
}

inline Int GetEnvInt(const char* name, Int defaultValue = 0)
{
    Int val;
    FindEnvVar(name, val, defaultValue);
    return val;
}

}
