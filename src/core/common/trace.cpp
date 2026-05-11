#include "core/common/common.h"
#include "core/common/trace.h"
#include "core/util/env.h"

namespace d3dsw {

ApiTracer::ApiTracer()
{
    const Char* env = GetEnvString("D3DSW_TRACE");
    if (!env || std::strcmp(env, "0") == 0)
    {
        return;
    }

    _enabled = true;

    if (std::strcmp(env, "1") == 0 || std::strcmp(env, "all") == 0)
    {
        _mask = static_cast<Uint32>(TraceCategory::All);
    }
    else
    {
        _mask = 0;
        const Char* p = env;
        while (*p)
        {
            while (*p == ',' || *p == '|' || *p == '+')
            {
                ++p;
            }
            if (!*p)
            {
                break;
            }

            const Char* start = p;
            while (*p && *p != ',' && *p != '|' && *p != '+')
            {
                ++p;
            }

            Usize len = static_cast<Usize>(p - start);
            if (std::strncmp(start, "create", len) == 0 && len == 6)
            {
                _mask |= static_cast<Uint32>(TraceCategory::Create);
            }
            else if (std::strncmp(start, "state", len) == 0 && len == 5)
            {
                _mask |= static_cast<Uint32>(TraceCategory::State);
            }
            else if (std::strncmp(start, "draw", len) == 0 && len == 4)
            {
                _mask |= static_cast<Uint32>(TraceCategory::Draw);
            }
            else if (std::strncmp(start, "shader", len) == 0 && len == 6)
            {
                _mask |= static_cast<Uint32>(TraceCategory::Shader);
            }
            else if (std::strncmp(start, "map", len) == 0 && len == 3)
            {
                _mask |= static_cast<Uint32>(TraceCategory::Map);
            }
            else if (std::strncmp(start, "present", len) == 0 && len == 7)
            {
                _mask |= static_cast<Uint32>(TraceCategory::Present);
            }
        }

        if (_mask == 0)
        {
            _mask = static_cast<Uint32>(TraceCategory::All);
        }
    }

    const Char* file = GetEnvString("D3DSW_TRACE_FILE");
    if (file && std::strlen(file) > 0)
    {
        _output = std::fopen(file, "w");
    }

    if (!_output)
    {
        _output = stderr;
    }
}

ApiTracer::~ApiTracer()
{
    if (_output && _output != stderr)
    {
        std::fclose(_output);
    }
}

void ApiTracer::Emit(const Char* name, const Char* params)
{
    Uint64 idx = _callIndex++;
    if (params)
    {
        std::fprintf(_output, "[%llu] %s(%s)\n", idx, name, params);
    }
    else
    {
        std::fprintf(_output, "[%llu] %s()\n", idx, name);
    }
}

}
