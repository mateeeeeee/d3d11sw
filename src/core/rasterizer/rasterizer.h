#pragma once
#include "core/pipeline/sw_pipeline_state.h"
#include "core/rasterizer/output_merger.h"
#include "core/rasterizer/hi_z.h"
#include "core/rasterizer/vertex_pipeline.h"
#include "core/rasterizer/shader_util.h"
#include "core/shaders/shader_abi.h"
#include "core/shaders/parsed_shader.h"
#include <memory>

namespace d3dsw {

class TileThreadPool;

class D3DSW_API SWRasterizer
{
public:
    SWRasterizer();
    ~SWRasterizer();

    void Draw(const SWDrawCommand& cmd, SWPipelineState& state);
    void DrawIndexed(const SWDrawCommand& cmd, SWPipelineState& state);

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
                           const D3DSW_ParsedShader& vsRefl,
                           const D3DSW_ParsedShader& psRefl,
                           SW_PSFn psQuadFn, SW_Resources& psRes,
                           OMState& om, SWPipelineState& state,
                           Bool alreadyClipped = false);

    void RasterizeLine(const SW_VSOutput endpts[2],
                       const D3DSW_ParsedShader& vsRefl,
                       const D3DSW_ParsedShader& psRefl,
                       SW_PSFn psQuadFn, SW_Resources& psRes,
                       OMState& om, SWPipelineState& state);

    void RasterizePoint(const SW_VSOutput& point,
                        const D3DSW_ParsedShader& vsRefl,
                        const D3DSW_ParsedShader& psRefl,
                        SW_PSFn psQuadFn, SW_Resources& psRes,
                        OMState& om, SWPipelineState& state);

    void DrawInternal(VertexState& vs, OMState& om,
                      const Uint* indices, Uint vertexCount, Int baseVertex,
                      SWPipelineState& state);

    void DrawWithGS(VertexState& vs, OMState& om,
                    const Uint* indices, Uint vertexCount, Int baseVertex,
                    const D3DSW_ParsedShader& vsRefl,
                    const D3DSW_ParsedShader& psRefl,
                    SW_PSFn psFn, SW_Resources& psRes,
                    SWPipelineState& state);

    void DrawDirect(VertexState& vs, OMState& om,
                    const Uint* indices, Uint vertexCount, Int baseVertex,
                    const D3DSW_ParsedShader& vsRefl,
                    const D3DSW_ParsedShader& psRefl,
                    SW_PSFn psFn, SW_Resources& psRes,
                    SWPipelineState& state);

    void DrawWithTessellation(VertexState& vs, OMState& om,
                              const Uint* indices, Uint vertexCount, Int baseVertex,
                              const D3DSW_ParsedShader& vsRefl,
                              const D3DSW_ParsedShader& psRefl,
                              SW_PSFn psFn, SW_Resources& psRes,
                              SWPipelineState& state);

    void RasterizePrimitive(const SW_VSOutput* v, Uint n,
                            const D3DSW_ParsedShader& vsRefl,
                            const D3DSW_ParsedShader& psRefl,
                            SW_PSFn psFn, SW_Resources& psRes,
                            OMState& om, SWPipelineState& state);

    void RasterizeGSOutput(const SW_GSOutput& gsOut,
                           const D3DSW_ParsedShader& gsRefl,
                           const D3DSW_ParsedShader& psRefl,
                           SW_PSFn psQuadFn, SW_Resources& psRes,
                           OMState& om, SWPipelineState& state);

    void WriteSOVertices(const SW_GSOutput& gsOut,
                         const D3DSW_ParsedShader& gsRefl,
                         SWPipelineState& state);
};

}
