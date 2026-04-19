#pragma once
#include "context/pipeline_state.h"
#include "context/output_merger.h"
#include "context/vertex_pipeline.h"
#include "context/shader_util.h"
#include "shaders/shader_abi.h"
#include "shaders/dxbc_parser.h"
#include <memory>

namespace d3d11sw {

class TileThreadPool;

class D3D11SW_API SWRasterizer
{
public:
    SWRasterizer();
    ~SWRasterizer();

    void Draw(Uint vertexCount, Uint startVertex,
              Uint instanceCount, Uint startInstance,
              D3D11SW_PIPELINE_STATE& state);

    void DrawIndexed(Uint indexCount, Uint startIndex, Int baseVertex,
                     Uint instanceCount, Uint startInstance,
                     D3D11SW_PIPELINE_STATE& state);

    void ClearHiZ(Uint8* dsvData, Int width, Int height, Float clearDepth);
    
private:
    struct Config
    {
        Bool  tiling;
        Int   tileSize;
        Int   tileThreads;
        Float guardBandK;

        static Config FromEnv();
    };

    Config _config;
    std::unique_ptr<TileThreadPool> _tilePool;
    HiZBuffer _hiZ;

private:
    void RasterizeTriangle(const SW_VSOutput tri[3],
                           const D3D11SW_ParsedShader& vsRefl,
                           const D3D11SW_ParsedShader& psRefl,
                           SW_PSFn psQuadFn, SW_Resources& psRes,
                           OMState& om, D3D11SW_PIPELINE_STATE& state,
                           Bool alreadyClipped = false);

    void RasterizeLine(const SW_VSOutput endpts[2],
                       const D3D11SW_ParsedShader& vsRefl,
                       const D3D11SW_ParsedShader& psRefl,
                       SW_PSFn psQuadFn, SW_Resources& psRes,
                       OMState& om, D3D11SW_PIPELINE_STATE& state);

    void RasterizePoint(const SW_VSOutput& point,
                        const D3D11SW_ParsedShader& vsRefl,
                        const D3D11SW_ParsedShader& psRefl,
                        SW_PSFn psQuadFn, SW_Resources& psRes,
                        OMState& om, D3D11SW_PIPELINE_STATE& state);

    void DrawInternal(VertexState& vs, OMState& om,
                      const Uint* indices, Uint vertexCount, Int baseVertex,
                      D3D11SW_PIPELINE_STATE& state);

    void DrawWithGS(VertexState& vs, OMState& om,
                    const Uint* indices, Uint vertexCount, Int baseVertex,
                    const D3D11SW_ParsedShader& vsRefl,
                    const D3D11SW_ParsedShader& psRefl,
                    SW_PSFn psFn, SW_Resources& psRes,
                    D3D11SW_PIPELINE_STATE& state);

    void DrawDirect(VertexState& vs, OMState& om,
                    const Uint* indices, Uint vertexCount, Int baseVertex,
                    const D3D11SW_ParsedShader& vsRefl,
                    const D3D11SW_ParsedShader& psRefl,
                    SW_PSFn psFn, SW_Resources& psRes,
                    D3D11SW_PIPELINE_STATE& state);

    void DrawWithTessellation(VertexState& vs, OMState& om,
                              const Uint* indices, Uint vertexCount, Int baseVertex,
                              const D3D11SW_ParsedShader& vsRefl,
                              const D3D11SW_ParsedShader& psRefl,
                              SW_PSFn psFn, SW_Resources& psRes,
                              D3D11SW_PIPELINE_STATE& state);

    void RasterizePrimitive(const SW_VSOutput* v, Uint n,
                            const D3D11SW_ParsedShader& vsRefl,
                            const D3D11SW_ParsedShader& psRefl,
                            SW_PSFn psFn, SW_Resources& psRes,
                            OMState& om, D3D11SW_PIPELINE_STATE& state);

    void RasterizeGSOutput(const SW_GSOutput& gsOut,
                           const D3D11SW_ParsedShader& gsRefl,
                           const D3D11SW_ParsedShader& psRefl,
                           SW_PSFn psQuadFn, SW_Resources& psRes,
                           OMState& om, D3D11SW_PIPELINE_STATE& state);

    void WriteSOVertices(const SW_GSOutput& gsOut,
                         const D3D11SW_ParsedShader& gsRefl,
                         const D3D11GeometryShaderSW& gs,
                         D3D11SW_PIPELINE_STATE& state);
};

}
