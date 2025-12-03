#include "context/rasterizer.h"
#include "context/context_util.h"
#include "misc/input_layout.h"
#include "shaders/vertex_shader.h"
#include "shaders/pixel_shader.h"
#include "states/rasterizer_state.h"
#include "states/blend_state.h"
#include "states/depth_stencil_state.h"
#include "views/render_target_view.h"
#include "views/depth_stencil_view.h"
#include "views/shader_resource_view.h"
#include "states/sampler_state.h"
#include "util/env.h"
#include <algorithm>
#include <cstring>

namespace d3d11sw {

RasterizerConfig RasterizerConfig::FromEnv()
{
    RasterizerConfig cfg;
    cfg.tiling   = GetEnvBool("D3D11SW_TILING", true);
    cfg.tileSize = GetEnvInt("D3D11SW_TILE_SIZE", 16);
    if (cfg.tileSize < 4 || (cfg.tileSize & (cfg.tileSize - 1)) != 0)
    {
        cfg.tileSize = 16;
    }
    return cfg;
}

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

static Float ReadDepth(DXGI_FORMAT fmt, const UINT8* src)
{
    switch (fmt)
    {
        case DXGI_FORMAT_D32_FLOAT:
        {
            Float d;
            std::memcpy(&d, src, 4);
            return d;
        }
        case DXGI_FORMAT_D16_UNORM:
        {
            UINT16 u;
            std::memcpy(&u, src, 2);
            return u / 65535.f;
        }
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        {
            UINT32 u;
            std::memcpy(&u, src, 4);
            return (u & 0x00FFFFFF) / 16777215.f;
        }
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        {
            Float d;
            std::memcpy(&d, src, 4);
            return d;
        }
        default:
            return 1.f;
    }
}

static void WriteDepth(DXGI_FORMAT fmt, UINT8* dst, Float depth)
{
    switch (fmt)
    {
        case DXGI_FORMAT_D32_FLOAT:
            std::memcpy(dst, &depth, 4);
            break;
        case DXGI_FORMAT_D16_UNORM:
        {
            UINT16 u = static_cast<UINT16>(std::clamp(depth, 0.f, 1.f) * 65535.f + 0.5f);
            std::memcpy(dst, &u, 2);
            break;
        }
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        {
            UINT32 existing;
            std::memcpy(&existing, dst, 4);
            UINT32 d24 = static_cast<UINT32>(std::clamp(depth, 0.f, 1.f) * 16777215.f + 0.5f);
            existing = (existing & 0xFF000000) | (d24 & 0x00FFFFFF);
            std::memcpy(dst, &existing, 4);
            break;
        }
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            std::memcpy(dst, &depth, 4);
            break;
        default:
            break;
    }
}

static UINT DepthPixelStride(DXGI_FORMAT fmt)
{
    switch (fmt)
    {
        case DXGI_FORMAT_D32_FLOAT:            return 4;
        case DXGI_FORMAT_D16_UNORM:            return 2;
        case DXGI_FORMAT_D24_UNORM_S8_UINT:    return 4;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT: return 8;
        default:                               return 4;
    }
}

static Bool DepthTest(D3D11_COMPARISON_FUNC func, Float src, Float dst)
{
    switch (func)
    {
        case D3D11_COMPARISON_NEVER:         return false;
        case D3D11_COMPARISON_LESS:          return src < dst;
        case D3D11_COMPARISON_EQUAL:         return src == dst;
        case D3D11_COMPARISON_LESS_EQUAL:    return src <= dst;
        case D3D11_COMPARISON_GREATER:       return src > dst;
        case D3D11_COMPARISON_NOT_EQUAL:     return src != dst;
        case D3D11_COMPARISON_GREATER_EQUAL: return src >= dst;
        case D3D11_COMPARISON_ALWAYS:        return true;
        default:                             return true;
    }
}

static void UnpackRTVColor(DXGI_FORMAT fmt, const UINT8* src, FLOAT rgba[4])
{
    rgba[0] = rgba[1] = rgba[2] = rgba[3] = 0.f;
    switch (fmt)
    {
        case DXGI_FORMAT_R8G8B8A8_UNORM:
        case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
            rgba[0] = src[0] / 255.f;
            rgba[1] = src[1] / 255.f;
            rgba[2] = src[2] / 255.f;
            rgba[3] = src[3] / 255.f;
            break;
        case DXGI_FORMAT_B8G8R8A8_UNORM:
        case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
            rgba[0] = src[2] / 255.f;
            rgba[1] = src[1] / 255.f;
            rgba[2] = src[0] / 255.f;
            rgba[3] = src[3] / 255.f;
            break;
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            std::memcpy(rgba, src, 16);
            break;
        case DXGI_FORMAT_R32G32_FLOAT:
            std::memcpy(rgba, src, 8);
            break;
        case DXGI_FORMAT_R32_FLOAT:
            std::memcpy(rgba, src, 4);
            break;
        case DXGI_FORMAT_R16G16B16A16_UNORM:
        {
            UINT16 v[4];
            std::memcpy(v, src, 8);
            rgba[0] = v[0] / 65535.f;
            rgba[1] = v[1] / 65535.f;
            rgba[2] = v[2] / 65535.f;
            rgba[3] = v[3] / 65535.f;
            break;
        }
        default:
            break;
    }
}

static Float ApplyBlendFactor(D3D11_BLEND factor, const FLOAT src[4], const FLOAT dst[4],
                               const FLOAT blendFactor[4], Int comp)
{
    switch (factor)
    {
        case D3D11_BLEND_ZERO:           return 0.f;
        case D3D11_BLEND_ONE:            return 1.f;
        case D3D11_BLEND_SRC_COLOR:      return src[comp];
        case D3D11_BLEND_INV_SRC_COLOR:  return 1.f - src[comp];
        case D3D11_BLEND_SRC_ALPHA:      return src[3];
        case D3D11_BLEND_INV_SRC_ALPHA:  return 1.f - src[3];
        case D3D11_BLEND_DEST_ALPHA:     return dst[3];
        case D3D11_BLEND_INV_DEST_ALPHA: return 1.f - dst[3];
        case D3D11_BLEND_DEST_COLOR:     return dst[comp];
        case D3D11_BLEND_INV_DEST_COLOR: return 1.f - dst[comp];
        case D3D11_BLEND_BLEND_FACTOR:     return blendFactor[comp];
        case D3D11_BLEND_INV_BLEND_FACTOR: return 1.f - blendFactor[comp];
        case D3D11_BLEND_SRC_ALPHA_SAT:
        {
            Float f = std::min(src[3], 1.f - dst[3]);
            return comp == 3 ? 1.f : f;
        }
        default: return 1.f;
    }
}

static Float BlendOp(D3D11_BLEND_OP op, Float srcTerm, Float dstTerm)
{
    switch (op)
    {
        case D3D11_BLEND_OP_ADD:          return srcTerm + dstTerm;
        case D3D11_BLEND_OP_SUBTRACT:     return srcTerm - dstTerm;
        case D3D11_BLEND_OP_REV_SUBTRACT: return dstTerm - srcTerm;
        case D3D11_BLEND_OP_MIN:          return std::min(srcTerm, dstTerm);
        case D3D11_BLEND_OP_MAX:          return std::max(srcTerm, dstTerm);
        default:                          return srcTerm + dstTerm;
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

static void WritePixelOutputs(
    Int px, Int py,
    const SW_PSOutput& psOut,
    D3D11SW_PIPELINE_STATE& state)
{
    D3D11_BLEND_DESC1 bsDesc{};
    Bool haveBlendState = false;
    if (state.blendState)
    {
        state.blendState->GetDesc1(&bsDesc);
        haveBlendState = true;
    }

    for (UINT rt = 0; rt < state.numRenderTargets; ++rt)
    {
        D3D11RenderTargetViewSW* rtv = state.renderTargets[rt];
        if (!rtv) { continue; }

        UINT8* rtvData     = rtv->GetDataPtr();
        DXGI_FORMAT rtvFmt = rtv->GetFormat();
        UINT rtvRowPitch   = rtv->GetLayout().RowPitch;
        UINT rtvPixStride  = rtv->GetLayout().PixelStride;

        UINT8* rtvPx = rtvData + (UINT64)py * rtvRowPitch + (UINT64)px * rtvPixStride;

        D3D11_RENDER_TARGET_BLEND_DESC1 blendDesc{};
        blendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (haveBlendState)
        {
            blendDesc = bsDesc.RenderTarget[rt];
        }

        FLOAT srcColor[4] = { psOut.oC[rt].x, psOut.oC[rt].y, psOut.oC[rt].z, psOut.oC[rt].w };
        FLOAT finalColor[4];

        if (blendDesc.BlendEnable)
        {
            FLOAT dstColor[4];
            UnpackRTVColor(rtvFmt, rtvPx, dstColor);

            for (Int c = 0; c < 3; ++c)
            {
                Float sf = ApplyBlendFactor(blendDesc.SrcBlend, srcColor, dstColor, state.blendFactor, c);
                Float df = ApplyBlendFactor(blendDesc.DestBlend, srcColor, dstColor, state.blendFactor, c);
                finalColor[c] = BlendOp(blendDesc.BlendOp, srcColor[c] * sf, dstColor[c] * df);
            }
            {
                Float sf = ApplyBlendFactor(blendDesc.SrcBlendAlpha, srcColor, dstColor, state.blendFactor, 3);
                Float df = ApplyBlendFactor(blendDesc.DestBlendAlpha, srcColor, dstColor, state.blendFactor, 3);
                finalColor[3] = BlendOp(blendDesc.BlendOpAlpha, srcColor[3] * sf, dstColor[3] * df);
            }
        }
        else
        {
            finalColor[0] = srcColor[0];
            finalColor[1] = srcColor[1];
            finalColor[2] = srcColor[2];
            finalColor[3] = srcColor[3];
        }

        UINT8 writeMask = blendDesc.RenderTargetWriteMask;
        if (writeMask != D3D11_COLOR_WRITE_ENABLE_ALL)
        {
            FLOAT existing[4];
            UnpackRTVColor(rtvFmt, rtvPx, existing);
            if (!(writeMask & D3D11_COLOR_WRITE_ENABLE_RED))   { finalColor[0] = existing[0]; }
            if (!(writeMask & D3D11_COLOR_WRITE_ENABLE_GREEN)) { finalColor[1] = existing[1]; }
            if (!(writeMask & D3D11_COLOR_WRITE_ENABLE_BLUE))  { finalColor[2] = existing[2]; }
            if (!(writeMask & D3D11_COLOR_WRITE_ENABLE_ALPHA)) { finalColor[3] = existing[3]; }
        }

        UINT8 packed[16];
        PackRTVColor(rtvFmt, finalColor, packed);
        std::memcpy(rtvPx, packed, rtvPixStride);
    }
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

    Int vpMinX = static_cast<Int>(vp.TopLeftX);
    Int vpMinY = static_cast<Int>(vp.TopLeftY);
    Int vpMaxX = static_cast<Int>(vp.TopLeftX + vp.Width);
    Int vpMaxY = static_cast<Int>(vp.TopLeftY + vp.Height);

    if (rsDesc.ScissorEnable && state.numScissorRects > 0)
    {
        const D3D11_RECT& sr = state.scissorRects[0];
        vpMinX = std::max(vpMinX, static_cast<Int>(sr.left));
        vpMinY = std::max(vpMinY, static_cast<Int>(sr.top));
        vpMaxX = std::min(vpMaxX, static_cast<Int>(sr.right));
        vpMaxY = std::min(vpMaxY, static_cast<Int>(sr.bottom));
    }

    minX = std::max(minX, vpMinX);
    minY = std::max(minY, vpMinY);
    maxX = std::min(maxX, vpMaxX);
    maxY = std::min(maxY, vpMaxY);

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

    Float invArea2 = 1.f / static_cast<Float>(fixedArea2);

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

    SW_float4 v0pw[D3D11_VS_OUTPUT_REGISTER_COUNT];
    SW_float4 v1pw[D3D11_VS_OUTPUT_REGISTER_COUNT];
    SW_float4 v2pw[D3D11_VS_OUTPUT_REGISTER_COUNT];
    for (Int vi = 0; vi < numVaryings; ++vi)
    {
        Int vsR = varyings[vi].vsOutReg;
        v0pw[vi] = { v0.o[vsR].x * iw0, v0.o[vsR].y * iw0, v0.o[vsR].z * iw0, v0.o[vsR].w * iw0 };
        v1pw[vi] = { v1.o[vsR].x * iw1, v1.o[vsR].y * iw1, v1.o[vsR].z * iw1, v1.o[vsR].w * iw1 };
        v2pw[vi] = { v2.o[vsR].x * iw2, v2.o[vsR].y * iw2, v2.o[vsR].z * iw2, v2.o[vsR].w * iw2 };
    }

    Bool depthEnabled = true;
    D3D11_COMPARISON_FUNC depthFunc = D3D11_COMPARISON_LESS;
    D3D11_DEPTH_WRITE_MASK depthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    if (state.depthStencilState)
    {
        D3D11_DEPTH_STENCIL_DESC dsDesc{};
        state.depthStencilState->GetDesc(&dsDesc);
        depthEnabled   = dsDesc.DepthEnable ? true : false;
        depthFunc      = dsDesc.DepthFunc;
        depthWriteMask = dsDesc.DepthWriteMask;
    }

    D3D11DepthStencilViewSW* dsv = state.depthStencilView;
    UINT8* dsvData     = dsv ? dsv->GetDataPtr() : nullptr;
    DXGI_FORMAT dsvFmt = dsv ? dsv->GetFormat()  : DXGI_FORMAT_UNKNOWN;
    UINT dsvRowPitch   = dsv ? dsv->GetLayout().RowPitch : 0;
    UINT dsvPixStride  = dsv ? DepthPixelStride(dsvFmt) : 0;

    if (!dsv) { depthEnabled = false; }

    if (state.numRenderTargets == 0) { return; }

    Float diw0 = iw0 - iw2;
    Float diw1 = iw1 - iw2;
    Float dz0 = ndcZ[0] - ndcZ[2];
    Float dz1 = ndcZ[1] - ndcZ[2];

    D3D11_BLEND_DESC1 bsDesc{};
    Bool haveBlendState = false;
    if (state.blendState)
    {
        state.blendState->GetDesc1(&bsDesc);
        haveBlendState = true;
    }

    struct RTInfo
    {
        UINT8*                          data;
        DXGI_FORMAT                     fmt;
        UINT                            rowPitch;
        UINT                            pixStride;
        D3D11_RENDER_TARGET_BLEND_DESC1 blendDesc;
    };

    RTInfo rtInfos[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
    UINT activeRTCount = 0;
    for (UINT rt = 0; rt < state.numRenderTargets; ++rt)
    {
        D3D11RenderTargetViewSW* rtv = state.renderTargets[rt];
        if (!rtv) { continue; }
        RTInfo& info    = rtInfos[activeRTCount++];
        info.data       = rtv->GetDataPtr();
        info.fmt        = rtv->GetFormat();
        info.rowPitch   = rtv->GetLayout().RowPitch;
        info.pixStride  = rtv->GetLayout().PixelStride;
        info.blendDesc  = {};
        info.blendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (haveBlendState) { info.blendDesc = bsDesc.RenderTarget[rt]; }
    }

    const Int TILE = _config.tileSize;
    const Bool useTiling = _config.tiling;

    Int tileMinX = useTiling ? (minX & ~(TILE - 1)) : minX;
    Int tileMinY = useTiling ? (minY & ~(TILE - 1)) : minY;
    Int tileStepX = useTiling ? TILE : (maxX - minX);
    Int tileStepY = useTiling ? TILE : (maxY - minY);

    I64 tileStartX = static_cast<I64>(tileMinX) * 16 + 8;
    I64 tileStartY = static_cast<I64>(tileMinY) * 16 + 8;

    I64 w0_dx = -(fy[2] - fy[1]) * 16;
    I64 w1_dx = -(fy[0] - fy[2]) * 16;
    I64 w2_dx = -(fy[1] - fy[0]) * 16;

    I64 w0_dy = (fx[2] - fx[1]) * 16;
    I64 w1_dy = (fx[0] - fx[2]) * 16;
    I64 w2_dy = (fx[1] - fx[0]) * 16;

    I64 w0_tileOrig = edgeFn(fx[1], fy[1], fx[2], fy[2], tileStartX, tileStartY) + bias0;
    I64 w1_tileOrig = edgeFn(fx[2], fy[2], fx[0], fy[0], tileStartX, tileStartY) + bias1;
    I64 w2_tileOrig = edgeFn(fx[0], fy[0], fx[1], fy[1], tileStartX, tileStartY) + bias2;

    I64 w0_tileStepX = w0_dx * TILE;
    I64 w1_tileStepX = w1_dx * TILE;
    I64 w2_tileStepX = w2_dx * TILE;

    I64 w0_tileStepY = w0_dy * TILE;
    I64 w1_tileStepY = w1_dy * TILE;
    I64 w2_tileStepY = w2_dy * TILE;

    I64 w0_cornerDx = w0_dx * (TILE - 1);
    I64 w1_cornerDx = w1_dx * (TILE - 1);
    I64 w2_cornerDx = w2_dx * (TILE - 1);

    I64 w0_cornerDy = w0_dy * (TILE - 1);
    I64 w1_cornerDy = w1_dy * (TILE - 1);
    I64 w2_cornerDy = w2_dy * (TILE - 1);

    SW_PSInput psIn{};
    SW_PSOutput psOut{};

    I64 w0_tileRowStart = w0_tileOrig;
    I64 w1_tileRowStart = w1_tileOrig;
    I64 w2_tileRowStart = w2_tileOrig;
    for (Int tileY = tileMinY; tileY < maxY; tileY += tileStepY)
    {
        I64 w0_tile = w0_tileRowStart;
        I64 w1_tile = w1_tileRowStart;
        I64 w2_tile = w2_tileRowStart;

        for (Int tileX = tileMinX; tileX < maxX; tileX += tileStepX)
        {
            Bool allOutside = false;
            Bool fullyCovered = false;

            if (useTiling)
            {
                I64 w0_TL = w0_tile;
                I64 w1_TL = w1_tile;
                I64 w2_TL = w2_tile;

                I64 w0_TR = w0_TL + w0_cornerDx;
                I64 w1_TR = w1_TL + w1_cornerDx;
                I64 w2_TR = w2_TL + w2_cornerDx;

                I64 w0_BL = w0_TL + w0_cornerDy;
                I64 w1_BL = w1_TL + w1_cornerDy;
                I64 w2_BL = w2_TL + w2_cornerDy;

                I64 w0_BR = w0_TR + w0_cornerDy;
                I64 w1_BR = w1_TR + w1_cornerDy;
                I64 w2_BR = w2_TR + w2_cornerDy;

                allOutside =
                    (w0_TL < 0 && w0_TR < 0 && w0_BL < 0 && w0_BR < 0) ||
                    (w1_TL < 0 && w1_TR < 0 && w1_BL < 0 && w1_BR < 0) ||
                    (w2_TL < 0 && w2_TR < 0 && w2_BL < 0 && w2_BR < 0);

                fullyCovered =
                    (w0_TL >= 0 && w0_TR >= 0 && w0_BL >= 0 && w0_BR >= 0) &&
                    (w1_TL >= 0 && w1_TR >= 0 && w1_BL >= 0 && w1_BR >= 0) &&
                    (w2_TL >= 0 && w2_TR >= 0 && w2_BL >= 0 && w2_BR >= 0);
            }

            if (allOutside)
            {
                w0_tile += w0_tileStepX;
                w1_tile += w1_tileStepX;
                w2_tile += w2_tileStepX;
                continue;
            }

            Int tMinX = std::max(tileX, minX);
            Int tMaxX = std::min(tileX + tileStepX, maxX);
            Int tMinY = std::max(tileY, minY);
            Int tMaxY = std::min(tileY + tileStepY, maxY);

            I64 w0_pixRowStart = w0_tile + w0_dx * (tMinX - tileX) + w0_dy * (tMinY - tileY);
            I64 w1_pixRowStart = w1_tile + w1_dx * (tMinX - tileX) + w1_dy * (tMinY - tileY);
            I64 w2_pixRowStart = w2_tile + w2_dx * (tMinX - tileX) + w2_dy * (tMinY - tileY);

            for (Int py = tMinY; py < tMaxY; ++py)
            {
                I64 w0 = w0_pixRowStart;
                I64 w1 = w1_pixRowStart;
                I64 w2 = w2_pixRowStart;

                UINT8* dsvRow = depthEnabled ? (dsvData + (UINT64)py * dsvRowPitch) : nullptr;

                UINT8* rtRows[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
                for (UINT rt = 0; rt < activeRTCount; ++rt)
                {
                    rtRows[rt] = rtInfos[rt].data + (UINT64)py * rtInfos[rt].rowPitch;
                }

                Float pyCenter = static_cast<Float>(py) + 0.5f;

                for (Int px = tMinX; px < tMaxX; ++px)
                {
                    if (fullyCovered || (w0 >= 0 && w1 >= 0 && w2 >= 0))
                    {
                        Float b0 = static_cast<Float>(w0) * invArea2;
                        Float b1 = static_cast<Float>(w1) * invArea2;

                        Float perspW = iw2 + b0 * diw0 + b1 * diw1;
                        Float invPerspW = (perspW != 0.f) ? 1.f / perspW : 0.f;

                        Float depth = ndcZ[2] + b0 * dz0 + b1 * dz1;
                        depth = std::clamp(depth, vp.MinDepth, vp.MaxDepth);

                        if (depthEnabled)
                        {
                            UINT8* dsvPx = dsvRow + (UINT64)px * dsvPixStride;
                            Float existing = ReadDepth(dsvFmt, dsvPx);
                            if (!DepthTest(depthFunc, depth, existing))
                            {
                                w0 += w0_dx;
                                w1 += w1_dx;
                                w2 += w2_dx;
                                continue;
                            }
                        }

                        Float pxCenter = static_cast<Float>(px) + 0.5f;

                        psIn.pos = { pxCenter, pyCenter, depth, perspW };
                        if (svPosRegPS >= 0)
                        {
                            psIn.v[svPosRegPS] = psIn.pos;
                        }

                        Float b2 = 1.f - b0 - b1;
                        for (Int vi = 0; vi < numVaryings; ++vi)
                        {
                            Int psR = varyings[vi].psInReg;

                            Float ax = (b0 * v0pw[vi].x + b1 * v1pw[vi].x + b2 * v2pw[vi].x) * invPerspW;
                            Float ay = (b0 * v0pw[vi].y + b1 * v1pw[vi].y + b2 * v2pw[vi].y) * invPerspW;
                            Float az = (b0 * v0pw[vi].z + b1 * v1pw[vi].z + b2 * v2pw[vi].z) * invPerspW;
                            Float aw = (b0 * v0pw[vi].w + b1 * v1pw[vi].w + b2 * v2pw[vi].w) * invPerspW;

                            psIn.v[psR] = { ax, ay, az, aw };
                        }

                        psOut = {};
                        psFn(&psIn, &psOut, &psRes);

                        if (depthEnabled && depthWriteMask == D3D11_DEPTH_WRITE_MASK_ALL)
                        {
                            Float writeD = psOut.depthWritten ? psOut.oDepth : depth;
                            UINT8* dsvPx = dsvRow + (UINT64)px * dsvPixStride;
                            WriteDepth(dsvFmt, dsvPx, writeD);
                        }

                        for (UINT rt = 0; rt < activeRTCount; ++rt)
                        {
                            RTInfo& info = rtInfos[rt];
                            UINT8* rtvPx = rtRows[rt] + (UINT64)px * info.pixStride;

                            FLOAT srcColor[4] = { psOut.oC[rt].x, psOut.oC[rt].y, psOut.oC[rt].z, psOut.oC[rt].w };
                            FLOAT finalColor[4];

                            if (info.blendDesc.BlendEnable)
                            {
                                FLOAT dstColor[4];
                                UnpackRTVColor(info.fmt, rtvPx, dstColor);

                                for (Int c = 0; c < 3; ++c)
                                {
                                    Float sf = ApplyBlendFactor(info.blendDesc.SrcBlend, srcColor, dstColor, state.blendFactor, c);
                                    Float df = ApplyBlendFactor(info.blendDesc.DestBlend, srcColor, dstColor, state.blendFactor, c);
                                    finalColor[c] = BlendOp(info.blendDesc.BlendOp, srcColor[c] * sf, dstColor[c] * df);
                                }
                                {
                                    Float sf = ApplyBlendFactor(info.blendDesc.SrcBlendAlpha, srcColor, dstColor, state.blendFactor, 3);
                                    Float df = ApplyBlendFactor(info.blendDesc.DestBlendAlpha, srcColor, dstColor, state.blendFactor, 3);
                                    finalColor[3] = BlendOp(info.blendDesc.BlendOpAlpha, srcColor[3] * sf, dstColor[3] * df);
                                }
                            }
                            else
                            {
                                finalColor[0] = srcColor[0];
                                finalColor[1] = srcColor[1];
                                finalColor[2] = srcColor[2];
                                finalColor[3] = srcColor[3];
                            }

                            UINT8 writeMask = info.blendDesc.RenderTargetWriteMask;
                            if (writeMask != D3D11_COLOR_WRITE_ENABLE_ALL)
                            {
                                FLOAT existing[4];
                                UnpackRTVColor(info.fmt, rtvPx, existing);
                                if (!(writeMask & D3D11_COLOR_WRITE_ENABLE_RED))   { finalColor[0] = existing[0]; }
                                if (!(writeMask & D3D11_COLOR_WRITE_ENABLE_GREEN)) { finalColor[1] = existing[1]; }
                                if (!(writeMask & D3D11_COLOR_WRITE_ENABLE_BLUE))  { finalColor[2] = existing[2]; }
                                if (!(writeMask & D3D11_COLOR_WRITE_ENABLE_ALPHA)) { finalColor[3] = existing[3]; }
                            }

                            UINT8 packed[16];
                            PackRTVColor(info.fmt, finalColor, packed);
                            std::memcpy(rtvPx, packed, info.pixStride);
                        }
                    }

                    w0 += w0_dx;
                    w1 += w1_dx;
                    w2 += w2_dx;
                }

                w0_pixRowStart += w0_dy;
                w1_pixRowStart += w1_dy;
                w2_pixRowStart += w2_dy;
            }

            w0_tile += w0_tileStepX;
            w1_tile += w1_tileStepX;
            w2_tile += w2_tileStepX;
        }

        w0_tileRowStart += w0_tileStepY;
        w1_tileRowStart += w1_tileStepY;
        w2_tileRowStart += w2_tileStepY;
    }
}

SW_VSOutput SWRasterizer::RunVS(
    UINT vertIdx, SW_VSFn vsFn,
    const D3D11SW_ParsedShader& vsReflection,
    SW_Resources& vsRes,
    const D3D11SW_PIPELINE_STATE& state)
{
    SW_VSInput vsIn{};
    FetchVertex(vsIn, vertIdx, state, vsReflection);

    SW_VSOutput vsOut{};
    vsFn(&vsIn, &vsOut, &vsRes);
    return vsOut;
}

void SWRasterizer::RasterizeLine(
    const SW_VSOutput endpts[2],
    const D3D11SW_ParsedShader& vsReflection,
    const D3D11SW_ParsedShader& psReflection,
    SW_PSFn psFn,
    SW_Resources& psRes,
    D3D11SW_PIPELINE_STATE& state)
{
    if (state.numViewports == 0) { return; }
    const D3D11_VIEWPORT& vp = state.viewports[0];

    for (Int v = 0; v < 2; ++v)
    {
        if (endpts[v].pos.w <= 0.f) { return; }
    }

    Float screenX[2], screenY[2];
    for (Int v = 0; v < 2; ++v)
    {
        Float invW = 1.f / endpts[v].pos.w;
        Float ndcX = endpts[v].pos.x * invW;
        Float ndcY = endpts[v].pos.y * invW;

        screenX[v] = (ndcX * 0.5f + 0.5f) * vp.Width  + vp.TopLeftX;
        screenY[v] = (1.f - (ndcY * 0.5f + 0.5f)) * vp.Height + vp.TopLeftY;
    }

    Int vpMinX = static_cast<Int>(vp.TopLeftX);
    Int vpMinY = static_cast<Int>(vp.TopLeftY);
    Int vpMaxX = static_cast<Int>(vp.TopLeftX + vp.Width);
    Int vpMaxY = static_cast<Int>(vp.TopLeftY + vp.Height);

    if (state.numRenderTargets == 0) { return; }

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

    Float dx = screenX[1] - screenX[0];
    Float dy = screenY[1] - screenY[0];
    Int steps = static_cast<Int>(std::max(std::fabs(dx), std::fabs(dy)));
    if (steps == 0) { steps = 1; }

    Float xInc = dx / steps;
    Float yInc = dy / steps;

    for (Int s = 0; s <= steps; ++s)
    {
        Float fx = screenX[0] + xInc * s;
        Float fy = screenY[0] + yInc * s;
        Int px = static_cast<Int>(fx);
        Int py = static_cast<Int>(fy);

        if (px < vpMinX || px >= vpMaxX || py < vpMinY || py >= vpMaxY) { continue; }

        Float t = static_cast<Float>(s) / static_cast<Float>(steps);

        SW_PSInput psIn{};
        if (svPosRegPS >= 0)
        {
            psIn.v[svPosRegPS] = { fx + 0.5f, fy + 0.5f, 0.f, 1.f };
        }
        psIn.pos = { fx + 0.5f, fy + 0.5f, 0.f, 1.f };

        for (Int vi = 0; vi < numVaryings; ++vi)
        {
            Int vsR = varyings[vi].vsOutReg;
            Int psR = varyings[vi].psInReg;
            const SW_float4& a = endpts[0].o[vsR];
            const SW_float4& b = endpts[1].o[vsR];
            psIn.v[psR] = {
                a.x + (b.x - a.x) * t,
                a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t,
                a.w + (b.w - a.w) * t,
            };
        }

        SW_PSOutput psOut{};
        psFn(&psIn, &psOut, &psRes);

        WritePixelOutputs(px, py, psOut, state);
    }
}

void SWRasterizer::RasterizePoint(
    const SW_VSOutput& point,
    const D3D11SW_ParsedShader& vsReflection,
    const D3D11SW_ParsedShader& psReflection,
    SW_PSFn psFn,
    SW_Resources& psRes,
    D3D11SW_PIPELINE_STATE& state)
{
    if (state.numViewports == 0) { return; }
    const D3D11_VIEWPORT& vp = state.viewports[0];

    if (point.pos.w <= 0.f) { return; }

    Float invW = 1.f / point.pos.w;
    Float ndcX = point.pos.x * invW;
    Float ndcY = point.pos.y * invW;

    Float sx = (ndcX * 0.5f + 0.5f) * vp.Width  + vp.TopLeftX;
    Float sy = (1.f - (ndcY * 0.5f + 0.5f)) * vp.Height + vp.TopLeftY;

    Int px = static_cast<Int>(sx);
    Int py = static_cast<Int>(sy);

    Int vpMinX = static_cast<Int>(vp.TopLeftX);
    Int vpMinY = static_cast<Int>(vp.TopLeftY);
    Int vpMaxX = static_cast<Int>(vp.TopLeftX + vp.Width);
    Int vpMaxY = static_cast<Int>(vp.TopLeftY + vp.Height);

    if (px < vpMinX || px >= vpMaxX || py < vpMinY || py >= vpMaxY) { return; }

    if (state.numRenderTargets == 0) { return; }

    Int svPosRegPS = FindSVPositionInput(psReflection);

    SW_PSInput psIn{};
    if (svPosRegPS >= 0)
    {
        psIn.v[svPosRegPS] = { sx + 0.5f, sy + 0.5f, 0.f, 1.f };
    }
    psIn.pos = { sx + 0.5f, sy + 0.5f, 0.f, 1.f };

    struct VaryingMap { Int vsOutReg; Int psInReg; };
    VaryingMap varyings[D3D11_VS_OUTPUT_REGISTER_COUNT];
    Int numVaryings = 0;
    for (const auto& pi : psReflection.inputs)
    {
        if (pi.svType != 0) { continue; }
        Int vsOutReg = FindSemanticRegister(vsReflection.outputs, pi.name, pi.semanticIndex);
        if (vsOutReg >= 0)
        {
            varyings[numVaryings++] = { vsOutReg, static_cast<Int>(pi.reg) };
        }
    }

    for (Int vi = 0; vi < numVaryings; ++vi)
    {
        psIn.v[varyings[vi].psInReg] = point.o[varyings[vi].vsOutReg];
    }

    SW_PSOutput psOut{};
    psFn(&psIn, &psOut, &psRes);

    WritePixelOutputs(px, py, psOut, state);
}

void SWRasterizer::DrawInternal(
    const UINT* indices, UINT vertexCount, INT baseVertex,
    D3D11SW_PIPELINE_STATE& state)
{
    if (!state.vs || !state.ps) { return; }

    SW_VSFn vsFn = state.vs->GetJitFn();
    SW_PSFn psFn = state.ps->GetJitFn();
    if (!vsFn || !psFn) { return; }

    const D3D11SW_ParsedShader& vsRefl = state.vs->GetReflection();
    const D3D11SW_ParsedShader& psRefl = state.ps->GetReflection();

    SW_Resources vsRes{};
    BuildStageResources(vsRes, state.vsCBs, state.vsSRVs, state.vsSamplers);

    SW_Resources psRes{};
    BuildStageResources(psRes, state.psCBs, state.psSRVs, state.psSamplers);

    auto fetchVS = [&](UINT i) -> SW_VSOutput
    {
        UINT vertIdx = indices ? (indices[i] + baseVertex) : (i + baseVertex);
        return RunVS(vertIdx, vsFn, vsRefl, vsRes, state);
    };

    switch (state.topology)
    {
    case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
    {
        for (UINT i = 0; i + 2 < vertexCount; i += 3)
        {
            SW_VSOutput tri[3] = { fetchVS(i), fetchVS(i + 1), fetchVS(i + 2) };
            RasterizeTriangle(tri, vsRefl, psRefl, psFn, psRes, state);
        }
        break;
    }
    case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
    {
        for (UINT i = 0; i + 2 < vertexCount; ++i)
        {
            SW_VSOutput tri[3];
            if (i & 1)
            {
                tri[0] = fetchVS(i + 1);
                tri[1] = fetchVS(i);
                tri[2] = fetchVS(i + 2);
            }
            else
            {
                tri[0] = fetchVS(i);
                tri[1] = fetchVS(i + 1);
                tri[2] = fetchVS(i + 2);
            }
            RasterizeTriangle(tri, vsRefl, psRefl, psFn, psRes, state);
        }
        break;
    }
    case D3D11_PRIMITIVE_TOPOLOGY_LINELIST:
    {
        for (UINT i = 0; i + 1 < vertexCount; i += 2)
        {
            SW_VSOutput endpts[2] = { fetchVS(i), fetchVS(i + 1) };
            RasterizeLine(endpts, vsRefl, psRefl, psFn, psRes, state);
        }
        break;
    }
    case D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP:
    {
        if (vertexCount < 2) { break; }
        SW_VSOutput prev = fetchVS(0);
        for (UINT i = 1; i < vertexCount; ++i)
        {
            SW_VSOutput cur = fetchVS(i);
            SW_VSOutput endpts[2] = { prev, cur };
            RasterizeLine(endpts, vsRefl, psRefl, psFn, psRes, state);
            prev = cur;
        }
        break;
    }
    case D3D11_PRIMITIVE_TOPOLOGY_POINTLIST:
    {
        for (UINT i = 0; i < vertexCount; ++i)
        {
            SW_VSOutput pt = fetchVS(i);
            RasterizePoint(pt, vsRefl, psRefl, psFn, psRes, state);
        }
        break;
    }
    default:
        break;
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
