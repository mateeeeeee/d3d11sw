#pragma once
#include "context/pipeline_state.h"
#include "shaders/shader_abi.h"
#include "shaders/dxbc_parser.h"
#include <unordered_map>

namespace d3d11sw {

class VertexPipeline
{
public:
    explicit VertexPipeline(const D3D11SW_PIPELINE_STATE& state);

    Bool IsValid() const { return _vsFn != nullptr; }
    const D3D11SW_ParsedShader& Reflection() const { return *_vsReflection; }

    void SetInstance(UINT instanceID);
    SW_VSOutput RunVS(UINT vertIdx);
    UINT FetchIndex(UINT location) const;

    template<typename OnTriangle, typename OnLine, typename OnPoint>
    void ProcessPrimitives(const UINT* indices, UINT vertexCount, INT baseVertex,
                           D3D11_PRIMITIVE_TOPOLOGY topology,
                           OnTriangle onTri, OnLine onLine, OnPoint onPoint);

private:
    SW_VSFn _vsFn;
    const D3D11SW_ParsedShader* _vsReflection;
    SW_Resources _vsRes;
    const D3D11SW_PIPELINE_STATE* _state;
    std::unordered_map<UINT, SW_VSOutput> _cache;
    Bool _cacheEnabled;
    UINT _instanceID = 0;

private:
    void FetchVertex(SW_VSInput& vsIn, UINT vertexIndex);
};

template<typename OnTriangle, typename OnLine, typename OnPoint>
void VertexPipeline::ProcessPrimitives(
    const UINT* indices, UINT vertexCount, INT baseVertex,
    D3D11_PRIMITIVE_TOPOLOGY topology,
    OnTriangle onTri, OnLine onLine, OnPoint onPoint)
{
    auto fetch = [&](UINT i) -> SW_VSOutput
    {
        UINT vertIdx = indices ? (indices[i] + baseVertex) : (i + baseVertex);
        return RunVS(vertIdx);
    };

    switch (topology)
    {
    case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
        for (UINT i = 0; i + 2 < vertexCount; i += 3)
        {
            SW_VSOutput tri[3] = { fetch(i), fetch(i + 1), fetch(i + 2) };
            onTri(tri);
        }
        break;
    case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
        for (UINT i = 0; i + 2 < vertexCount; ++i)
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
        for (UINT i = 0; i + 1 < vertexCount; i += 2)
        {
            SW_VSOutput endpts[2] = { fetch(i), fetch(i + 1) };
            onLine(endpts);
        }
        break;
    case D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP:
    {
        if (vertexCount < 2) { break; }
        SW_VSOutput prev = fetch(0);
        for (UINT i = 1; i < vertexCount; ++i)
        {
            SW_VSOutput cur = fetch(i);
            SW_VSOutput endpts[2] = { prev, cur };
            onLine(endpts);
            prev = cur;
        }
        break;
    }
    case D3D11_PRIMITIVE_TOPOLOGY_POINTLIST:
        for (UINT i = 0; i < vertexCount; ++i)
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
