#pragma once
#include "context/pipeline_state.h"
#include "context/rasterizer.h"
#include "context/vertex_pipeline.h"
#include "context/output_merger.h"
#include "shaders/shader_abi.h"
#include "shaders/dxbc_parser.h"

namespace d3d11sw {

class D3D11SW_API SWDrawer
{
public:
    SWDrawer();
    ~SWDrawer();

    void Draw(UINT vertexCount, UINT startVertex,
              UINT instanceCount, UINT startInstance,
              D3D11SW_PIPELINE_STATE& state);

    void DrawIndexed(UINT indexCount, UINT startIndex, INT baseVertex,
                     UINT instanceCount, UINT startInstance,
                     D3D11SW_PIPELINE_STATE& state);

private:
    Rasterizer _rasterizer;

    void DrawInternal(VertexPipeline& vp, OutputMerger& om,
                      const UINT* indices, UINT vertexCount, INT baseVertex,
                      D3D11SW_PIPELINE_STATE& state);
};

}
