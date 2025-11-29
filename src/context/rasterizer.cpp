#include "context/rasterizer.h"
#include "context/context_util.h"
#include "misc/input_layout.h"
#include "shaders/vertex_shader.h"
#include "shaders/pixel_shader.h"
#include "states/rasterizer_state.h"
#include "views/render_target_view.h"
#include "views/shader_resource_view.h"
#include "states/sampler_state.h"
#include <algorithm>
#include <cstring>

namespace d3d11sw {

static void UnpackVertexElement(DXGI_FORMAT fmt, const UINT8* src, SW_float4& out)
{
    out = {0.f, 0.f, 0.f, 0.f};
    switch (fmt)
    {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            std::memcpy(&out, src, 16);
            break;
        case DXGI_FORMAT_R32G32B32_FLOAT:
            std::memcpy(&out, src, 12);
            out.w = 0.f;
            break;
        case DXGI_FORMAT_R32G32_FLOAT:
            std::memcpy(&out, src, 8);
            break;
        case DXGI_FORMAT_R32_FLOAT:
            std::memcpy(&out.x, src, 4);
            break;
        default:
            break;
    }
}

void SWRasterizer::BuildStageResources(
    SW_Resources& res,
    D3D11BufferSW* const* cbs,
    D3D11ShaderResourceViewSW* const* srvs,
    D3D11SamplerStateSW* const* samplers)
{
    for (UINT i = 0; i < SW_MAX_CBUFS; ++i)
    {
        if (cbs[i])
        {
            res.cb[i] = static_cast<const SW_float4*>(cbs[i]->GetDataPtr());
        }
    }

    for (UINT i = 0; i < SW_MAX_TEXTURES; ++i)
    {
        D3D11ShaderResourceViewSW* srv = srvs[i];
        if (!srv) { continue; }

        D3D11SW_SUBRESOURCE_LAYOUT layout = srv->GetLayout();
        SW_Texture& tex = res.tex[i];
        tex.data        = srv->GetDataPtr();
        tex.format      = srv->GetFormat();
        tex.width       = layout.PixelStride > 0 ? layout.RowPitch / layout.PixelStride : 0;
        tex.height      = layout.NumRows;
        tex.depth       = layout.NumSlices;
        tex.rowPitch    = layout.RowPitch;
        tex.slicePitch  = layout.DepthPitch;
        tex.mipLevels   = 1;
    }

    for (UINT i = 0; i < SW_MAX_SAMPLERS; ++i)
    {
        D3D11SamplerStateSW* smp = samplers[i];
        if (!smp) { continue; }

        D3D11_SAMPLER_DESC desc{};
        smp->GetDesc(&desc);
        res.smp[i].filter          = desc.Filter;
        res.smp[i].addressU        = desc.AddressU;
        res.smp[i].addressV        = desc.AddressV;
        res.smp[i].addressW        = desc.AddressW;
        res.smp[i].mipLODBias      = desc.MipLODBias;
        res.smp[i].minLOD          = desc.MinLOD;
        res.smp[i].maxLOD          = desc.MaxLOD;
        res.smp[i].comparisonFunc  = desc.ComparisonFunc;
    }
}

void SWRasterizer::FetchVertex(
    SW_VSInput& vsIn,
    UINT vertexIndex,
    const D3D11SW_PIPELINE_STATE& state,
    const D3D11SW_ParsedShader& vsReflection)
{
    if (!state.inputLayout)
    {
        return;
    }

    const auto& elements = state.inputLayout->GetElements();
    for (const auto& elem : elements)
    {
        UINT slot = elem.InputSlot;
        D3D11BufferSW* vb = state.vertexBuffers[slot];
        if (!vb)
        {
            continue;
        }

        UINT stride = state.vbStrides[slot];
        UINT offset = state.vbOffsets[slot] + elem.AlignedByteOffset + vertexIndex * stride;
        const UINT8* src = static_cast<const UINT8*>(vb->GetDataPtr()) + offset;

        SW_float4 value{};
        UnpackVertexElement(elem.Format, src, value);
        for (const auto& inp : vsReflection.inputs)
        {
            if (inp.semanticIndex == elem.SemanticIndex &&
                std::strcmp(inp.name, elem.SemanticName) == 0)
            {
                vsIn.v[inp.reg] = value;
                break;
            }
        }
    }
}

UINT SWRasterizer::FetchIndex(UINT location, const D3D11SW_PIPELINE_STATE& state)
{
    if (!state.indexBuffer)
    {
        return 0;
    }

    const UINT8* base = static_cast<const UINT8*>(state.indexBuffer->GetDataPtr());
    if (state.indexFormat == DXGI_FORMAT_R16_UINT)
    {
        UINT16 idx;
        std::memcpy(&idx, base + state.indexOffset + location * 2, 2);
        return idx;
    }
    else
    {
        UINT32 idx;
        std::memcpy(&idx, base + state.indexOffset + location * 4, 4);
        return idx;
    }
}

static Int FindSemanticRegister(const std::vector<D3D11SW_ShaderSignatureElement>& sig,
                                const Char* name, Uint32 semIdx)
{
    for (const auto& e : sig)
    {
        if (e.semanticIndex == semIdx && std::strcmp(e.name, name) == 0)
        {
            return static_cast<Int>(e.reg);
        }
    }
    return -1;
}

static Int FindSVPositionInput(const D3D11SW_ParsedShader& shader)
{
    for (const auto& e : shader.inputs)
    {
        if (std::strcmp(e.name, "SV_Position") == 0 || std::strcmp(e.name, "SV_POSITION") == 0)
        {
            return static_cast<Int>(e.reg);
        }
    }
    return -1;
}

void SWRasterizer::RasterizeTriangle(
    const SW_VSOutput tri[3],
    const D3D11SW_ParsedShader& vsReflection,
    const D3D11SW_ParsedShader& psReflection,
    SW_PSFn psFn,
    SW_Resources& psRes,
    D3D11SW_PIPELINE_STATE& state)
{
    if (state.numViewports == 0)
    {
        return;
    }

    const D3D11_VIEWPORT& vp = state.viewports[0];
    for (Int v = 0; v < 3; ++v)
    {
        if (tri[v].pos.w <= 0.f)
        {
            return;
        }
    }

    Float ndcX[3], ndcY[3], ndcZ[3], invW[3];
    Float screenX[3], screenY[3];
    for (Int v = 0; v < 3; ++v)
    {
        Float w = tri[v].pos.w;
        invW[v] = 1.f / w;
        ndcX[v] = tri[v].pos.x * invW[v];
        ndcY[v] = tri[v].pos.y * invW[v];
        ndcZ[v] = tri[v].pos.z * invW[v];

        screenX[v] = (ndcX[v] * 0.5f + 0.5f) * vp.Width  + vp.TopLeftX;
        screenY[v] = (1.f - (ndcY[v] * 0.5f + 0.5f)) * vp.Height + vp.TopLeftY;
    }

    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_BACK;
    rsDesc.FrontCounterClockwise = FALSE;
    if (state.rsState)
    {
        state.rsState->GetDesc(&rsDesc);
    }

    Float edgeArea = (screenX[1] - screenX[0]) * (screenY[2] - screenY[0])
                   - (screenY[1] - screenY[0]) * (screenX[2] - screenX[0]);

    Bool frontFace = rsDesc.FrontCounterClockwise ? (edgeArea > 0.f) : (edgeArea < 0.f);

    if (rsDesc.CullMode == D3D11_CULL_BACK  && !frontFace) { return; }
    if (rsDesc.CullMode == D3D11_CULL_FRONT &&  frontFace) { return; }

    if (edgeArea == 0.f) { return; }

    // 28.4 fixed-point
    using I64 = Int64;
    auto toFixed = [](Float v) -> I64 { return static_cast<I64>(v * 16.f + 0.5f); };

    I64 fx[3], fy[3];
    for (Int v = 0; v < 3; ++v)
    {
        fx[v] = toFixed(screenX[v]);
        fy[v] = toFixed(screenY[v]);
    }

    I64 fixedArea2 = (fx[1] - fx[0]) * (fy[2] - fy[0]) - (fy[1] - fy[0]) * (fx[2] - fx[0]);
    if (fixedArea2 == 0) { return; }

    Bool ccw = fixedArea2 > 0;
    Int i0 = 0, i1 = 1, i2 = 2;
    if (!ccw)
    {
        std::swap(i1, i2);
        fixedArea2 = -fixedArea2;
        std::swap(fx[1], fx[2]);
        std::swap(fy[1], fy[2]);
        std::swap(screenX[1], screenX[2]);
        std::swap(screenY[1], screenY[2]);
        std::swap(ndcZ[1], ndcZ[2]);
        std::swap(invW[1], invW[2]);
    }

    Float minXf = std::min({screenX[0], screenX[1], screenX[2]});
    Float minYf = std::min({screenY[0], screenY[1], screenY[2]});
    Float maxXf = std::max({screenX[0], screenX[1], screenX[2]});
    Float maxYf = std::max({screenY[0], screenY[1], screenY[2]});

    Int minX = static_cast<Int>(std::floor(minXf));
    Int minY = static_cast<Int>(std::floor(minYf));
    Int maxX = static_cast<Int>(std::ceil(maxXf));
    Int maxY = static_cast<Int>(std::ceil(maxYf));

    minX = std::max(minX, static_cast<Int>(vp.TopLeftX));
    minY = std::max(minY, static_cast<Int>(vp.TopLeftY));
    maxX = std::min(maxX, static_cast<Int>(vp.TopLeftX + vp.Width));
    maxY = std::min(maxY, static_cast<Int>(vp.TopLeftY + vp.Height));

    if (minX >= maxX || minY >= maxY) { return; }

    auto edgeFn = [](I64 ax, I64 ay, I64 bx, I64 by, I64 px, I64 py) -> I64
    {
        return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
    };

    auto isTopLeft = [&](Int a, Int b) -> Bool
    {
        I64 edgeY = fy[b] - fy[a];
        I64 edgeX = fx[b] - fx[a];
        Bool isTop  = (edgeY == 0 && edgeX > 0);
        Bool isLeft = (edgeY < 0);
        return isTop || isLeft;
    };

    I64 bias0 = isTopLeft(1, 2) ? 0 : -1;
    I64 bias1 = isTopLeft(2, 0) ? 0 : -1;
    I64 bias2 = isTopLeft(0, 1) ? 0 : -1;

    double invArea2 = 1.0 / static_cast<double>(fixedArea2);

    Int svPosRegPS = FindSVPositionInput(psReflection);

    struct VaryingMap { Int vsOutReg; Int psInReg; };
    VaryingMap varyings[D3D11_VS_OUTPUT_REGISTER_COUNT];
    Int numVaryings = 0;

    for (const auto& psIn : psReflection.inputs)
    {
        if (psIn.svType != 0) { continue; }

        Int vsOutReg = FindSemanticRegister(vsReflection.outputs, psIn.name, psIn.semanticIndex);
        if (vsOutReg >= 0)
        {
            varyings[numVaryings++] = { vsOutReg, static_cast<Int>(psIn.reg) };
        }
    }

    const SW_VSOutput& v0 = tri[i0];
    const SW_VSOutput& v1 = tri[i1];
    const SW_VSOutput& v2 = tri[i2];
    Float iw0 = invW[0], iw1 = invW[1], iw2 = invW[2];

    D3D11RenderTargetViewSW* rtv0 = (state.numRenderTargets > 0) ? state.renderTargets[0] : nullptr;
    if (!rtv0) { return; }

    UINT8* rtvData          = rtv0->GetDataPtr();
    DXGI_FORMAT rtvFmt      = rtv0->GetFormat();
    UINT rtvRowPitch        = rtv0->GetLayout().RowPitch;
    UINT rtvPixStride       = rtv0->GetLayout().PixelStride;

    for (Int py = minY; py < maxY; ++py)
    {
        for (Int px = minX; px < maxX; ++px)
        {
            I64 sampleX = static_cast<I64>(px) * 16 + 8;
            I64 sampleY = static_cast<I64>(py) * 16 + 8;

            I64 w0 = edgeFn(fx[1], fy[1], fx[2], fy[2], sampleX, sampleY) + bias0;
            I64 w1 = edgeFn(fx[2], fy[2], fx[0], fy[0], sampleX, sampleY) + bias1;
            I64 w2 = edgeFn(fx[0], fy[0], fx[1], fy[1], sampleX, sampleY) + bias2;

            if (w0 < 0 || w1 < 0 || w2 < 0) { continue; }

            double b0 = static_cast<double>(w0) * invArea2;
            double b1 = static_cast<double>(w1) * invArea2;
            double b2 = 1.0 - b0 - b1;

            Float perspW = static_cast<Float>(b0 * iw0 + b1 * iw1 + b2 * iw2);
            Float invPerspW = (perspW != 0.f) ? 1.f / perspW : 0.f;

            Float depth = static_cast<Float>(b0 * ndcZ[0] + b1 * ndcZ[1] + b2 * ndcZ[2]);
            depth = std::clamp(depth, vp.MinDepth, vp.MaxDepth);

            SW_PSInput psIn{};

            if (svPosRegPS >= 0)
            {
                psIn.v[svPosRegPS] = { static_cast<Float>(px) + 0.5f,
                                       static_cast<Float>(py) + 0.5f,
                                       depth,
                                       perspW };
            }
            psIn.pos = { static_cast<Float>(px) + 0.5f,
                         static_cast<Float>(py) + 0.5f,
                         depth,
                         perspW };

            for (Int vi = 0; vi < numVaryings; ++vi)
            {
                Int vsR = varyings[vi].vsOutReg;
                Int psR = varyings[vi].psInReg;

                Float ax = static_cast<Float>(b0 * v0.o[vsR].x * iw0 + b1 * v1.o[vsR].x * iw1 + b2 * v2.o[vsR].x * iw2) * invPerspW;
                Float ay = static_cast<Float>(b0 * v0.o[vsR].y * iw0 + b1 * v1.o[vsR].y * iw1 + b2 * v2.o[vsR].y * iw2) * invPerspW;
                Float az = static_cast<Float>(b0 * v0.o[vsR].z * iw0 + b1 * v1.o[vsR].z * iw1 + b2 * v2.o[vsR].z * iw2) * invPerspW;
                Float aw = static_cast<Float>(b0 * v0.o[vsR].w * iw0 + b1 * v1.o[vsR].w * iw1 + b2 * v2.o[vsR].w * iw2) * invPerspW;

                psIn.v[psR] = { ax, ay, az, aw };
            }

            SW_PSOutput psOut{};
            psFn(&psIn, &psOut, &psRes);

            FLOAT finalColor[4] = { psOut.oC[0].x, psOut.oC[0].y, psOut.oC[0].z, psOut.oC[0].w };

            UINT8* rtvPx = rtvData + (UINT64)py * rtvRowPitch + (UINT64)px * rtvPixStride;
            UINT8 packed[16];
            PackRTVColor(rtvFmt, finalColor, packed);
            std::memcpy(rtvPx, packed, rtvPixStride);
        }
    }
}

void SWRasterizer::DrawInternal(
    const UINT* indices, UINT vertexCount, INT baseVertex,
    D3D11SW_PIPELINE_STATE& state)
{
    if (!state.vs || !state.ps) { return; }
    if (state.topology != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) { return; }
    if (vertexCount < 3) { return; }

    SW_VSFn vsFn = state.vs->GetJitFn();
    SW_PSFn psFn = state.ps->GetJitFn();
    if (!vsFn || !psFn) { return; }

    const D3D11SW_ParsedShader& vsReflection = state.vs->GetReflection();
    const D3D11SW_ParsedShader& psReflection = state.ps->GetReflection();

    SW_Resources vsRes{};
    BuildStageResources(vsRes, state.vsCBs, state.vsSRVs, state.vsSamplers);

    SW_Resources psRes{};
    BuildStageResources(psRes, state.psCBs, state.psSRVs, state.psSamplers);

    UINT triCount = vertexCount / 3;

    for (UINT t = 0; t < triCount; ++t)
    {
        SW_VSOutput vsOut[3];

        for (Int v = 0; v < 3; ++v)
        {
            UINT vertIdx = indices ? (indices[t * 3 + v] + baseVertex) : (t * 3 + v + baseVertex);

            SW_VSInput vsIn{};
            FetchVertex(vsIn, vertIdx, state, vsReflection);

            std::memset(&vsOut[v], 0, sizeof(SW_VSOutput));
            vsFn(&vsIn, &vsOut[v], &vsRes);
        }

        RasterizeTriangle(vsOut, vsReflection, psReflection, psFn, psRes, state);
    }
}

void SWRasterizer::Draw(
    UINT vertexCount, UINT startVertex,
    UINT instanceCount, UINT startInstance,
    D3D11SW_PIPELINE_STATE& state)
{
    for (UINT inst = 0; inst < instanceCount; ++inst)
    {
        DrawInternal(nullptr, vertexCount, static_cast<INT>(startVertex), state);
    }
}

void SWRasterizer::DrawIndexed(
    UINT indexCount, UINT startIndex, INT baseVertex,
    UINT instanceCount, UINT startInstance,
    D3D11SW_PIPELINE_STATE& state)
{
    std::vector<UINT> indices(indexCount);
    for (UINT i = 0; i < indexCount; ++i)
    {
        indices[i] = FetchIndex(startIndex + i, state);
    }

    for (UINT inst = 0; inst < instanceCount; ++inst)
    {
        DrawInternal(indices.data(), indexCount, baseVertex, state);
    }
}

} // namespace d3d11sw
