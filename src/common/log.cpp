#include "common/common.h"
#include "common/log.h"
#include <chrono>

namespace d3d11sw
{
    std::string GetLogPrefix(LogLevel level)
    {
        switch (level)
        {
        case LogLevel::Debug:   return "[DEBUG]";
        case LogLevel::Info:    return "[INFO]";
        case LogLevel::Warning: return "[WARNING]";
        case LogLevel::Error:   return "[ERROR]";
        }
        return "";
    }

    std::string GetCurrentTimeString()
    {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::tm local_time{};
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&local_time, &in_time_t);
#else
        localtime_r(&in_time_t, &local_time);
#endif
        Char timestamp[64];
        std::snprintf(timestamp, sizeof(timestamp), "[%04d-%02d-%02d %02d:%02d:%02d.%03d]",
            local_time.tm_year + 1900,
            local_time.tm_mon + 1,
            local_time.tm_mday,
            local_time.tm_hour,
            local_time.tm_min,
            local_time.tm_sec,
            static_cast<Int>(ms.count()));
        return timestamp;
    }

    void LogOutput(const std::string& msg)
    {
        printf("%s", msg.c_str());
#if defined(D3D11SW_PLATFORM_WINDOWS)
        OutputDebugStringA(msg.c_str());
#endif
    }

    void LogInit()
    {
        setbuf(stdout, nullptr);
    }

    void LogDestroy()
    {
    }
}
