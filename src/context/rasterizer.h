#pragma once
#include "context/pipeline_state.h"
#include "context/depth_stencil_util.h"
#include "shaders/shader_abi.h"
#include "shaders/dxbc_parser.h"
#include <memory>
#include <unordered_map>

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

    struct RTInfo
    {
        Uint8*                          data;
        DXGI_FORMAT                     fmt;
        Uint                            rowPitch;
        Uint                            pixStride;
        D3D11_RENDER_TARGET_BLEND_DESC1 blendDesc;
    };

    struct OMState
    {
        Bool depthEnabled;
        D3D11_COMPARISON_FUNC depthFunc;
        D3D11_DEPTH_WRITE_MASK depthWriteMask;
        Uint8* dsvData;
        DXGI_FORMAT dsvFmt;
        Uint dsvRowPitch;
        Uint dsvPixStride;

        Bool stencilEnabled;
        Uint8 stencilReadMask;
        Uint8 stencilWriteMask;
        Uint8 stencilRef;
        D3D11_DEPTH_STENCILOP_DESC stencilFront;
        D3D11_DEPTH_STENCILOP_DESC stencilBack;

        RTInfo rtInfos[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
        Uint activeRTCount;
        const Float* blendFactor;
    };

private:

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

    static OMState InitOM(D3D11SW_PIPELINE_STATE& state);
    static VertexState InitVS(const D3D11SW_PIPELINE_STATE& state);

    static SW_VSOutput RunVS(VertexState& vs, Uint vertIdx);
    static void FetchVertex(const VertexState& vs, SW_VSInput& vsIn, Uint vertexIndex);
    static Uint FetchIndex(const VertexState& vs, Uint location);

    template<typename OnTriangleFn, typename OnLineFn, typename OnPointFn>
    static void ProcessPrimitives(VertexState& vs, const Uint* indices, Uint vertexCount,
                                  Int baseVertex, D3D11_PRIMITIVE_TOPOLOGY topology,
                                  OnTriangleFn onTri, OnLineFn onLine, OnPointFn onPoint);

    void RasterizeTriangle(const SW_VSOutput tri[3],
                           const D3D11SW_ParsedShader& vsRefl,
                           const D3D11SW_ParsedShader& psRefl,
                           SW_PSFn psFn, SW_Resources& psRes,
                           OMState& om, D3D11SW_PIPELINE_STATE& state,
                           Bool alreadyClipped = false);

    void RasterizeLine(const SW_VSOutput endpts[2],
                       const D3D11SW_ParsedShader& vsRefl,
                       const D3D11SW_ParsedShader& psRefl,
                       SW_PSFn psFn, SW_Resources& psRes,
                       OMState& om, D3D11SW_PIPELINE_STATE& state);

    void RasterizePoint(const SW_VSOutput& point,
                        const D3D11SW_ParsedShader& vsRefl,
                        const D3D11SW_ParsedShader& psRefl,
                        SW_PSFn psFn, SW_Resources& psRes,
                        OMState& om, D3D11SW_PIPELINE_STATE& state);

    static Bool TestDepth(const OMState& om, Int px, Int py, Float depth);
    static void WriteDepth(OMState& om, Int px, Int py, Float depth);
    static void WritePixel(OMState& om, Int px, Int py, const SW_PSOutput& psOut);
    static void BlendAndWrite(const OMState& om, Int px, Int py, Uint rtIdx,
                              const SW_float4& color, const SW_float4& src1Color);
    static UINT8 ReadStencil(const OMState& om, Int px, Int py);
    static void WriteStencil(OMState& om, Int px, Int py, Uint8 val);

    static Int FindSemanticRegister(const std::vector<D3D11SW_ShaderSignatureElement>& sig,
                                    const Char* name, Uint32 semIdx);
    static Int FindSVPositionInput(const D3D11SW_ParsedShader& shader);

    void DrawInternal(VertexState& vs, OMState& om,
                      const Uint* indices, Uint vertexCount, Int baseVertex,
                      D3D11SW_PIPELINE_STATE& state);

    friend void ProcessOneTile(const struct TileContext& ctx, Uint32 tileIdx,
                                SW_PSInput& psIn, SW_PSOutput& psOut);
};

template<typename OnTriangleFn, typename OnLineFn, typename OnPointFn>
void SWRasterizer::ProcessPrimitives(
    VertexState& vs, const Uint* indices, Uint vertexCount, Int baseVertex,
    D3D11_PRIMITIVE_TOPOLOGY topology,
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
