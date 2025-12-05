#include "context/drawer.h"
#include "shaders/vertex_shader.h"
#include "shaders/pixel_shader.h"
#include <vector>

namespace d3d11sw {

SWDrawer::SWDrawer() = default;
SWDrawer::~SWDrawer() = default;

void SWDrawer::DrawInternal(
    VertexPipeline& vp, OutputMerger& om,
    const UINT* indices, UINT vertexCount, INT baseVertex,
    D3D11SW_PIPELINE_STATE& state)
{
    SW_PSFn psFn = state.ps->GetJitFn();
    if (!vp.IsValid() || !psFn) { return; }

    const D3D11SW_ParsedShader& vsRefl = vp.Reflection();
    const D3D11SW_ParsedShader& psRefl = state.ps->GetReflection();

    SW_Resources psRes{};
    BuildStageResources(psRes, state.psCBs, state.psSRVs, state.psSamplers);

    vp.ProcessPrimitives(indices, vertexCount, baseVertex, state.topology,
        [&](const SW_VSOutput tri[3])
        {
            _rasterizer.RasterizeTriangle(tri, vsRefl, psRefl, psFn, psRes, om, state);
        },
        [&](const SW_VSOutput endpts[2])
        {
            _rasterizer.RasterizeLine(endpts, vsRefl, psRefl, psFn, psRes, om, state);
        },
        [&](const SW_VSOutput& pt)
        {
            _rasterizer.RasterizePoint(pt, vsRefl, psRefl, psFn, psRes, om, state);
        });
}

void SWDrawer::Draw(
    UINT vertexCount, UINT startVertex,
    UINT instanceCount, UINT startInstance,
    D3D11SW_PIPELINE_STATE& state)
{
    if (!state.vs || !state.ps) { return; }

    VertexPipeline vp(state);
    OutputMerger om(state);

    for (UINT inst = 0; inst < instanceCount; ++inst)
    {
        DrawInternal(vp, om, nullptr, vertexCount, static_cast<INT>(startVertex), state);
    }
}

void SWDrawer::DrawIndexed(
    UINT indexCount, UINT startIndex, INT baseVertex,
    UINT instanceCount, UINT startInstance,
    D3D11SW_PIPELINE_STATE& state)
{
    if (!state.vs || !state.ps) { return; }

    VertexPipeline vp(state);
    OutputMerger om(state);

    std::vector<UINT> indices(indexCount);
    for (UINT i = 0; i < indexCount; ++i)
    {
        indices[i] = vp.FetchIndex(startIndex + i);
    }

    for (UINT inst = 0; inst < instanceCount; ++inst)
    {
        DrawInternal(vp, om, indices.data(), indexCount, baseVertex, state);
    }
}

}
