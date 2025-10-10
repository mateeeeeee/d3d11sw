#pragma once
#include <format>
#include <string>
#include <cstdio>

namespace d3d11sw
{
    enum class LogLevel : Uint8
    {
        Debug,
        Info,
        Warning,
        Error
    };
    std::string GetLogPrefix(LogLevel level);
    std::string GetCurrentTimeString();

    void LogInit();
    void LogDestroy();

    template<typename... Args>
    void Log(LogLevel level, char const* fmt, Args&&... args)
    {
        std::string log_entry = std::vformat(fmt, std::make_format_args(args...));
        std::string full_log_entry = std::format("{} {} {}", GetCurrentTimeString(), GetLogPrefix(level), log_entry);
        printf("%s\n", full_log_entry.c_str());
    }

    struct LogInitScope
    {
        LogInitScope()  { LogInit();    }
        ~LogInitScope() { LogDestroy(); }
    };
}
#define D3D11SW_LOG_INIT() d3d11sw::LogInitScope __log_init_scope{};

#if defined(DEBUG)
#define D3D11SW_DEBUG(fmt, ...) d3d11sw::Log(d3d11sw::LogLevel::Debug,   fmt, ##__VA_ARGS__)
#define D3D11SW_INFO(fmt, ...)  d3d11sw::Log(d3d11sw::LogLevel::Info,    fmt, ##__VA_ARGS__)
#define D3D11SW_WARN(fmt, ...)  d3d11sw::Log(d3d11sw::LogLevel::Warning, fmt, ##__VA_ARGS__)
#else
#define D3D11SW_DEBUG(fmt, ...) ((void)0)
#define D3D11SW_INFO(fmt, ...)  ((void)0)
#define D3D11SW_WARN(fmt, ...)  ((void)0)
#endif
#define D3D11SW_ERROR(fmt, ...) d3d11sw::Log(d3d11sw::LogLevel::Error, fmt, ##__VA_ARGS__)
