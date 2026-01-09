#include "context/rasterizer.h"
#include "context/context_util.h"
#include "context/pipeline_state.h"
#include "misc/input_layout.h"
#include "shaders/vertex_shader.h"
#include "shaders/pixel_shader.h"
#include "states/blend_state.h"
#include "states/depth_stencil_state.h"
#include "states/rasterizer_state.h"
#include "states/sampler_state.h"
#include "views/render_target_view.h"
#include "views/depth_stencil_view.h"
#include "views/shader_resource_view.h"
#include "resources/buffer.h"
#include "util/env.h"
#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstring>
#include <latch>
#include <semaphore>
#include <thread>
#include <vector>
#include <d3dcommon.h>

namespace d3d11sw {

SWRasterizer::Config SWRasterizer::Config::FromEnv()
{
    Config cfg;
    cfg.tiling       = GetEnvBool("D3D11SW_TILING", true);
    cfg.tileSize     = GetEnvInt("D3D11SW_TILE_SIZE", 16);
    cfg.tileThreads  = GetEnvInt("D3D11SW_TILE_THREADS", -1);
    cfg.guardBandK   = GetEnvFloat("D3D11SW_GUARD_BAND_K", 16.f);
    if (cfg.tileSize < 4 || (cfg.tileSize & (cfg.tileSize - 1)) != 0)
    {
        cfg.tileSize = 16;
    }
    if (cfg.tileThreads == -1)
    {
        cfg.tileThreads = static_cast<Int>(std::thread::hardware_concurrency());
        if (cfg.tileThreads < 1) { cfg.tileThreads = 1; }
    }
    if (cfg.guardBandK < 1.f) { cfg.guardBandK = 1.f; }
    return cfg;
}

class TileThreadPool
{
public:
    explicit TileThreadPool(Uint32 n) : _n(n)
    {
        for (Uint32 i = 0; i < n; ++i)
        {
            _threads.emplace_back([this]() { Run(); });
        }
    }

    ~TileThreadPool()
    {
        _stop.store(true, std::memory_order_release);
        _ready.release(_n);
        for (auto& t : _threads)
        {
            t.join();
        }
    }

    Uint32 Size() const { return _n; }

    using TileFn = void(*)(void* ctx, Uint32 tileIdx);
    void Dispatch(Uint32 count, TileFn fn, void* ctx)
    {
        _fn    = fn;
        _ctx   = ctx;
        _count = count;
        _next.store(0, std::memory_order_relaxed);

        std::latch done(_n + 1);
        _done = &done;
        _ready.release(_n);

        while (true)
        {
            Uint32 idx = _next.fetch_add(1, std::memory_order_relaxed);
            if (idx >= _count) { break; }
            _fn(_ctx, idx);
        }
        done.count_down();
        done.wait();
    }

private:
    Uint32                     _n;
    std::vector<std::thread>   _threads;
    std::counting_semaphore<>  _ready{0};
    std::atomic<Bool>          _stop{false};
    std::latch*                _done{nullptr};
    TileFn                     _fn{nullptr};
    void*                      _ctx{nullptr};
    Uint32                     _count{0};
    std::atomic<Uint32>        _next{0};

private:
    void Run()
    {
        while (true)
        {
            _ready.acquire();
            if (_stop.load(std::memory_order_acquire)) { return; }

            while (true)
            {
                Uint32 idx = _next.fetch_add(1, std::memory_order_relaxed);
                if (idx >= _count) { break; }
                _fn(_ctx, idx);
            }
            _done->count_down();
        }
    }
};

SWRasterizer::SWRasterizer() : _config(Config::FromEnv()) {}
SWRasterizer::~SWRasterizer() = default;

SWRasterizer::OMState SWRasterizer::InitOM(D3D11SW_PIPELINE_STATE& state)
{
    OMState om{};
    om.depthEnabled = true;
    om.depthFunc = D3D11_COMPARISON_LESS;
    om.depthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    om.stencilEnabled = false;
    om.stencilReadMask = 0xFF;
    om.stencilWriteMask = 0xFF;
    om.stencilRef = 0;
    om.stencilFront = {};
    om.stencilBack = {};

    if (state.depthStencilState)
    {
        D3D11_DEPTH_STENCIL_DESC dsDesc{};
        state.depthStencilState->GetDesc(&dsDesc);
        om.depthEnabled   = dsDesc.DepthEnable ? true : false;
        om.depthFunc      = dsDesc.DepthFunc;
        om.depthWriteMask = dsDesc.DepthWriteMask;
        om.stencilEnabled   = dsDesc.StencilEnable ? true : false;
        om.stencilReadMask  = dsDesc.StencilReadMask;
        om.stencilWriteMask = dsDesc.StencilWriteMask;
        om.stencilFront     = dsDesc.FrontFace;
        om.stencilBack      = dsDesc.BackFace;
    }
    om.stencilRef = static_cast<UINT8>(state.stencilRef);

    D3D11DepthStencilViewSW* dsv = state.depthStencilView;
    om.dsvData     = dsv ? dsv->GetDataPtr() : nullptr;
    om.dsvFmt      = dsv ? dsv->GetFormat()  : DXGI_FORMAT_UNKNOWN;
    om.dsvRowPitch = dsv ? dsv->GetLayout().RowPitch : 0;
    om.dsvPixStride = dsv ? DepthPixelStride(om.dsvFmt) : 0;
    if (!dsv) { om.depthEnabled = false; }
    if (!dsv || !FormatHasStencil(om.dsvFmt)) { om.stencilEnabled = false; }

    D3D11_BLEND_DESC1 bsDesc{};
    Bool haveBlendState = false;
    if (state.blendState)
    {
        state.blendState->GetDesc1(&bsDesc);
        haveBlendState = true;
    }

    om.activeRTCount = 0;
    for (UINT rt = 0; rt < state.numRenderTargets; ++rt)
    {
        D3D11RenderTargetViewSW* rtv = state.renderTargets[rt];
        if (!rtv) { continue; }
        RTInfo& info    = om.rtInfos[om.activeRTCount++];
        info.data       = rtv->GetDataPtr();
        info.fmt        = rtv->GetFormat();
        info.rowPitch   = rtv->GetLayout().RowPitch;
        info.pixStride  = rtv->GetLayout().PixelStride;
        info.blendDesc  = {};
        info.blendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (haveBlendState) { info.blendDesc = bsDesc.RenderTarget[rt]; }
    }

    om.blendFactor = state.blendFactor;
    return om;
}

SWRasterizer::VertexState SWRasterizer::InitVS(const D3D11SW_PIPELINE_STATE& state)
{
    VertexState vs{};
    vs.vsFn = state.vs ? state.vs->GetJitFn() : nullptr;
    vs.vsReflection = state.vs ? &state.vs->GetReflection() : nullptr;
    vs.vsRes = {};
    vs.state = &state;
    vs.cacheEnabled = GetEnvBool("D3D11SW_VERTEX_CACHE", false);
    vs.instanceID = 0;

    if (state.vs)
    {
        BuildStageResources(vs.vsRes, state.vsCBs, state.vsSRVs, state.vsSamplers);
    }
    return vs;
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
        case DXGI_FORMAT_R16G16B16A16_FLOAT:
        {
            auto toF = [](UINT16 h) -> Float
            {
                UINT32 sign = (h >> 15) & 1;
                UINT32 exp  = (h >> 10) & 0x1F;
                UINT32 man  = h & 0x3FF;
                if (exp == 0)
                {
                    if (man == 0) { return sign ? -0.f : 0.f; }
                    Float f = std::ldexp(static_cast<Float>(man), -24);
                    return sign ? -f : f;
                }
                if (exp == 31)
                {
                    if (man == 0) { return sign ? -INFINITY : INFINITY; }
                    return NAN;
                }
                Float f = std::ldexp(static_cast<Float>(man + 1024), static_cast<Int>(exp) - 25);
                return sign ? -f : f;
            };
            UINT16 v[4];
            std::memcpy(v, src, 8);
            out = { toF(v[0]), toF(v[1]), toF(v[2]), toF(v[3]) };
            break;
        }
        case DXGI_FORMAT_R16G16_FLOAT:
        {
            auto toF = [](UINT16 h) -> Float
            {
                UINT32 sign = (h >> 15) & 1;
                UINT32 exp  = (h >> 10) & 0x1F;
                UINT32 man  = h & 0x3FF;
                if (exp == 0)
                {
                    if (man == 0) { return sign ? -0.f : 0.f; }
                    Float f = std::ldexp(static_cast<Float>(man), -24);
                    return sign ? -f : f;
                }
                if (exp == 31)
                {
                    if (man == 0) { return sign ? -INFINITY : INFINITY; }
                    return NAN;
                }
                Float f = std::ldexp(static_cast<Float>(man + 1024), static_cast<Int>(exp) - 25);
                return sign ? -f : f;
            };
            UINT16 v[2];
            std::memcpy(v, src, 4);
            out = { toF(v[0]), toF(v[1]), 0.f, 0.f };
            break;
        }
        case DXGI_FORMAT_R8G8B8A8_UNORM:
            out = { src[0] / 255.f, src[1] / 255.f, src[2] / 255.f, src[3] / 255.f };
            break;
        case DXGI_FORMAT_R8G8B8A8_SNORM:
        {
            auto toF = [](UINT8 b) -> Float
            {
                return std::max(static_cast<Float>(std::bit_cast<Int8>(b)) / 127.f, -1.f);
            };
            out = { toF(src[0]), toF(src[1]), toF(src[2]), toF(src[3]) };
            break;
        }
        case DXGI_FORMAT_R8G8B8A8_UINT:
        {
            out = {
                std::bit_cast<Float>(static_cast<Uint32>(src[0])),
                std::bit_cast<Float>(static_cast<Uint32>(src[1])),
                std::bit_cast<Float>(static_cast<Uint32>(src[2])),
                std::bit_cast<Float>(static_cast<Uint32>(src[3]))
            };
            break;
        }
        case DXGI_FORMAT_R8G8B8A8_SINT:
        {
            auto toF = [](UINT8 b) -> Float
            {
                return std::bit_cast<Float>(static_cast<Uint32>(static_cast<Int32>(std::bit_cast<Int8>(b))));
            };
            out = { toF(src[0]), toF(src[1]), toF(src[2]), toF(src[3]) };
            break;
        }
        case DXGI_FORMAT_R32G32B32A32_UINT:
        case DXGI_FORMAT_R32G32B32A32_SINT:
        {
            Uint32 v[4];
            std::memcpy(v, src, 16);
            out = {
                std::bit_cast<Float>(v[0]),
                std::bit_cast<Float>(v[1]),
                std::bit_cast<Float>(v[2]),
                std::bit_cast<Float>(v[3])
            };
            break;
        }
        case DXGI_FORMAT_R10G10B10A2_UNORM:
        {
            UINT32 packed;
            std::memcpy(&packed, src, 4);
            out = {
                (packed & 0x3FF) / 1023.f,
                ((packed >> 10) & 0x3FF) / 1023.f,
                ((packed >> 20) & 0x3FF) / 1023.f,
                ((packed >> 30) & 0x3) / 3.f
            };
            break;
        }
        case DXGI_FORMAT_R16G16_SNORM:
        {
            Int16 v[2];
            std::memcpy(v, src, 4);
            out = { std::max(v[0] / 32767.f, -1.f), std::max(v[1] / 32767.f, -1.f), 0.f, 0.f };
            break;
        }
        case DXGI_FORMAT_R16G16_UNORM:
        {
            UINT16 v[2];
            std::memcpy(v, src, 4);
            out = { v[0] / 65535.f, v[1] / 65535.f, 0.f, 0.f };
            break;
        }
        case DXGI_FORMAT_R16G16_UINT:
        {
            UINT16 v[2];
            std::memcpy(v, src, 4);
            out = {
                std::bit_cast<Float>(static_cast<Uint32>(v[0])),
                std::bit_cast<Float>(static_cast<Uint32>(v[1])),
                0.f, 0.f
            };
            break;
        }
        case DXGI_FORMAT_R16G16_SINT:
        {
            Int16 v[2];
            std::memcpy(v, src, 4);
            out = {
                std::bit_cast<Float>(static_cast<Uint32>(static_cast<Int32>(v[0]))),
                std::bit_cast<Float>(static_cast<Uint32>(static_cast<Int32>(v[1]))),
                0.f, 0.f
            };
            break;
        }
        default:
            break;
    }
}

void SWRasterizer::FetchVertex(const VertexState& vs, SW_VSInput& vsIn, UINT vertexIndex)
{
    if (!vs.state->inputLayout)
    {
        return;
    }

    const auto& elements = vs.state->inputLayout->GetElements();
    for (const auto& elem : elements)
    {
        UINT slot = elem.InputSlot;
        D3D11BufferSW* vb = vs.state->vertexBuffers[slot];
        if (!vb)
        {
            continue;
        }

        UINT stride = vs.state->vbStrides[slot];
        UINT index = vertexIndex;
        if (elem.InputSlotClass == D3D11_INPUT_PER_INSTANCE_DATA)
        {
            UINT step = elem.InstanceDataStepRate;
            index = step ? (vs.instanceID / step) : 0;
        }
        UINT offset = vs.state->vbOffsets[slot] + elem.AlignedByteOffset + index * stride;
        const UINT8* src = static_cast<const UINT8*>(vb->GetDataPtr()) + offset;

        SW_float4 value{};
        UnpackVertexElement(elem.Format, src, value);
        for (const auto& inp : vs.vsReflection->inputs)
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

SW_VSOutput SWRasterizer::RunVS(VertexState& vs, UINT vertIdx)
{
    if (vs.cacheEnabled)
    {
        auto it = vs.cache.find(vertIdx);
        if (it != vs.cache.end()) { return it->second; }
    }

    SW_VSInput vsIn{};
    FetchVertex(vs, vsIn, vertIdx);

    for (const auto& inp : vs.vsReflection->inputs)
    {
        if (inp.svType == D3D_NAME_VERTEX_ID)
        {
            Float bits = std::bit_cast<Float>(vertIdx);
            vsIn.v[inp.reg] = {bits, 0.f, 0.f, 0.f};
        }
        else if (inp.svType == D3D_NAME_INSTANCE_ID)
        {
            Float bits = std::bit_cast<Float>(vs.instanceID);
            vsIn.v[inp.reg] = {bits, 0.f, 0.f, 0.f};
        }
    }

    SW_VSOutput vsOut{};
    vs.vsFn(&vsIn, &vsOut, &vs.vsRes);

    if (vs.cacheEnabled)
    {
        vs.cache[vertIdx] = vsOut;
    }

    return vsOut;
}

UINT SWRasterizer::FetchIndex(const VertexState& vs, UINT location)
{
    if (!vs.state->indexBuffer)
    {
        return 0;
    }

    const UINT8* base = static_cast<const UINT8*>(vs.state->indexBuffer->GetDataPtr());
    if (vs.state->indexFormat == DXGI_FORMAT_R16_UINT)
    {
        UINT16 idx;
        std::memcpy(&idx, base + vs.state->indexOffset + location * 2, 2);
        return idx;
    }
    else
    {
        UINT32 idx;
        std::memcpy(&idx, base + vs.state->indexOffset + location * 4, 4);
        return idx;
    }
}

Bool SWRasterizer::TestDepth(const OMState& om, Int px, Int py, Float depth)
{
    UINT8* dsvPx = om.dsvData + (UINT64)py * om.dsvRowPitch + (UINT64)px * om.dsvPixStride;
    Float existing = ReadDepthValue(om.dsvFmt, dsvPx);
    return CompareDepth(om.depthFunc, depth, existing);
}

void SWRasterizer::WriteDepth(OMState& om, Int px, Int py, Float depth)
{
    UINT8* dsvPx = om.dsvData + (UINT64)py * om.dsvRowPitch + (UINT64)px * om.dsvPixStride;
    WriteDepthValue(om.dsvFmt, dsvPx, depth);
}

UINT8 SWRasterizer::ReadStencil(const OMState& om, Int px, Int py)
{
    UINT8* dsvPx = om.dsvData + (UINT64)py * om.dsvRowPitch + (UINT64)px * om.dsvPixStride;
    return ReadStencilValue(om.dsvFmt, dsvPx);
}

void SWRasterizer::WriteStencil(OMState& om, Int px, Int py, UINT8 val)
{
    UINT8* dsvPx = om.dsvData + (UINT64)py * om.dsvRowPitch + (UINT64)px * om.dsvPixStride;
    WriteStencilValue(om.dsvFmt, dsvPx, val);
}

void SWRasterizer::WritePixel(OMState& om, Int px, Int py, const SW_PSOutput& psOut)
{
    for (UINT rt = 0; rt < om.activeRTCount; ++rt)
    {
        BlendAndWrite(om, px, py, rt, psOut.oC[rt], psOut.oC[1]);
    }
}

void SWRasterizer::BlendAndWrite(const OMState& om, Int px, Int py, UINT rtIdx,
                                 const SW_float4& color, const SW_float4& src1Color)
{
    const RTInfo& info = om.rtInfos[rtIdx];
    UINT8* rtvPx = info.data + (UINT64)py * info.rowPitch + (UINT64)px * info.pixStride;

    if (info.blendDesc.LogicOpEnable)
    {
        FLOAT srcColorF[4] = { color.x, color.y, color.z, color.w };
        UINT8 srcPacked[16];
        PackRTVColor(info.fmt, srcColorF, srcPacked);

        UINT8 result[16];
        for (UINT b = 0; b < info.pixStride; ++b)
        {
            result[b] = ApplyLogicOp(info.blendDesc.LogicOp, srcPacked[b], rtvPx[b]);
        }

        UINT8 writeMask = info.blendDesc.RenderTargetWriteMask;
        if (writeMask != D3D11_COLOR_WRITE_ENABLE_ALL)
        {
            UINT bytesPerComp = info.pixStride / 4;
            if (bytesPerComp == 0) { bytesPerComp = 1; }
            for (UINT c = 0; c < 4; ++c)
            {
                if (!(writeMask & (1 << c)))
                {
                    for (UINT b = c * bytesPerComp; b < (c + 1) * bytesPerComp && b < info.pixStride; ++b)
                    {
                        result[b] = rtvPx[b];
                    }
                }
            }
        }

        std::memcpy(rtvPx, result, info.pixStride);
        return;
    }

    FLOAT srcColor[4] = { color.x, color.y, color.z, color.w };
    FLOAT src1[4] = { src1Color.x, src1Color.y, src1Color.z, src1Color.w };
    FLOAT finalColor[4];
    if (info.blendDesc.BlendEnable)
    {
        FLOAT dstColor[4];
        UnpackColor(info.fmt, rtvPx, dstColor);
        for (Int c = 0; c < 3; ++c)
        {
            Float sf = ComputeBlendFactor(info.blendDesc.SrcBlend, srcColor, dstColor, om.blendFactor, c, src1);
            Float df = ComputeBlendFactor(info.blendDesc.DestBlend, srcColor, dstColor, om.blendFactor, c, src1);
            finalColor[c] = ComputeBlendOp(info.blendDesc.BlendOp, srcColor[c] * sf, dstColor[c] * df);
        }
        {
            Float sf = ComputeBlendFactor(info.blendDesc.SrcBlendAlpha, srcColor, dstColor, om.blendFactor, 3, src1);
            Float df = ComputeBlendFactor(info.blendDesc.DestBlendAlpha, srcColor, dstColor, om.blendFactor, 3, src1);
            finalColor[3] = ComputeBlendOp(info.blendDesc.BlendOpAlpha, srcColor[3] * sf, dstColor[3] * df);
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
        UnpackColor(info.fmt, rtvPx, existing);
        if (!(writeMask & D3D11_COLOR_WRITE_ENABLE_RED))   { finalColor[0] = existing[0]; }
        if (!(writeMask & D3D11_COLOR_WRITE_ENABLE_GREEN)) { finalColor[1] = existing[1]; }
        if (!(writeMask & D3D11_COLOR_WRITE_ENABLE_BLUE))  { finalColor[2] = existing[2]; }
        if (!(writeMask & D3D11_COLOR_WRITE_ENABLE_ALPHA)) { finalColor[3] = existing[3]; }
    }

    UINT8 packed[16];
    PackRTVColor(info.fmt, finalColor, packed);
    std::memcpy(rtvPx, packed, info.pixStride);
}

Float SWRasterizer::ReadDepthValue(DXGI_FORMAT fmt, const UINT8* src)
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

void SWRasterizer::WriteDepthValue(DXGI_FORMAT fmt, UINT8* dst, Float depth)
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

UINT SWRasterizer::DepthPixelStride(DXGI_FORMAT fmt)
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

Bool SWRasterizer::CompareDepth(D3D11_COMPARISON_FUNC func, Float src, Float dst)
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

Float SWRasterizer::ComputeBlendFactor(D3D11_BLEND factor, const FLOAT src[4], const FLOAT dst[4],
                                        const FLOAT blendFactor[4], Int comp, const FLOAT src1[4])
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
        case D3D11_BLEND_SRC1_COLOR:      return src1[comp];
        case D3D11_BLEND_INV_SRC1_COLOR:  return 1.f - src1[comp];
        case D3D11_BLEND_SRC1_ALPHA:      return src1[3];
        case D3D11_BLEND_INV_SRC1_ALPHA:  return 1.f - src1[3];
        default: return 1.f;
    }
}

Float SWRasterizer::ComputeBlendOp(D3D11_BLEND_OP op, Float srcTerm, Float dstTerm)
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

UINT8 SWRasterizer::ApplyLogicOp(D3D11_LOGIC_OP op, UINT8 src, UINT8 dst)
{
    switch (op)
    {
        case D3D11_LOGIC_OP_CLEAR:         return 0;
        case D3D11_LOGIC_OP_SET:           return 0xFF;
        case D3D11_LOGIC_OP_COPY:          return src;
        case D3D11_LOGIC_OP_COPY_INVERTED: return ~src;
        case D3D11_LOGIC_OP_NOOP:          return dst;
        case D3D11_LOGIC_OP_INVERT:        return ~dst;
        case D3D11_LOGIC_OP_AND:           return src & dst;
        case D3D11_LOGIC_OP_NAND:          return ~(src & dst);
        case D3D11_LOGIC_OP_OR:            return src | dst;
        case D3D11_LOGIC_OP_NOR:           return ~(src | dst);
        case D3D11_LOGIC_OP_XOR:           return src ^ dst;
        case D3D11_LOGIC_OP_EQUIV:         return ~(src ^ dst);
        case D3D11_LOGIC_OP_AND_REVERSE:   return src & ~dst;
        case D3D11_LOGIC_OP_AND_INVERTED:  return ~src & dst;
        case D3D11_LOGIC_OP_OR_REVERSE:    return src | ~dst;
        case D3D11_LOGIC_OP_OR_INVERTED:   return ~src | dst;
        default:                           return src;
    }
}

UINT8 SWRasterizer::ReadStencilValue(DXGI_FORMAT fmt, const UINT8* src)
{
    switch (fmt)
    {
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        {
            UINT32 u;
            std::memcpy(&u, src, 4);
            return static_cast<UINT8>(u >> 24);
        }
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            return src[4];
        default:
            return 0;
    }
}

void SWRasterizer::WriteStencilValue(DXGI_FORMAT fmt, UINT8* dst, UINT8 val)
{
    switch (fmt)
    {
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        {
            UINT32 u;
            std::memcpy(&u, dst, 4);
            u = (u & 0x00FFFFFFu) | (static_cast<UINT32>(val) << 24);
            std::memcpy(dst, &u, 4);
            break;
        }
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            dst[4] = val;
            break;
        default:
            break;
    }
}

Bool SWRasterizer::FormatHasStencil(DXGI_FORMAT fmt)
{
    return fmt == DXGI_FORMAT_D24_UNORM_S8_UINT ||
           fmt == DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
}

Bool SWRasterizer::CompareStencil(D3D11_COMPARISON_FUNC func, UINT8 ref, UINT8 val)
{
    switch (func)
    {
        case D3D11_COMPARISON_NEVER:         return false;
        case D3D11_COMPARISON_LESS:          return ref < val;
        case D3D11_COMPARISON_EQUAL:         return ref == val;
        case D3D11_COMPARISON_LESS_EQUAL:    return ref <= val;
        case D3D11_COMPARISON_GREATER:       return ref > val;
        case D3D11_COMPARISON_NOT_EQUAL:     return ref != val;
        case D3D11_COMPARISON_GREATER_EQUAL: return ref >= val;
        case D3D11_COMPARISON_ALWAYS:        return true;
        default:                             return true;
    }
}

UINT8 SWRasterizer::ApplyStencilOp(D3D11_STENCIL_OP op, UINT8 curVal, UINT8 ref)
{
    switch (op)
    {
        case D3D11_STENCIL_OP_KEEP:     return curVal;
        case D3D11_STENCIL_OP_ZERO:     return 0;
        case D3D11_STENCIL_OP_REPLACE:  return ref;
        case D3D11_STENCIL_OP_INCR_SAT: return curVal < 255 ? curVal + 1 : 255;
        case D3D11_STENCIL_OP_DECR_SAT: return curVal > 0 ? curVal - 1 : 0;
        case D3D11_STENCIL_OP_INVERT:   return ~curVal;
        case D3D11_STENCIL_OP_INCR:     return static_cast<UINT8>(curVal + 1);
        case D3D11_STENCIL_OP_DECR:     return static_cast<UINT8>(curVal - 1);
        default:                        return curVal;
    }
}

struct VaryingMap { Int vsOutReg; Int psInReg; };

using I64 = Int64;
struct TileContext
{
    I64 w0_tileOrig, w1_tileOrig, w2_tileOrig;
    I64 w0_tileStepX, w1_tileStepX, w2_tileStepX;
    I64 w0_tileStepY, w1_tileStepY, w2_tileStepY;
    I64 w0_dx, w1_dx, w2_dx;
    I64 w0_dy, w1_dy, w2_dy;
    I64 w0_cornerDx, w1_cornerDx, w2_cornerDx;
    I64 w0_cornerDy, w1_cornerDy, w2_cornerDy;

    Float invArea2;
    Float iw2, diw0, diw1;
    Float dz0, dz1;
    Float ndcZ2;
    Float depthBias;
    Float vpMinDepth, vpMaxDepth;

    const SW_float4* v0pw;
    const SW_float4* v1pw;
    const SW_float4* v2pw;
    const VaryingMap* varyings;
    Int numVaryings;
    Int svPosRegPS;

    SWRasterizer::OMState* om;

    SW_PSFn psFn;
    SW_Resources* psRes;

    Int tileMinX, tileMinY;
    Int minX, minY, maxX, maxY;
    Int tileStepX, tileStepY;
    Int numTilesX;
    Bool useTiling;
    Bool frontFace;
    Bool earlyZ;
};

void ProcessOneTile(const TileContext& ctx, Uint32 tileIdx,
                           SW_PSInput& psIn, SW_PSOutput& psOut)
{
    Int tileCol = static_cast<Int>(tileIdx) % ctx.numTilesX;
    Int tileRow = static_cast<Int>(tileIdx) / ctx.numTilesX;

    Int tileX = ctx.tileMinX + tileCol * ctx.tileStepX;
    Int tileY = ctx.tileMinY + tileRow * ctx.tileStepY;

    I64 w0_tile = ctx.w0_tileOrig + static_cast<I64>(tileCol) * ctx.w0_tileStepX
                                  + static_cast<I64>(tileRow) * ctx.w0_tileStepY;
    I64 w1_tile = ctx.w1_tileOrig + static_cast<I64>(tileCol) * ctx.w1_tileStepX
                                  + static_cast<I64>(tileRow) * ctx.w1_tileStepY;
    I64 w2_tile = ctx.w2_tileOrig + static_cast<I64>(tileCol) * ctx.w2_tileStepX
                                  + static_cast<I64>(tileRow) * ctx.w2_tileStepY;

    Bool allOutside = false;
    Bool fullyCovered = false;
    if (ctx.useTiling)
    {
        I64 w0_TL = w0_tile;
        I64 w1_TL = w1_tile;
        I64 w2_TL = w2_tile;

        I64 w0_TR = w0_TL + ctx.w0_cornerDx;
        I64 w1_TR = w1_TL + ctx.w1_cornerDx;
        I64 w2_TR = w2_TL + ctx.w2_cornerDx;

        I64 w0_BL = w0_TL + ctx.w0_cornerDy;
        I64 w1_BL = w1_TL + ctx.w1_cornerDy;
        I64 w2_BL = w2_TL + ctx.w2_cornerDy;

        I64 w0_BR = w0_TR + ctx.w0_cornerDy;
        I64 w1_BR = w1_TR + ctx.w1_cornerDy;
        I64 w2_BR = w2_TR + ctx.w2_cornerDy;

        allOutside =
            (w0_TL < 0 && w0_TR < 0 && w0_BL < 0 && w0_BR < 0) ||
            (w1_TL < 0 && w1_TR < 0 && w1_BL < 0 && w1_BR < 0) ||
            (w2_TL < 0 && w2_TR < 0 && w2_BL < 0 && w2_BR < 0);

        fullyCovered =
            (w0_TL >= 0 && w0_TR >= 0 && w0_BL >= 0 && w0_BR >= 0) &&
            (w1_TL >= 0 && w1_TR >= 0 && w1_BL >= 0 && w1_BR >= 0) &&
            (w2_TL >= 0 && w2_TR >= 0 && w2_BL >= 0 && w2_BR >= 0);
    }

    if (allOutside) { return; }

    Int tMinX = std::max(tileX, ctx.minX);
    Int tMaxX = std::min(tileX + ctx.tileStepX, ctx.maxX);
    Int tMinY = std::max(tileY, ctx.minY);
    Int tMaxY = std::min(tileY + ctx.tileStepY, ctx.maxY);

    I64 w0_pixRowStart = w0_tile + ctx.w0_dx * (tMinX - tileX) + ctx.w0_dy * (tMinY - tileY);
    I64 w1_pixRowStart = w1_tile + ctx.w1_dx * (tMinX - tileX) + ctx.w1_dy * (tMinY - tileY);
    I64 w2_pixRowStart = w2_tile + ctx.w2_dx * (tMinX - tileX) + ctx.w2_dy * (tMinY - tileY);

    auto& om = *ctx.om;
    psIn.isFrontFace = ctx.frontFace ? 1u : 0u;
    for (Int py = tMinY; py < tMaxY; ++py)
    {
        I64 w0 = w0_pixRowStart;
        I64 w1 = w1_pixRowStart;
        I64 w2 = w2_pixRowStart;

        Float pyCenter = static_cast<Float>(py) + 0.5f;
        for (Int px = tMinX; px < tMaxX; ++px)
        {
            if (fullyCovered || (w0 >= 0 && w1 >= 0 && w2 >= 0))
            {
                Float b0 = static_cast<Float>(w0) * ctx.invArea2;
                Float b1 = static_cast<Float>(w1) * ctx.invArea2;

                Float perspW = ctx.iw2 + b0 * ctx.diw0 + b1 * ctx.diw1;
                Float invPerspW = (perspW != 0.f) ? 1.f / perspW : 0.f;

                Float depth = ctx.ndcZ2 + b0 * ctx.dz0 + b1 * ctx.dz1;
                depth += ctx.depthBias;
                depth = std::clamp(depth, ctx.vpMinDepth, ctx.vpMaxDepth);

                UINT8 oldStencil = 0;
                const D3D11_DEPTH_STENCILOP_DESC* stencilOps = nullptr;

                if (ctx.earlyZ)
                {
                    if (om.stencilEnabled)
                    {
                        oldStencil = SWRasterizer::ReadStencil(om, px, py);
                        stencilOps = ctx.frontFace ? &om.stencilFront : &om.stencilBack;

                        UINT8 maskedRef = om.stencilRef & om.stencilReadMask;
                        UINT8 maskedVal = oldStencil & om.stencilReadMask;
                        if (!SWRasterizer::CompareStencil(
                                ctx.frontFace ? om.stencilFront.StencilFunc : om.stencilBack.StencilFunc,
                                maskedRef, maskedVal))
                        {
                            UINT8 result = SWRasterizer::ApplyStencilOp(stencilOps->StencilFailOp, oldStencil, om.stencilRef);
                            UINT8 written = (result & om.stencilWriteMask) | (oldStencil & ~om.stencilWriteMask);
                            SWRasterizer::WriteStencil(om, px, py, written);
                            w0 += ctx.w0_dx;
                            w1 += ctx.w1_dx;
                            w2 += ctx.w2_dx;
                            continue;
                        }
                    }

                    if (om.depthEnabled)
                    {
                        if (!SWRasterizer::TestDepth(om, px, py, depth))
                        {
                            if (om.stencilEnabled)
                            {
                                UINT8 result = SWRasterizer::ApplyStencilOp(stencilOps->StencilDepthFailOp, oldStencil, om.stencilRef);
                                UINT8 written = (result & om.stencilWriteMask) | (oldStencil & ~om.stencilWriteMask);
                                SWRasterizer::WriteStencil(om, px, py, written);
                            }
                            w0 += ctx.w0_dx;
                            w1 += ctx.w1_dx;
                            w2 += ctx.w2_dx;
                            continue;
                        }
                    }
                }

                Float pxCenter = static_cast<Float>(px) + 0.5f;
                psIn.pos = { pxCenter, pyCenter, depth, perspW };
                if (ctx.svPosRegPS >= 0)
                {
                    psIn.v[ctx.svPosRegPS] = psIn.pos;
                }

                Float b2 = 1.f - b0 - b1;
                for (Int vi = 0; vi < ctx.numVaryings; ++vi)
                {
                    Int psR = ctx.varyings[vi].psInReg;

                    Float ax = (b0 * ctx.v0pw[vi].x + b1 * ctx.v1pw[vi].x + b2 * ctx.v2pw[vi].x) * invPerspW;
                    Float ay = (b0 * ctx.v0pw[vi].y + b1 * ctx.v1pw[vi].y + b2 * ctx.v2pw[vi].y) * invPerspW;
                    Float az = (b0 * ctx.v0pw[vi].z + b1 * ctx.v1pw[vi].z + b2 * ctx.v2pw[vi].z) * invPerspW;
                    Float aw = (b0 * ctx.v0pw[vi].w + b1 * ctx.v1pw[vi].w + b2 * ctx.v2pw[vi].w) * invPerspW;

                    psIn.v[psR] = { ax, ay, az, aw };
                }

                psOut = {};
                ctx.psFn(&psIn, &psOut, ctx.psRes);

                if (psOut.discarded)
                {
                    w0 += ctx.w0_dx;
                    w1 += ctx.w1_dx;
                    w2 += ctx.w2_dx;
                    continue;
                }

                if (!ctx.earlyZ)
                {
                    Float testDepth = psOut.depthWritten ? psOut.oDepth : depth;

                    if (om.stencilEnabled)
                    {
                        oldStencil = SWRasterizer::ReadStencil(om, px, py);
                        stencilOps = ctx.frontFace ? &om.stencilFront : &om.stencilBack;

                        UINT8 maskedRef = om.stencilRef & om.stencilReadMask;
                        UINT8 maskedVal = oldStencil & om.stencilReadMask;
                        if (!SWRasterizer::CompareStencil(
                                ctx.frontFace ? om.stencilFront.StencilFunc : om.stencilBack.StencilFunc,
                                maskedRef, maskedVal))
                        {
                            UINT8 result = SWRasterizer::ApplyStencilOp(stencilOps->StencilFailOp, oldStencil, om.stencilRef);
                            UINT8 written = (result & om.stencilWriteMask) | (oldStencil & ~om.stencilWriteMask);
                            SWRasterizer::WriteStencil(om, px, py, written);
                            w0 += ctx.w0_dx;
                            w1 += ctx.w1_dx;
                            w2 += ctx.w2_dx;
                            continue;
                        }
                    }

                    if (om.depthEnabled)
                    {
                        if (!SWRasterizer::TestDepth(om, px, py, testDepth))
                        {
                            if (om.stencilEnabled)
                            {
                                UINT8 result = SWRasterizer::ApplyStencilOp(stencilOps->StencilDepthFailOp, oldStencil, om.stencilRef);
                                UINT8 written = (result & om.stencilWriteMask) | (oldStencil & ~om.stencilWriteMask);
                                SWRasterizer::WriteStencil(om, px, py, written);
                            }
                            w0 += ctx.w0_dx;
                            w1 += ctx.w1_dx;
                            w2 += ctx.w2_dx;
                            continue;
                        }
                    }

                    depth = testDepth;
                }

                if (om.depthEnabled && om.depthWriteMask == D3D11_DEPTH_WRITE_MASK_ALL)
                {
                    SWRasterizer::WriteDepth(om, px, py, depth);
                }

                if (om.stencilEnabled)
                {
                    UINT8 result = SWRasterizer::ApplyStencilOp(stencilOps->StencilPassOp, oldStencil, om.stencilRef);
                    UINT8 written = (result & om.stencilWriteMask) | (oldStencil & ~om.stencilWriteMask);
                    SWRasterizer::WriteStencil(om, px, py, written);
                }

                SWRasterizer::WritePixel(om, px, py, psOut);
            }

            w0 += ctx.w0_dx;
            w1 += ctx.w1_dx;
            w2 += ctx.w2_dx;
        }

        w0_pixRowStart += ctx.w0_dy;
        w1_pixRowStart += ctx.w1_dy;
        w2_pixRowStart += ctx.w2_dy;
    }
}

static void ProcessTileTrampoline(void* ctx, Uint32 tileIdx)
{
    SW_PSInput psIn{};
    SW_PSOutput psOut{};
    ProcessOneTile(*static_cast<const TileContext*>(ctx), tileIdx, psIn, psOut);
}

static constexpr Float NearEpsilon = 1e-5f;

static SW_VSOutput LerpVertex(const SW_VSOutput& a, const SW_VSOutput& b, Float t)
{
    SW_VSOutput r;
    r.pos.x = a.pos.x + (b.pos.x - a.pos.x) * t;
    r.pos.y = a.pos.y + (b.pos.y - a.pos.y) * t;
    r.pos.z = a.pos.z + (b.pos.z - a.pos.z) * t;
    r.pos.w = a.pos.w + (b.pos.w - a.pos.w) * t;
    for (Int i = 0; i < SW_MAX_VARYINGS; ++i)
    {
        r.o[i].x = a.o[i].x + (b.o[i].x - a.o[i].x) * t;
        r.o[i].y = a.o[i].y + (b.o[i].y - a.o[i].y) * t;
        r.o[i].z = a.o[i].z + (b.o[i].z - a.o[i].z) * t;
        r.o[i].w = a.o[i].w + (b.o[i].w - a.o[i].w) * t;
    }
    for (Int i = 0; i < 8; ++i)
    {
        r.clipDist[i] = a.clipDist[i] + (b.clipDist[i] - a.clipDist[i]) * t;
    }
    return r;
}

static constexpr Int MaxClipVerts = 24;

static Int ClipPolygonAgainstPlane(
    const SW_VSOutput* in, Int inCount,
    SW_VSOutput* out,
    Float px, Float py, Float pz, Float pw)
{
    if (inCount < 3) { return 0; }

    Int outCount = 0;
    for (Int i = 0; i < inCount; ++i)
    {
        Int j = (i + 1) % inCount;
        const SW_VSOutput& a = in[i];
        const SW_VSOutput& b = in[j];

        Float dA = a.pos.x * px + a.pos.y * py + a.pos.z * pz + a.pos.w * pw;
        Float dB = b.pos.x * px + b.pos.y * py + b.pos.z * pz + b.pos.w * pw;

        Bool aInside = dA >= 0.f;
        Bool bInside = dB >= 0.f;

        if (aInside)
        {
            out[outCount++] = a;
        }

        if (aInside != bInside)
        {
            Float t = dA / (dA - dB);
            out[outCount++] = LerpVertex(a, b, t);
        }
    }
    return outCount;
}

static Int ClipPolygonAgainstClipDist(
    const SW_VSOutput* in, Int inCount,
    SW_VSOutput* out, Int clipIdx)
{
    if (inCount < 3) { return 0; }

    Int outCount = 0;
    for (Int i = 0; i < inCount; ++i)
    {
        Int j = (i + 1) % inCount;
        const SW_VSOutput& a = in[i];
        const SW_VSOutput& b = in[j];

        Float dA = a.clipDist[clipIdx];
        Float dB = b.clipDist[clipIdx];

        if (dA >= 0.f)
        {
            out[outCount++] = a;
        }

        if ((dA >= 0.f) != (dB >= 0.f))
        {
            Float t = dA / (dA - dB);
            out[outCount++] = LerpVertex(a, b, t);
        }
    }
    return outCount;
}

static Int ClipTriangle(const SW_VSOutput in[3], SW_VSOutput out[][3],
                        Float guardBandK, Bool depthClip, Int numClipDist)
{
    Bool anyNear = false;
    Bool anyOther = false;
    for (Int v = 0; v < 3; ++v)
    {
        const auto& p = in[v].pos;
        if (p.w < NearEpsilon) { anyNear = true; break; }
        Float Kw = guardBandK * p.w;
        if (p.x < -Kw || p.x > Kw || p.y < -Kw || p.y > Kw
            || (depthClip && p.z > p.w))
        {
            anyOther = true;
        }
        for (Int c = 0; c < numClipDist; ++c)
        {
            if (in[v].clipDist[c] < 0.f) { anyOther = true; }
        }
    }

    if (!anyNear && !anyOther)
    {
        out[0][0] = in[0];
        out[0][1] = in[1];
        out[0][2] = in[2];
        return 1;
    }

    SW_VSOutput bufA[MaxClipVerts];
    SW_VSOutput bufB[MaxClipVerts];

    bufA[0] = in[0];
    bufA[1] = in[1];
    bufA[2] = in[2];
    Int count = 3;

    SW_VSOutput* src = bufA;
    SW_VSOutput* dst = bufB;

    if (anyNear)
    {
        Int outCount = 0;
        for (Int i = 0; i < count; ++i)
        {
            Int j = (i + 1) % count;
            const SW_VSOutput& a = src[i];
            const SW_VSOutput& b = src[j];

            Float dA = a.pos.w - NearEpsilon;
            Float dB = b.pos.w - NearEpsilon;
            Bool aIn = dA >= 0.f;
            Bool bIn = dB >= 0.f;

            if (aIn) { dst[outCount++] = a; }
            if (aIn != bIn)
            {
                Float t = dA / (dA - dB);
                dst[outCount++] = LerpVertex(a, b, t);
            }
        }
        count = outCount;
        std::swap(src, dst);
        if (count < 3) { return 0; }
    }

    if (depthClip)
    {
        struct { Float px, py, pz, pw; } planes[] =
        {
            {  1.f,  0.f,  0.f, guardBandK },
            { -1.f,  0.f,  0.f, guardBandK },
            {  0.f, -1.f,  0.f, guardBandK },
            {  0.f,  1.f,  0.f, guardBandK },
            {  0.f,  0.f, -1.f, 1.f        },
        };

        for (const auto& pl : planes)
        {
            Int outCount = ClipPolygonAgainstPlane(src, count, dst, pl.px, pl.py, pl.pz, pl.pw);
            count = outCount;
            std::swap(src, dst);
            if (count < 3) { return 0; }
        }
    }
    else
    {
        struct { Float px, py, pz, pw; } planes[] =
        {
            {  1.f,  0.f,  0.f, guardBandK },
            { -1.f,  0.f,  0.f, guardBandK },
            {  0.f, -1.f,  0.f, guardBandK },
            {  0.f,  1.f,  0.f, guardBandK },
        };

        for (const auto& pl : planes)
        {
            Int outCount = ClipPolygonAgainstPlane(src, count, dst, pl.px, pl.py, pl.pz, pl.pw);
            count = outCount;
            std::swap(src, dst);
            if (count < 3) { return 0; }
        }
    }

    for (Int c = 0; c < numClipDist; ++c)
    {
        Int outCount = ClipPolygonAgainstClipDist(src, count, dst, c);
        count = outCount;
        std::swap(src, dst);
        if (count < 3) { return 0; }
    }

    Int numTris = count - 2;
    for (Int t = 0; t < numTris; ++t)
    {
        out[t][0] = src[0];
        out[t][1] = src[t + 1];
        out[t][2] = src[t + 2];
    }
    return numTris;
}

Int SWRasterizer::FindSVPositionInput(const D3D11SW_ParsedShader& shader)
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

Int SWRasterizer::FindSemanticRegister(const std::vector<D3D11SW_ShaderSignatureElement>& sig,
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

void SWRasterizer::RasterizeTriangle(
    const SW_VSOutput tri[3],
    const D3D11SW_ParsedShader& vsReflection,
    const D3D11SW_ParsedShader& psReflection,
    SW_PSFn psFn,
    SW_Resources& psRes,
    OMState& om,
    D3D11SW_PIPELINE_STATE& state,
    Bool alreadyClipped)
{
    if (state.numViewports == 0)
    {
        return;
    }

    const D3D11_VIEWPORT& vp = state.viewports[0];

    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_BACK;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthClipEnable = TRUE;
    if (state.rsState)
    {
        state.rsState->GetDesc(&rsDesc);
    }

    Bool depthClip = rsDesc.DepthClipEnable;
    Int numClipDist = static_cast<Int>(vsReflection.numClipDistances);
    Int numCullDist = static_cast<Int>(vsReflection.numCullDistances);

    for (Int c = 0; c < numCullDist; ++c)
    {
        if (tri[0].cullDist[c] < 0.f && tri[1].cullDist[c] < 0.f && tri[2].cullDist[c] < 0.f)
        {
            return;
        }
    }

    Bool needsClip = false;
    for (Int v = 0; v < 3; ++v)
    {
        const auto& p = tri[v].pos;
        Float Kw = _config.guardBandK * p.w;
        if (p.w < NearEpsilon
            || p.x < -Kw || p.x > Kw || p.y < -Kw || p.y > Kw
            || (depthClip && p.z > p.w))
        {
            needsClip = true;
            break;
        }
        for (Int c = 0; c < numClipDist; ++c)
        {
            if (tri[v].clipDist[c] < 0.f) { needsClip = true; break; }
        }
        if (needsClip) { break; }
    }
    if (needsClip)
    {
        if (alreadyClipped)
        {
            for (Int v = 0; v < 3; ++v)
            {
                if (tri[v].pos.w < NearEpsilon)
                {
                    return;
                }
            }
        }
        else
        {
            SW_VSOutput clipped[22][3];
            Int n = ClipTriangle(tri, clipped, _config.guardBandK, depthClip, numClipDist);
            for (Int t = 0; t < n; ++t)
            {
                RasterizeTriangle(clipped[t], vsReflection, psReflection, psFn, psRes, om, state, true);
            }
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

    Float edgeArea = (screenX[1] - screenX[0]) * (screenY[2] - screenY[0])
                   - (screenY[1] - screenY[0]) * (screenX[2] - screenX[0]);

    Bool frontFace = rsDesc.FrontCounterClockwise ? (edgeArea > 0.f) : (edgeArea < 0.f);

    if (rsDesc.CullMode == D3D11_CULL_BACK  && !frontFace) { return; }
    if (rsDesc.CullMode == D3D11_CULL_FRONT &&  frontFace) { return; }

    if (edgeArea == 0.f) { return; }

    Float depthBias = 0.f;
    if (rsDesc.DepthBias != 0 || rsDesc.SlopeScaledDepthBias != 0.f)
    {
        //https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-output-merger-stage-depth-bias
        Float invArea = 1.f / edgeArea;
        Float dzdx = ((ndcZ[1] - ndcZ[0]) * (screenY[2] - screenY[0]) -
                       (ndcZ[2] - ndcZ[0]) * (screenY[1] - screenY[0])) * invArea;
        Float dzdy = ((ndcZ[2] - ndcZ[0]) * (screenX[1] - screenX[0]) -
                       (ndcZ[1] - ndcZ[0]) * (screenX[2] - screenX[0])) * invArea;
        Float maxSlope = std::max(std::abs(dzdx), std::abs(dzdy));

        Float r = 0.f;
        switch (om.dsvFmt)
        {
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
            r = 1.f / static_cast<Float>(1 << 24);
            break;
        case DXGI_FORMAT_D16_UNORM:
            r = 1.f / static_cast<Float>(1 << 16);
            break;
        case DXGI_FORMAT_D32_FLOAT:
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        {
            Float maxZ = std::max({ndcZ[0], ndcZ[1], ndcZ[2]});
            Int exp;
            std::frexp(maxZ, &exp);
            r = std::ldexp(1.f, exp - 23);
            break;
        }
        default:
            break;
        }

        depthBias = static_cast<Float>(rsDesc.DepthBias) * r + rsDesc.SlopeScaledDepthBias * maxSlope;
        if (rsDesc.DepthBiasClamp > 0.f)
        {
            depthBias = std::min(depthBias, rsDesc.DepthBiasClamp);
        }
        else if (rsDesc.DepthBiasClamp < 0.f)
        {
            depthBias = std::max(depthBias, rsDesc.DepthBiasClamp);
        }
    }

    if (rsDesc.FillMode == D3D11_FILL_WIREFRAME)
    {
        SW_VSOutput edge[2];
        edge[0] = tri[0]; edge[1] = tri[1];
        RasterizeLine(edge, vsReflection, psReflection, psFn, psRes, om, state);
        edge[0] = tri[1]; edge[1] = tri[2];
        RasterizeLine(edge, vsReflection, psReflection, psFn, psRes, om, state);
        edge[0] = tri[2]; edge[1] = tri[0];
        RasterizeLine(edge, vsReflection, psReflection, psFn, psRes, om, state);
        return;
    }

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

    if (om.activeRTCount == 0) { return; }

    Float diw0 = iw0 - iw2;
    Float diw1 = iw1 - iw2;
    Float dz0 = ndcZ[0] - ndcZ[2];
    Float dz1 = ndcZ[1] - ndcZ[2];

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

    Int numTilesX = (maxX - tileMinX + tileStepX - 1) / tileStepX;
    Int numTilesY = (maxY - tileMinY + tileStepY - 1) / tileStepY;
    Int totalTiles = numTilesX * numTilesY;

    TileContext tctx{};
    tctx.w0_tileOrig  = w0_tileOrig;  tctx.w1_tileOrig  = w1_tileOrig;  tctx.w2_tileOrig  = w2_tileOrig;
    tctx.w0_tileStepX = w0_tileStepX; tctx.w1_tileStepX = w1_tileStepX; tctx.w2_tileStepX = w2_tileStepX;
    tctx.w0_tileStepY = w0_tileStepY; tctx.w1_tileStepY = w1_tileStepY; tctx.w2_tileStepY = w2_tileStepY;
    tctx.w0_dx = w0_dx; tctx.w1_dx = w1_dx; tctx.w2_dx = w2_dx;
    tctx.w0_dy = w0_dy; tctx.w1_dy = w1_dy; tctx.w2_dy = w2_dy;
    tctx.w0_cornerDx = w0_cornerDx; tctx.w1_cornerDx = w1_cornerDx; tctx.w2_cornerDx = w2_cornerDx;
    tctx.w0_cornerDy = w0_cornerDy; tctx.w1_cornerDy = w1_cornerDy; tctx.w2_cornerDy = w2_cornerDy;

    tctx.invArea2    = invArea2;
    tctx.iw2         = iw2;
    tctx.diw0        = diw0;
    tctx.diw1        = diw1;
    tctx.dz0         = dz0;
    tctx.dz1         = dz1;
    tctx.ndcZ2       = ndcZ[2];
    tctx.depthBias   = depthBias;
    tctx.vpMinDepth  = vp.MinDepth;
    tctx.vpMaxDepth  = vp.MaxDepth;

    tctx.v0pw        = v0pw;
    tctx.v1pw        = v1pw;
    tctx.v2pw        = v2pw;
    tctx.varyings    = varyings;
    tctx.numVaryings = numVaryings;
    tctx.svPosRegPS  = svPosRegPS;

    tctx.om          = &om;

    tctx.psFn        = psFn;
    tctx.psRes       = &psRes;

    tctx.tileMinX  = tileMinX;
    tctx.tileMinY  = tileMinY;
    tctx.minX      = minX;
    tctx.minY      = minY;
    tctx.maxX      = maxX;
    tctx.maxY      = maxY;
    tctx.tileStepX = tileStepX;
    tctx.tileStepY = tileStepY;
    tctx.numTilesX = numTilesX;
    tctx.useTiling = useTiling;
    tctx.frontFace = frontFace;
    tctx.earlyZ    = !psReflection.usesDiscard && !psReflection.writesSVDepth && !psReflection.usesUAVs;

    Bool useThreads = useTiling && _config.tileThreads > 0
                      && totalTiles > _config.tileThreads;

    if (useThreads)
    {
        if (!_tilePool)
        {
            _tilePool = std::make_unique<TileThreadPool>(
                static_cast<Uint32>(_config.tileThreads));
        }
        _tilePool->Dispatch(static_cast<Uint32>(totalTiles),
                            ProcessTileTrampoline, &tctx);
    }
    else
    {
        SW_PSInput psIn{};
        SW_PSOutput psOut{};
        for (Int i = 0; i < totalTiles; ++i)
        {
            ProcessOneTile(tctx, static_cast<Uint32>(i), psIn, psOut);
        }
    }
}

void SWRasterizer::RasterizeLine(
    const SW_VSOutput endpts[2],
    const D3D11SW_ParsedShader& vsReflection,
    const D3D11SW_ParsedShader& psReflection,
    SW_PSFn psFn,
    SW_Resources& psRes,
    OMState& om,
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

    if (om.activeRTCount == 0) { return; }

    Int svPosRegPS = FindSVPositionInput(psReflection);

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

        WritePixel(om, px, py, psOut);
    }
}

void SWRasterizer::RasterizePoint(
    const SW_VSOutput& point,
    const D3D11SW_ParsedShader& vsReflection,
    const D3D11SW_ParsedShader& psReflection,
    SW_PSFn psFn,
    SW_Resources& psRes,
    OMState& om,
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

    if (om.activeRTCount == 0) { return; }

    Int svPosRegPS = FindSVPositionInput(psReflection);

    SW_PSInput psIn{};
    if (svPosRegPS >= 0)
    {
        psIn.v[svPosRegPS] = { sx + 0.5f, sy + 0.5f, 0.f, 1.f };
    }
    psIn.pos = { sx + 0.5f, sy + 0.5f, 0.f, 1.f };

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

    WritePixel(om, px, py, psOut);
}

void SWRasterizer::DrawInternal(
    VertexState& vs, OMState& om,
    const UINT* indices, UINT vertexCount, INT baseVertex,
    D3D11SW_PIPELINE_STATE& state)
{
    SW_PSFn psFn = state.ps->GetJitFn();
    if (!vs.vsFn || !psFn) { return; }

    const D3D11SW_ParsedShader& vsRefl = *vs.vsReflection;
    const D3D11SW_ParsedShader& psRefl = state.ps->GetReflection();

    SW_Resources psRes{};
    BuildStageResources(psRes, state.psCBs, state.psSRVs, state.psSamplers);

    ProcessPrimitives(vs, indices, vertexCount, baseVertex, state.topology,
        [&](const SW_VSOutput tri[3])
        {
            RasterizeTriangle(tri, vsRefl, psRefl, psFn, psRes, om, state);
        },
        [&](const SW_VSOutput endpts[2])
        {
            RasterizeLine(endpts, vsRefl, psRefl, psFn, psRes, om, state);
        },
        [&](const SW_VSOutput& pt)
        {
            RasterizePoint(pt, vsRefl, psRefl, psFn, psRes, om, state);
        });
}

void SWRasterizer::Draw(
    UINT vertexCount, UINT startVertex,
    UINT instanceCount, UINT startInstance,
    D3D11SW_PIPELINE_STATE& state)
{
    if (!state.vs || !state.ps) { return; }

    VertexState vs = InitVS(state);
    OMState om = InitOM(state);

    for (UINT inst = 0; inst < instanceCount; ++inst)
    {
        vs.instanceID = startInstance + inst;
        if (vs.cacheEnabled) { vs.cache.clear(); }
        DrawInternal(vs, om, nullptr, vertexCount, static_cast<INT>(startVertex), state);
    }
}

void SWRasterizer::DrawIndexed(
    UINT indexCount, UINT startIndex, INT baseVertex,
    UINT instanceCount, UINT startInstance,
    D3D11SW_PIPELINE_STATE& state)
{
    if (!state.vs || !state.ps) { return; }

    VertexState vs = InitVS(state);
    OMState om = InitOM(state);

    std::vector<UINT> indices(indexCount);
    for (UINT i = 0; i < indexCount; ++i)
    {
        indices[i] = FetchIndex(vs, startIndex + i);
    }

    for (UINT inst = 0; inst < instanceCount; ++inst)
    {
        vs.instanceID = startInstance + inst;
        if (vs.cacheEnabled) { vs.cache.clear(); }
        DrawInternal(vs, om, indices.data(), indexCount, baseVertex, state);
    }
}

}
