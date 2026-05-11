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

    class Logger
    {
    public:
        static Logger& Get()
        {
            static Logger instance;
            return instance;
        }

        Bool IsEnabled(LogLevel level) const { return level >= _minLevel; }

        template<typename... ArgsT>
        void Log(LogLevel level, const Char* fmt, ArgsT&&... args)
        {
            if (!IsEnabled(level))
            {
                return;
            }
            std::string entry = std::vformat(fmt, std::make_format_args(args...));
            std::string full = std::format("{} {} {}\n", GetCurrentTimeString(), GetLogPrefix(level), entry);
            Output(full);
        }

    private:
        LogLevel _minLevel = LogLevel::Warning;

    private:
        Logger();
        ~Logger() = default;
        D3DSW_NONCOPYABLE_NONMOVABLE(Logger)

        static std::string GetLogPrefix(LogLevel level);
        static std::string GetCurrentTimeString();
        void Output(const std::string& msg);
    };
}

#if defined(FINAL_D3DSW)

#define D3DSW_DEBUG(fmt, ...) ((void)0)
#define D3DSW_INFO(fmt, ...)  ((void)0)
#define D3DSW_WARN(fmt, ...)  ((void)0)

#else

#define D3DSW_DEBUG(fmt, ...) do { if (d3dsw::Logger::Get().IsEnabled(d3dsw::LogLevel::Debug))   { d3dsw::Logger::Get().Log(d3dsw::LogLevel::Debug,   fmt, ##__VA_ARGS__); } } while(0)
#define D3DSW_INFO(fmt, ...)  do { if (d3dsw::Logger::Get().IsEnabled(d3dsw::LogLevel::Info))    { d3dsw::Logger::Get().Log(d3dsw::LogLevel::Info,    fmt, ##__VA_ARGS__); } } while(0)
#define D3DSW_WARN(fmt, ...)  do { if (d3dsw::Logger::Get().IsEnabled(d3dsw::LogLevel::Warning)) { d3dsw::Logger::Get().Log(d3dsw::LogLevel::Warning, fmt, ##__VA_ARGS__); } } while(0)

#endif

#define D3DSW_ERROR(fmt, ...) do { if (d3dsw::Logger::Get().IsEnabled(d3dsw::LogLevel::Error)) { d3dsw::Logger::Get().Log(d3dsw::LogLevel::Error, fmt, ##__VA_ARGS__); } } while(0)
