#pragma once
#include <cstdio>
#include <format>
#include <string>
#include "core/util/env.h"

namespace d3dsw {

enum class TraceCategory : Uint32
{
    Create  = 1 << 0,
    State   = 1 << 1,
    Draw    = 1 << 2,
    Shader  = 1 << 3,
    Map     = 1 << 4,
    Present = 1 << 5,
    All     = 0xFFFFFFFF
};

class ApiTracer
{
public:
    static ApiTracer& Get()
    {
        static ApiTracer instance;
        return instance;
    }

    Bool IsEnabled() const { return _enabled; }

    Bool IsEnabled(TraceCategory cat) const
    {
        return _enabled && (static_cast<Uint32>(cat) & _mask) != 0;
    }

    template<typename... ArgsT>
    void Trace(TraceCategory cat, const Char* name, const Char* fmt, ArgsT&&... args)
    {
        if (!IsEnabled(cat))
        {
            return;
        }
        std::string params = std::vformat(fmt, std::make_format_args(args...));
        Emit(name, params.c_str());
    }

    void Trace(TraceCategory cat, const Char* name)
    {
        if (!IsEnabled(cat))
        {
            return;
        }
        Emit(name, nullptr);
    }

    Uint64 GetCallIndex() const { return _callIndex; }

private:
    Bool   _enabled   = false;
    Uint32 _mask      = 0;
    Uint64 _callIndex = 0;
    FILE*  _output    = nullptr;

private:
    ApiTracer();
    ~ApiTracer();
    D3DSW_NONCOPYABLE_NONMOVABLE(ApiTracer)
    void Emit(const Char* name, const Char* params);
};

}

#if defined(FINAL_D3DSW)

#define D3DSW_TRACE(cat, name, ...)        ((void)0)
#define D3DSW_TRACE_DRAW(name, ...)        ((void)0)
#define D3DSW_TRACE_STATE(name, ...)       ((void)0)
#define D3DSW_TRACE_CREATE(name, ...)      ((void)0)
#define D3DSW_TRACE_SHADER(name, ...)      ((void)0)
#define D3DSW_TRACE_MAP(name, ...)         ((void)0)
#define D3DSW_TRACE_PRESENT(name, ...)     ((void)0)

#else

#define D3DSW_TRACE(cat, name, ...) \
    do { if (d3dsw::ApiTracer::Get().IsEnabled(d3dsw::TraceCategory::cat)) { \
        d3dsw::ApiTracer::Get().Trace(d3dsw::TraceCategory::cat, name, ##__VA_ARGS__); \
    } } while(0)

#define D3DSW_TRACE_DRAW(name, ...)    D3DSW_TRACE(Draw, name, ##__VA_ARGS__)
#define D3DSW_TRACE_STATE(name, ...)   D3DSW_TRACE(State, name, ##__VA_ARGS__)
#define D3DSW_TRACE_CREATE(name, ...)  D3DSW_TRACE(Create, name, ##__VA_ARGS__)
#define D3DSW_TRACE_SHADER(name, ...)  D3DSW_TRACE(Shader, name, ##__VA_ARGS__)
#define D3DSW_TRACE_MAP(name, ...)     D3DSW_TRACE(Map, name, ##__VA_ARGS__)
#define D3DSW_TRACE_PRESENT(name, ...) D3DSW_TRACE(Present, name, ##__VA_ARGS__)

#endif
