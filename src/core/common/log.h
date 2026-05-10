#pragma once
#include <format>
#include <string>
#include <cstdio>

namespace d3dsw
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

    void LogOutput(const std::string& msg);

    template<typename... Args>
    void Log(LogLevel level, char const* fmt, Args&&... args)
    {
        std::string log_entry = std::vformat(fmt, std::make_format_args(args...));
        std::string full_log_entry = std::format("{} {} {}\n", GetCurrentTimeString(), GetLogPrefix(level), log_entry);
        LogOutput(full_log_entry);
    }

    struct LogInitScope
    {
        LogInitScope()  { LogInit();    }
        ~LogInitScope() { LogDestroy(); }
    };
}
#define D3DSW_LOG_INIT() d3dsw::LogInitScope __log_init_scope{};

#if defined(DEBUG)
#define D3DSW_DEBUG(fmt, ...) d3dsw::Log(d3dsw::LogLevel::Debug,   fmt, ##__VA_ARGS__)
#define D3DSW_INFO(fmt, ...)  d3dsw::Log(d3dsw::LogLevel::Info,    fmt, ##__VA_ARGS__)
#define D3DSW_WARN(fmt, ...)  d3dsw::Log(d3dsw::LogLevel::Warning, fmt, ##__VA_ARGS__)
#else
#define D3DSW_DEBUG(fmt, ...) ((void)0)
#define D3DSW_INFO(fmt, ...)  ((void)0)
#define D3DSW_WARN(fmt, ...)  ((void)0)
#endif
#define D3DSW_ERROR(fmt, ...) d3dsw::Log(d3dsw::LogLevel::Error, fmt, ##__VA_ARGS__)
