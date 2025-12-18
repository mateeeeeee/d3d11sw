#pragma once
#include "context/pipeline_state.h"
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

    void Draw(UINT vertexCount, UINT startVertex,
              UINT instanceCount, UINT startInstance,
              D3D11SW_PIPELINE_STATE& state);

    void DrawIndexed(UINT indexCount, UINT startIndex, INT baseVertex,
                     UINT instanceCount, UINT startInstance,
                     D3D11SW_PIPELINE_STATE& state);

    static UINT8 ApplyStencilOp(D3D11_STENCIL_OP op, UINT8 curVal, UINT8 ref);
    static Bool CompareStencil(D3D11_COMPARISON_FUNC func, UINT8 ref, UINT8 val);
    static Bool CompareDepth(D3D11_COMPARISON_FUNC func, Float src, Float dst);
    static Float ReadDepthValue(DXGI_FORMAT fmt, const UINT8* src);
    static void WriteDepthValue(DXGI_FORMAT fmt, UINT8* dst, Float depth);
    static UINT8 ReadStencilValue(DXGI_FORMAT fmt, const UINT8* src);
    static void WriteStencilValue(DXGI_FORMAT fmt, UINT8* dst, UINT8 val);
    static UINT8 ApplyLogicOp(D3D11_LOGIC_OP op, UINT8 src, UINT8 dst);

    struct RTInfo
    {
        UINT8*                          data;
        DXGI_FORMAT                     fmt;
        UINT                            rowPitch;
        UINT                            pixStride;
        D3D11_RENDER_TARGET_BLEND_DESC1 blendDesc;
    };

    struct OMState
    {
        Bool depthEnabled;
        D3D11_COMPARISON_FUNC depthFunc;
        D3D11_DEPTH_WRITE_MASK depthWriteMask;
        UINT8* dsvData;
        DXGI_FORMAT dsvFmt;
        UINT dsvRowPitch;
        UINT dsvPixStride;

        Bool stencilEnabled;
        UINT8 stencilReadMask;
        UINT8 stencilWriteMask;
        UINT8 stencilRef;
        D3D11_DEPTH_STENCILOP_DESC stencilFront;
        D3D11_DEPTH_STENCILOP_DESC stencilBack;

        RTInfo rtInfos[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
        UINT activeRTCount;
        const FLOAT* blendFactor;
    };

private:

    struct VertexState
    {
        SW_VSFn vsFn;
        const D3D11SW_ParsedShader* vsReflection;
        SW_Resources vsRes;
        const D3D11SW_PIPELINE_STATE* state;
        std::unordered_map<UINT, SW_VSOutput> cache;
        Bool cacheEnabled;
        UINT instanceID;
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

    static SW_VSOutput RunVS(VertexState& vs, UINT vertIdx);
    static void FetchVertex(const VertexState& vs, SW_VSInput& vsIn, UINT vertexIndex);
    static UINT FetchIndex(const VertexState& vs, UINT location);

    template<typename OnTriangle, typename OnLine, typename OnPoint>
    static void ProcessPrimitives(VertexState& vs, const UINT* indices, UINT vertexCount,
                                  INT baseVertex, D3D11_PRIMITIVE_TOPOLOGY topology,
                                  OnTriangle onTri, OnLine onLine, OnPoint onPoint);

    void RasterizeTriangle(const SW_VSOutput tri[3],
                           const D3D11SW_ParsedShader& vsRefl,
                           const D3D11SW_ParsedShader& psRefl,
                           SW_PSFn psFn, SW_Resources& psRes,
                           OMState& om, D3D11SW_PIPELINE_STATE& state);

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
    static void BlendAndWrite(const OMState& om, Int px, Int py, UINT rtIdx,
                              const SW_float4& color, const SW_float4& src1Color);
    static UINT8 ReadStencil(const OMState& om, Int px, Int py);
    static void WriteStencil(OMState& om, Int px, Int py, UINT8 val);

    static UINT DepthPixelStride(DXGI_FORMAT fmt);
    static Bool FormatHasStencil(DXGI_FORMAT fmt);
    static void UnpackColor(DXGI_FORMAT fmt, const UINT8* src, FLOAT rgba[4]);
    static Float ComputeBlendFactor(D3D11_BLEND factor, const FLOAT src[4], const FLOAT dst[4],
                                    const FLOAT blendFactor[4], Int comp, const FLOAT src1[4]);
    static Float ComputeBlendOp(D3D11_BLEND_OP op, Float srcTerm, Float dstTerm);

    static Int FindSemanticRegister(const std::vector<D3D11SW_ShaderSignatureElement>& sig,
                                    const Char* name, Uint32 semIdx);
    static Int FindSVPositionInput(const D3D11SW_ParsedShader& shader);

    void DrawInternal(VertexState& vs, OMState& om,
                      const UINT* indices, UINT vertexCount, INT baseVertex,
                      D3D11SW_PIPELINE_STATE& state);

    friend void ProcessOneTile(const struct TileContext& ctx, Uint32 tileIdx,
                                SW_PSInput& psIn, SW_PSOutput& psOut);
};

template<typename OnTriangle, typename OnLine, typename OnPoint>
void SWRasterizer::ProcessPrimitives(
    VertexState& vs, const UINT* indices, UINT vertexCount, INT baseVertex,
    D3D11_PRIMITIVE_TOPOLOGY topology,
    OnTriangle onTri, OnLine onLine, OnPoint onPoint)
{
    auto fetch = [&](UINT i) -> SW_VSOutput
    {
        UINT vertIdx = indices ? (indices[i] + baseVertex) : (i + baseVertex);
        return RunVS(vs, vertIdx);
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
