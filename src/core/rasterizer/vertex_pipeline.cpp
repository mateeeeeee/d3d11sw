#include <bit>
#include <cmath>
#include <cstring>
#include <d3dcommon.h>
#include "core/rasterizer/vertex_pipeline.h"
#include "core/util/env.h"

namespace d3dsw {

static void UnpackVertexElement(DXGI_FORMAT fmt, const Uint8* src, SW_float4& out)
{
    out = {0.f, 0.f, 0.f, 1.f};
    switch (fmt)
    {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            std::memcpy(&out, src, 16);
            break;
        case DXGI_FORMAT_R32G32B32_FLOAT:
            std::memcpy(&out, src, 12);
            break;
        case DXGI_FORMAT_R32G32_FLOAT:
            std::memcpy(&out, src, 8);
            break;
        case DXGI_FORMAT_R32_FLOAT:
            std::memcpy(&out.x, src, 4);
            break;
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        {
            auto toF = [](Uint16 h) -> Float
            {
                Uint32 sign = (h >> 15) & 1;
                Uint32 exp  = (h >> 10) & 0x1F;
                Uint32 man  = h & 0x3FF;
                if (exp == 0)
                {
                    if (man == 0)
                    {
                        return sign ? -0.f : 0.f;
                    }
                    Float f = std::ldexp(static_cast<Float>(man), -24);
                    return sign ? -f : f;
                }
                if (exp == 31)
                {
                    if (man == 0)
                    {
                        return sign ? -INFINITY : INFINITY;
                    }
                    return NAN;
                }
                Float f = std::ldexp(static_cast<Float>(man + 1024), static_cast<Int>(exp) - 25);
                return sign ? -f : f;
            };
            Uint16 v[4];
            std::memcpy(v, src, 8);
            out = { toF(v[0]), toF(v[1]), toF(v[2]), toF(v[3]) };
            break;
        }
        case DXGI_FORMAT_R16G16_FLOAT:
        {
            auto toF = [](Uint16 h) -> Float
            {
                Uint32 sign = (h >> 15) & 1;
                Uint32 exp  = (h >> 10) & 0x1F;
                Uint32 man  = h & 0x3FF;
                if (exp == 0)
                {
                    if (man == 0)
                    {
                        return sign ? -0.f : 0.f;
                    }
                    Float f = std::ldexp(static_cast<Float>(man), -24);
                    return sign ? -f : f;
                }
                if (exp == 31)
                {
                    if (man == 0)
                    {
                        return sign ? -INFINITY : INFINITY;
                    }
                    return NAN;
                }
                Float f = std::ldexp(static_cast<Float>(man + 1024), static_cast<Int>(exp) - 25);
                return sign ? -f : f;
            };
            Uint16 v[2];
            std::memcpy(v, src, 4);
            out = { toF(v[0]), toF(v[1]), 0.f, 1.f };
            break;
        }
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            out = { src[0] / 255.f, src[1] / 255.f, src[2] / 255.f, src[3] / 255.f };
            break;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8X8_UNORM:
            // Memory order is B,G,R,A; return as float4(R,G,B,A).
            out = { src[2] / 255.f, src[1] / 255.f, src[0] / 255.f, src[3] / 255.f };
            break;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        {
            auto toF = [](Uint8 b) -> Float
            {
                return std::max(static_cast<Float>(std::bit_cast<Int8>(b)) / 127.f, -1.f);
            };
            out = { toF(src[0]), toF(src[1]), toF(src[2]), toF(src[3]) };
            break;
        }
        case DXGI_FORMAT_R8G8B8A8_UINT:
        {
            out = {
                std::bit_cast<Float>(static_cast<Uint32>(src[0])),
                std::bit_cast<Float>(static_cast<Uint32>(src[1])),
                std::bit_cast<Float>(static_cast<Uint32>(src[2])),
                std::bit_cast<Float>(static_cast<Uint32>(src[3]))
            };
            break;
        }
        case DXGI_FORMAT_R8G8B8A8_SINT:
        {
            auto toF = [](Uint8 b) -> Float
            {
                return std::bit_cast<Float>(static_cast<Uint32>(static_cast<Int32>(std::bit_cast<Int8>(b))));
            };
            out = { toF(src[0]), toF(src[1]), toF(src[2]), toF(src[3]) };
            break;
        }
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        {
            Uint32 v[4];
            std::memcpy(v, src, 16);
            out = {
                std::bit_cast<Float>(v[0]),
                std::bit_cast<Float>(v[1]),
                std::bit_cast<Float>(v[2]),
                std::bit_cast<Float>(v[3])
            };
            break;
        }
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        {
            Uint32 packed;
            std::memcpy(&packed, src, 4);
            out = {
                (packed & 0x3FF) / 1023.f,
                ((packed >> 10) & 0x3FF) / 1023.f,
                ((packed >> 20) & 0x3FF) / 1023.f,
                ((packed >> 30) & 0x3) / 3.f
            };
            break;
        }
        case DXGI_FORMAT_R16G16_SNORM:
        {
            Int16 v[2];
            std::memcpy(v, src, 4);
            out = { std::max(v[0] / 32767.f, -1.f), std::max(v[1] / 32767.f, -1.f), 0.f, 1.f };
            break;
        }
        case DXGI_FORMAT_R16G16_UNORM:
        {
            Uint16 v[2];
            std::memcpy(v, src, 4);
            out = { v[0] / 65535.f, v[1] / 65535.f, 0.f, 1.f };
            break;
        }
        case DXGI_FORMAT_R16G16_UINT:
        {
            Uint16 v[2];
            std::memcpy(v, src, 4);
            out = {
                std::bit_cast<Float>(static_cast<Uint32>(v[0])),
                std::bit_cast<Float>(static_cast<Uint32>(v[1])),
                0.f, std::bit_cast<Float>(1u)
            };
            break;
        }
        case DXGI_FORMAT_R16G16_SINT:
        {
            Int16 v[2];
            std::memcpy(v, src, 4);
            out = {
                std::bit_cast<Float>(static_cast<Uint32>(static_cast<Int32>(v[0]))),
                std::bit_cast<Float>(static_cast<Uint32>(static_cast<Int32>(v[1]))),
                0.f, std::bit_cast<Float>(1u)
            };
            break;
        }
        default:
            break;
    }
}

VertexState InitVS(const SWPipelineState& state)
{
    VertexState vs{};
    vs.vsFn = reinterpret_cast<SW_VSFn>(state.vs.fn);
    vs.vsReflection = state.vs.reflection;
    vs.vsRes = state.vsRes;
    vs.state = &state;
    vs.stats = state.stats;
    vs.cacheEnabled = GetEnvBool("D3DSW_VERTEX_CACHE", false);
    vs.instanceID = 0;
    return vs;
}

void FetchVertex(const VertexState& vs, SW_VSInput& vsIn, Uint vertexIndex)
{
    const SWPipelineState& st = *vs.state;
    if (st.numInputElements == 0)
    {
        return;
    }

    for (Uint32 i = 0; i < st.numInputElements; ++i)
    {
        const SWInputElement& elem = st.inputElements[i];
        Uint slot = elem.inputSlot;
        const SWVertexBufferBinding& vb = st.vertexBuffers[slot];
        if (!vb.data)
        {
            continue;
        }

        Uint stride = vb.stride;
        Uint index = vertexIndex;
        if (elem.inputSlotClass == SWInputSlotClass::PerInstanceData)
        {
            Uint step = elem.instanceDataStepRate;
            index = step ? (vs.instanceID / step) : 0;
        }
        Uint offset = vb.offset + elem.alignedByteOffset + index * stride;
        const Uint8* src = vb.data + offset;

        SW_float4 value{};
        UnpackVertexElement(elem.format, src, value);
        for (const auto& inp : vs.vsReflection->inputs)
        {
            if (inp.semanticIndex == elem.semanticIndex &&
                std::strcmp(inp.name, elem.semanticName) == 0)
            {
                vsIn.v[inp.reg] = value;
                break;
            }
        }
    }
}

SW_VSOutput RunVS(VertexState& vs, Uint vertIdx)
{
    if (vs.cacheEnabled)
    {
        auto it = vs.cache.find(vertIdx);
        if (it != vs.cache.end())
        {
            return it->second;
        }
    }

    SW_VSInput vsIn{};
    FetchVertex(vs, vsIn, vertIdx);
    for (const auto& inp : vs.vsReflection->inputs)
    {
        if (inp.svType == D3D_NAME_VERTEX_ID)
        {
            Float bits = std::bit_cast<Float>(vertIdx);
            vsIn.v[inp.reg] = {bits, 0.f, 0.f, 0.f};
        }
        else if (inp.svType == D3D_NAME_INSTANCE_ID)
        {
            Float bits = std::bit_cast<Float>(vs.instanceID);
            vsIn.v[inp.reg] = {bits, 0.f, 0.f, 0.f};
        }
    }

    SW_VSOutput vsOut{};
    vs.vsFn(&vsIn, &vsOut, &vs.vsRes);
    vs.stats->vsInvocations++;
    if (vs.cacheEnabled)
    {
        vs.cache[vertIdx] = vsOut;
    }
    return vsOut;
}

Uint FetchIndex(const VertexState& vs, Uint location)
{
    const SWIndexBufferBinding& ib = vs.state->indexBuffer;
    if (!ib.data)
    {
        return 0;
    }

    const Uint8* base = ib.data;
    if (ib.format == DXGI_FORMAT_R16_UINT)
    {
        Uint16 idx;
        std::memcpy(&idx, base + ib.offset + location * 2, 2);
        return idx;
    }
    else
    {
        Uint32 idx;
        std::memcpy(&idx, base + ib.offset + location * 4, 4);
        return idx;
    }
}

}
