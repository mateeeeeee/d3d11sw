#pragma once
#include "context/pipeline_state.h"
#include "context/output_merger.h"
#include "shaders/shader_abi.h"
#include "shaders/dxbc_parser.h"
#include <memory>

namespace d3d11sw {

class TileThreadPool;

class Rasterizer
{
public:
    Rasterizer();
    ~Rasterizer();

    void RasterizeTriangle(const SW_VSOutput tri[3],
                           const D3D11SW_ParsedShader& vsReflection,
                           const D3D11SW_ParsedShader& psReflection,
                           SW_PSFn psFn, SW_Resources& psRes,
                           OutputMerger& om,
                           D3D11SW_PIPELINE_STATE& state);

    void RasterizeLine(const SW_VSOutput endpts[2],
                       const D3D11SW_ParsedShader& vsReflection,
                       const D3D11SW_ParsedShader& psReflection,
                       SW_PSFn psFn, SW_Resources& psRes,
                       OutputMerger& om,
                       D3D11SW_PIPELINE_STATE& state);

    void RasterizePoint(const SW_VSOutput& point,
                        const D3D11SW_ParsedShader& vsReflection,
                        const D3D11SW_ParsedShader& psReflection,
                        SW_PSFn psFn, SW_Resources& psRes,
                        OutputMerger& om,
                        D3D11SW_PIPELINE_STATE& state);

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

private:
    static Int FindSemanticRegister(const std::vector<D3D11SW_ShaderSignatureElement>& sig,
                                    const Char* name, Uint32 semIdx);
    static Int FindSVPositionInput(const D3D11SW_ParsedShader& shader);
};

}
