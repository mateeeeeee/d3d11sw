#pragma once
#include <unordered_map>
#include "core/pipeline/sw_pipeline_state.h"
#include "core/shaders/shader_abi.h"
#include "core/shaders/parsed_shader.h"

namespace d3dsw {

struct VertexState
{
    SW_VSFn vsFn;
    const D3DSW_ParsedShader* vsReflection;
    SW_Resources vsRes;
    const SWPipelineState* state;
    SWPipelineStatistics* stats;
    std::unordered_map<Uint, SW_VSOutput> cache;
    Bool cacheEnabled;
    Uint instanceID;
};

VertexState InitVS(const SWPipelineState& state);
SW_VSOutput RunVS(VertexState& vs, Uint vertIdx);
void FetchVertex(const VertexState& vs, SW_VSInput& vsIn, Uint vertexIndex);
Uint FetchIndex(const VertexState& vs, Uint location);

template<typename OnPrimitiveFn>
void ProcessPrimitives(VertexState& vs, const Uint* indices, Uint vertexCount,
                       Int baseVertex, SWTopology topology,
                       OnPrimitiveFn onPrimitive)
{
    auto fetch = [&](Uint i) -> SW_VSOutput
    {
        Uint vertIdx = indices ? (indices[i] + baseVertex) : (i + baseVertex);
        return RunVS(vs, vertIdx);
    };

    switch (topology)
    {
    case SWTopology::TriangleList:
        for (Uint i = 0; i + 2 < vertexCount; i += 3)
        {
            SW_VSOutput tri[3] = { fetch(i), fetch(i + 1), fetch(i + 2) };
            onPrimitive(tri, 3);
        }
        break;
    case SWTopology::TriangleStrip:
        for (Uint i = 0; i + 2 < vertexCount; ++i)
        {
            SW_VSOutput tri[3];
            if (i & 1)
            {
                tri[0] = fetch(i + 1);
                tri[1] = fetch(i);
                tri[2] = fetch(i + 2);
            }
            else
            {
                tri[0] = fetch(i);
                tri[1] = fetch(i + 1);
                tri[2] = fetch(i + 2);
            }
            onPrimitive(tri, 3);
        }
        break;
    case SWTopology::LineList:
        for (Uint i = 0; i + 1 < vertexCount; i += 2)
        {
            SW_VSOutput endpts[2] = { fetch(i), fetch(i + 1) };
            onPrimitive(endpts, 2);
        }
        break;
    case SWTopology::LineStrip:
    {
        if (vertexCount < 2) { break; }
        SW_VSOutput prev = fetch(0);
        for (Uint i = 1; i < vertexCount; ++i)
        {
            SW_VSOutput cur = fetch(i);
            SW_VSOutput endpts[2] = { prev, cur };
            onPrimitive(endpts, 2);
            prev = cur;
        }
        break;
    }
    case SWTopology::PointList:
        for (Uint i = 0; i < vertexCount; ++i)
        {
            SW_VSOutput pt = fetch(i);
            onPrimitive(&pt, 1);
        }
        break;
    case SWTopology::LineListAdj:
        for (Uint i = 0; i + 3 < vertexCount; i += 4)
        {
            SW_VSOutput v[4] = { fetch(i), fetch(i + 1), fetch(i + 2), fetch(i + 3) };
            onPrimitive(v, 4);
        }
        break;
    case SWTopology::LineStripAdj:
        for (Uint i = 0; i + 3 < vertexCount; ++i)
        {
            SW_VSOutput v[4] = { fetch(i), fetch(i + 1), fetch(i + 2), fetch(i + 3) };
            onPrimitive(v, 4);
        }
        break;
    case SWTopology::TriangleListAdj:
        for (Uint i = 0; i + 5 < vertexCount; i += 6)
        {
            SW_VSOutput v[6] = { fetch(i), fetch(i + 1), fetch(i + 2),
                                 fetch(i + 3), fetch(i + 4), fetch(i + 5) };
            onPrimitive(v, 6);
        }
        break;
    case SWTopology::TriangleStripAdj:
    {
        Uint numTris = (vertexCount >= 4) ? (vertexCount - 4) / 2 : 0;
        for (Uint t = 0; t < numTris; ++t)
        {
            SW_VSOutput v[6];
            if (t == 0)
            {
                v[0] = fetch(0); v[1] = fetch(1); v[2] = fetch(2);
                v[3] = fetch(3); v[4] = fetch(4); v[5] = fetch(5);
            }
            else if (t & 1)
            {
                Uint base = 2 * t;
                v[0] = fetch(base + 2); v[1] = fetch(base - 2); v[2] = fetch(base);
                v[3] = fetch(base + 3); v[4] = fetch(base + 4);
                v[5] = (t + 1 < numTris) ? fetch(base + 6) : fetch(base + 4);
            }
            else
            {
                Uint base = 2 * t;
                v[0] = fetch(base);     v[1] = fetch(base + 3); v[2] = fetch(base + 2);
                v[3] = fetch(base - 2); v[4] = fetch(base + 4);
                v[5] = (t + 1 < numTris) ? fetch(base + 6) : fetch(base + 4);
            }
            onPrimitive(v, 6);
        }
        break;
    }
    case SWTopology::TriangleFan:
        // Triangle i: v[0], v[i+1], v[i+2] for i = 0...(vertexCount-3)
        if (vertexCount >= 3)
        {
            SW_VSOutput hub = fetch(0);
            SW_VSOutput prev = fetch(1);
            for (Uint i = 2; i < vertexCount; ++i)
            {
                SW_VSOutput cur = fetch(i);
                SW_VSOutput tri[3] = { hub, prev, cur };
                onPrimitive(tri, 3);
                prev = cur;
            }
        }
        break;
    default:
    {
        Uint topVal = static_cast<Uint>(topology);
        if (topVal >= static_cast<Uint>(SWTopology::PatchList1) && topVal <= static_cast<Uint>(SWTopology::PatchList32))
        {
            Uint cpCount = topVal - 32;
            for (Uint i = 0; i + cpCount <= vertexCount; i += cpCount)
            {
                SW_VSOutput patch[SW_MAX_PATCH_CP];
                for (Uint j = 0; j < cpCount; ++j)
                {
                    patch[j] = fetch(i + j);
                }
                onPrimitive(patch, cpCount);
            }
        }
        break;
    }
    }
}

}
