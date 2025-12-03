#pragma once
#include "context/pipeline_state.h"
#include "shaders/shader_abi.h"
#include "shaders/dxbc_parser.h"
#include <cstdlib>
#include <cstring>

namespace d3d11sw {

struct RasterizerConfig
{
    Bool tiling;
    Int  tileSize;

    static RasterizerConfig FromEnv()
    {
        RasterizerConfig cfg;
        cfg.tiling   = true;
        cfg.tileSize = 16;

        if (const char* v = std::getenv("D3D11SW_TILING"))
        {
            cfg.tiling = (std::strcmp(v, "0") != 0);
        }
        if (const char* v = std::getenv("D3D11SW_TILE_SIZE"))
        {
            Int s = std::atoi(v);
            if (s >= 4 && (s & (s - 1)) == 0)
            {
                cfg.tileSize = s;
            }
        }
        return cfg;
    }
};

class D3D11SW_API SWRasterizer
{
public:
    SWRasterizer() : _config(RasterizerConfig::FromEnv()) {}

    void Draw(UINT vertexCount, UINT startVertex,
              UINT instanceCount, UINT startInstance,
              D3D11SW_PIPELINE_STATE& state);

    void DrawIndexed(UINT indexCount, UINT startIndex, INT baseVertex,
                     UINT instanceCount, UINT startInstance,
                     D3D11SW_PIPELINE_STATE& state);

private:
    RasterizerConfig _config;

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

    void RasterizeLine(const SW_VSOutput endpts[2],
                       const D3D11SW_ParsedShader& vsReflection,
                       const D3D11SW_ParsedShader& psReflection,
                       SW_PSFn psFn,
                       SW_Resources& psRes,
                       D3D11SW_PIPELINE_STATE& state);

    void RasterizePoint(const SW_VSOutput& point,
                        const D3D11SW_ParsedShader& vsReflection,
                        const D3D11SW_ParsedShader& psReflection,
                        SW_PSFn psFn,
                        SW_Resources& psRes,
                        D3D11SW_PIPELINE_STATE& state);

    SW_VSOutput RunVS(UINT vertIdx, SW_VSFn vsFn,
                      const D3D11SW_ParsedShader& vsReflection,
                      SW_Resources& vsRes,
                      const D3D11SW_PIPELINE_STATE& state);
};

}
