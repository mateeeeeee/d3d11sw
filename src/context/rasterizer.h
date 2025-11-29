#pragma once
#include "context/pipeline_state.h"
#include "shaders/shader_abi.h"
#include "shaders/dxbc_parser.h"

namespace d3d11sw {

class D3D11SW_API SWRasterizer
{
public:
    void Draw(UINT vertexCount, UINT startVertex,
              UINT instanceCount, UINT startInstance,
              D3D11SW_PIPELINE_STATE& state);

    void DrawIndexed(UINT indexCount, UINT startIndex, INT baseVertex,
                     UINT instanceCount, UINT startInstance,
                     D3D11SW_PIPELINE_STATE& state);

private:
    void DrawInternal(const UINT* indices, UINT vertexCount, INT baseVertex,
                      D3D11SW_PIPELINE_STATE& state);

    void BuildStageResources(SW_Resources& res,
                             D3D11BufferSW* const* cbs,
                             D3D11ShaderResourceViewSW* const* srvs,
                             D3D11SamplerStateSW* const* samplers);

    void FetchVertex(SW_VSInput& vsIn,
                     UINT vertexIndex,
                     const D3D11SW_PIPELINE_STATE& state,
                     const D3D11SW_ParsedShader& vsReflection);

    UINT FetchIndex(UINT location, const D3D11SW_PIPELINE_STATE& state);

    void RasterizeTriangle(const SW_VSOutput tri[3],
                           const D3D11SW_ParsedShader& vsReflection,
                           const D3D11SW_ParsedShader& psReflection,
                           SW_PSFn psFn,
                           SW_Resources& psRes,
                           D3D11SW_PIPELINE_STATE& state);
};

}
