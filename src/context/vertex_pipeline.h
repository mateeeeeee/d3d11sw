#pragma once
#include "context/pipeline_state.h"
#include "shaders/shader_abi.h"
#include "shaders/dxbc_parser.h"
#include <unordered_map>

namespace d3d11sw {

struct VertexState
{
    SW_VSFn vsFn;
    const D3D11SW_ParsedShader* vsReflection;
    SW_Resources vsRes;
    const D3D11SW_PIPELINE_STATE* state;
    std::unordered_map<Uint, SW_VSOutput> cache;
    Bool cacheEnabled;
    Uint instanceID;
};

VertexState InitVS(const D3D11SW_PIPELINE_STATE& state);
SW_VSOutput RunVS(VertexState& vs, Uint vertIdx);
void FetchVertex(const VertexState& vs, SW_VSInput& vsIn, Uint vertexIndex);
Uint FetchIndex(const VertexState& vs, Uint location);

template<typename OnTriangleFn, typename OnLineFn, typename OnPointFn>
void ProcessPrimitives(VertexState& vs, const Uint* indices, Uint vertexCount,
                       Int baseVertex, D3D11_PRIMITIVE_TOPOLOGY topology,
                       OnTriangleFn onTri, OnLineFn onLine, OnPointFn onPoint)
{
    auto fetch = [&](Uint i) -> SW_VSOutput
    {
        Uint vertIdx = indices ? (indices[i] + baseVertex) : (i + baseVertex);
        return RunVS(vs, vertIdx);
    };

    switch (topology)
    {
    case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
        for (Uint i = 0; i + 2 < vertexCount; i += 3)
        {
            SW_VSOutput tri[3] = { fetch(i), fetch(i + 1), fetch(i + 2) };
            onTri(tri);
        }
        break;
    case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
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
            onTri(tri);
        }
        break;
    case D3D11_PRIMITIVE_TOPOLOGY_LINELIST:
        for (Uint i = 0; i + 1 < vertexCount; i += 2)
        {
            SW_VSOutput endpts[2] = { fetch(i), fetch(i + 1) };
            onLine(endpts);
        }
        break;
    case D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP:
    {
        if (vertexCount < 2) { break; }
        SW_VSOutput prev = fetch(0);
        for (Uint i = 1; i < vertexCount; ++i)
        {
            SW_VSOutput cur = fetch(i);
            SW_VSOutput endpts[2] = { prev, cur };
            onLine(endpts);
            prev = cur;
        }
        break;
    }
    case D3D11_PRIMITIVE_TOPOLOGY_POINTLIST:
        for (Uint i = 0; i < vertexCount; ++i)
        {
            SW_VSOutput pt = fetch(i);
            onPoint(pt);
        }
        break;
    default:
        break;
    }
}

}
