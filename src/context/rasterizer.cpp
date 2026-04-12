#include "context/rasterizer.h"
#include "context/depth_stencil_util.h"
#include "context/blend_util.h"
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
#include "util/fixed_point.h"
#include "util/scratch_arena.h"
#include "util/simd_lane.h"
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
            if (idx >= _count) 
            { 
                break; 
            }

            _fn(_ctx, idx);
        }
        done.count_down();
        done.wait();
    }

private:
    Uint32                     _n;
    std::vector<std::thread>   _threads;
    std::counting_semaphore<>  _ready{0};
    std::atomic<Bool>          _stop = false;
    std::latch*                _done = nullptr;
    TileFn                     _fn = nullptr;
    void*                      _ctx = nullptr;
    Uint32                     _count = 0;
    std::atomic<Uint32>        _next = 0;

private:
    void Run()
    {
        while (true)
        {
            _ready.acquire();
            if (_stop.load(std::memory_order_acquire)) 
            { 
                return; 
            }

            while (true)
            {
                Uint32 idx = _next.fetch_add(1, std::memory_order_relaxed);
                if (idx >= _count) 
                { 
                    break; 
                }
                _fn(_ctx, idx);
            }
            _done->count_down();
        }
    }
};

SWRasterizer::SWRasterizer() : _config(Config::FromEnv()) {}
SWRasterizer::~SWRasterizer() = default;

void SWRasterizer::ClearHiZ(Uint8* dsvData, Int width, Int height, Float clearDepth)
{
    _hiZ.Clear(dsvData, width, height, _config.tileSize, clearDepth);
}

struct VaryingMap { Int vsOutReg; Int psInReg; };

D3D11SW_FORCEINLINE SW_float4 InterpolateVarying(
    const SW_float4& v0, const SW_float4& v1, const SW_float4& v2,
    Float b0, Float b1, Float b2, Float invPerspW)
{
    SimdVec4 r = MulScalar(SimdVec4::Load(&v0.x), b0);
    r = MulAdd(r, SimdVec4::Load(&v1.x), b1);
    r = MulAdd(r, SimdVec4::Load(&v2.x), b2);
    r = MulScalar(r, invPerspW);
    SW_float4 out;
    r.Store(&out.x);
    return out;
}

struct TileContext
{
    Fixed28_4 w0_tileOrig, w1_tileOrig, w2_tileOrig;
    Fixed28_4 w0_tileStepX, w1_tileStepX, w2_tileStepX;
    Fixed28_4 w0_tileStepY, w1_tileStepY, w2_tileStepY;
    Fixed28_4 w0_dx, w1_dx, w2_dx;
    Fixed28_4 w0_dy, w1_dy, w2_dy;
    Fixed28_4 w0_cornerDx, w1_cornerDx, w2_cornerDx;
    Fixed28_4 w0_cornerDy, w1_cornerDy, w2_cornerDy;

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

    OMState* om;

    SW_PSFn psFn;
    SW_Resources* psRes;

    Int tileMinX, tileMinY;
    Int minX, minY, maxX, maxY;
    Int tileStepX, tileStepY;
    Int numTilesX;
    Bool useTiling;
    Bool frontFace;
    Bool earlyZ;
    Bool quadMode;
    SW_PSQuadFn psQuadFn;

    Float* hiZ;
    Int hiZTilesX;
    Int hiZWidth, hiZHeight;
    Int hiZTileSize;
};

void ProcessOneTile(const TileContext& ctx, Uint32 tileIdx,
                           SW_PSInput& psIn, SW_PSOutput& psOut)
{
    Int tileCol = static_cast<Int>(tileIdx) % ctx.numTilesX;
    Int tileRow = static_cast<Int>(tileIdx) / ctx.numTilesX;

    Int tileX = ctx.tileMinX + tileCol * ctx.tileStepX;
    Int tileY = ctx.tileMinY + tileRow * ctx.tileStepY;

    Fixed28_4 w0_tile = ctx.w0_tileOrig + ctx.w0_tileStepX * static_cast<Int64>(tileCol)
                                      + ctx.w0_tileStepY * static_cast<Int64>(tileRow);
    Fixed28_4 w1_tile = ctx.w1_tileOrig + ctx.w1_tileStepX * static_cast<Int64>(tileCol)
                                      + ctx.w1_tileStepY * static_cast<Int64>(tileRow);
    Fixed28_4 w2_tile = ctx.w2_tileOrig + ctx.w2_tileStepX * static_cast<Int64>(tileCol)
                                      + ctx.w2_tileStepY * static_cast<Int64>(tileRow);

    Bool allOutside = false;
    Bool fullyCovered = false;
    if (ctx.useTiling)
    {
        Fixed28_4 w0_TL = w0_tile;
        Fixed28_4 w1_TL = w1_tile;
        Fixed28_4 w2_TL = w2_tile;

        Fixed28_4 w0_TR = w0_TL + ctx.w0_cornerDx;
        Fixed28_4 w1_TR = w1_TL + ctx.w1_cornerDx;
        Fixed28_4 w2_TR = w2_TL + ctx.w2_cornerDx;

        Fixed28_4 w0_BL = w0_TL + ctx.w0_cornerDy;
        Fixed28_4 w1_BL = w1_TL + ctx.w1_cornerDy;
        Fixed28_4 w2_BL = w2_TL + ctx.w2_cornerDy;

        Fixed28_4 w0_BR = w0_TR + ctx.w0_cornerDy;
        Fixed28_4 w1_BR = w1_TR + ctx.w1_cornerDy;
        Fixed28_4 w2_BR = w2_TR + ctx.w2_cornerDy;

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
        return;
    }

    if (ctx.useTiling && ctx.hiZ && ctx.om->depthEnabled &&
        (ctx.om->depthFunc == D3D11_COMPARISON_LESS || ctx.om->depthFunc == D3D11_COMPARISON_LESS_EQUAL))
    {
        Float b0_TL = static_cast<Float>(w0_tile) * ctx.invArea2;
        Float b1_TL = static_cast<Float>(w1_tile) * ctx.invArea2;
        Float b0_TR = static_cast<Float>(w0_tile + ctx.w0_cornerDx) * ctx.invArea2;
        Float b1_TR = static_cast<Float>(w1_tile + ctx.w1_cornerDx) * ctx.invArea2;
        Float b0_BL = static_cast<Float>(w0_tile + ctx.w0_cornerDy) * ctx.invArea2;
        Float b1_BL = static_cast<Float>(w1_tile + ctx.w1_cornerDy) * ctx.invArea2;
        Float b0_BR = static_cast<Float>(w0_tile + ctx.w0_cornerDx + ctx.w0_cornerDy) * ctx.invArea2;
        Float b1_BR = static_cast<Float>(w1_tile + ctx.w1_cornerDx + ctx.w1_cornerDy) * ctx.invArea2;

        Float zTL = ctx.ndcZ2 + b0_TL * ctx.dz0 + b1_TL * ctx.dz1 + ctx.depthBias;
        Float zTR = ctx.ndcZ2 + b0_TR * ctx.dz0 + b1_TR * ctx.dz1 + ctx.depthBias;
        Float zBL = ctx.ndcZ2 + b0_BL * ctx.dz0 + b1_BL * ctx.dz1 + ctx.depthBias;
        Float zBR = ctx.ndcZ2 + b0_BR * ctx.dz0 + b1_BR * ctx.dz1 + ctx.depthBias;

        Float zMinTri = std::min({zTL, zTR, zBL, zBR});
        zMinTri = std::clamp(zMinTri, ctx.vpMinDepth, ctx.vpMaxDepth);

        Int gCol = tileX / ctx.hiZTileSize;
        Int gRow = tileY / ctx.hiZTileSize;
        Float hiZVal = ctx.hiZ[gRow * ctx.hiZTilesX + gCol];

        if (ctx.om->depthFunc == D3D11_COMPARISON_LESS && zMinTri >= hiZVal)
        {
            return;
        }
        if (ctx.om->depthFunc == D3D11_COMPARISON_LESS_EQUAL && zMinTri > hiZVal)
        {
            return;
        }
    }

    Int tMinX = std::max(tileX, ctx.minX);
    Int tMaxX = std::min(tileX + ctx.tileStepX, ctx.maxX);
    Int tMinY = std::max(tileY, ctx.minY);
    Int tMaxY = std::min(tileY + ctx.tileStepY, ctx.maxY);

    if (ctx.quadMode)
    {
        Int qMinX = tMinX & ~1;
        Int qMinY = tMinY & ~1;

        Fixed28_4 w0_qRowStart = w0_tile + ctx.w0_dx * (qMinX - tileX) + ctx.w0_dy * (qMinY - tileY);
        Fixed28_4 w1_qRowStart = w1_tile + ctx.w1_dx * (qMinX - tileX) + ctx.w1_dy * (qMinY - tileY);
        Fixed28_4 w2_qRowStart = w2_tile + ctx.w2_dx * (qMinX - tileX) + ctx.w2_dy * (qMinY - tileY);

        OMState& om = *ctx.om;
        for (Int py = qMinY; py < tMaxY; py += 2)
        {
            Fixed28_4 w0_q = w0_qRowStart;
            Fixed28_4 w1_q = w1_qRowStart;
            Fixed28_4 w2_q = w2_qRowStart;

            for (Int px = qMinX; px < tMaxX; px += 2)
            {
                Fixed28_4 qw0[4], qw1[4], qw2[4];
                qw0[0] = w0_q;                         qw1[0] = w1_q;                         qw2[0] = w2_q;
                qw0[1] = w0_q + ctx.w0_dx;             qw1[1] = w1_q + ctx.w1_dx;             qw2[1] = w2_q + ctx.w2_dx;
                qw0[2] = w0_q + ctx.w0_dy;             qw1[2] = w1_q + ctx.w1_dy;             qw2[2] = w2_q + ctx.w2_dy;
                qw0[3] = w0_q + ctx.w0_dx + ctx.w0_dy; qw1[3] = w1_q + ctx.w1_dx + ctx.w1_dy; qw2[3] = w2_q + ctx.w2_dx + ctx.w2_dy;

                Int qx[4] = { px, px+1, px, px+1 };
                Int qy[4] = { py, py, py+1, py+1 };

                unsigned activeMask = 0;
                for (Int q = 0; q < 4; ++q)
                {
                    if (qx[q] >= tMinX && qx[q] < tMaxX && qy[q] >= tMinY && qy[q] < tMaxY &&
                        (fullyCovered || (qw0[q] >= 0 && qw1[q] >= 0 && qw2[q] >= 0)))
                    {
                        activeMask |= (1u << q);
                    }
                }
                if (activeMask == 0)
                {
                    w0_q += ctx.w0_dx * 2;
                    w1_q += ctx.w1_dx * 2;
                    w2_q += ctx.w2_dx * 2;
                    continue;
                }

                Float quadDepth[4];
                Float quadPerspW[4];
                Float quadB0[4], quadB1[4], quadB2[4], quadInvPerspW[4];
                for (Int q = 0; q < 4; ++q)
                {
                    quadB0[q] = static_cast<Float>(qw0[q]) * ctx.invArea2;
                    quadB1[q] = static_cast<Float>(qw1[q]) * ctx.invArea2;
                    quadB2[q] = 1.f - quadB0[q] - quadB1[q];
                    quadPerspW[q] = ctx.iw2 + quadB0[q] * ctx.diw0 + quadB1[q] * ctx.diw1;
                    quadInvPerspW[q] = (quadPerspW[q] != 0.f) ? 1.f / quadPerspW[q] : 0.f;
                    quadDepth[q] = ctx.ndcZ2 + quadB0[q] * ctx.dz0 + quadB1[q] * ctx.dz1;
                    quadDepth[q] += ctx.depthBias;
                    quadDepth[q] = std::clamp(quadDepth[q], ctx.vpMinDepth, ctx.vpMaxDepth);
                }

                unsigned depthPassMask = activeMask;
                if (ctx.earlyZ)
                {
                    depthPassMask = 0;
                    for (Int q = 0; q < 4; ++q)
                    {
                        if (!(activeMask & (1u << q)))
                        {
                            continue;
                        }

                        if (om.stencilEnabled)
                        {
                            Uint8 oldStencil = ReadStencil(om, qx[q], qy[q]);
                            Uint8 maskedRef = om.stencilRef & om.stencilReadMask;
                            Uint8 maskedVal = oldStencil & om.stencilReadMask;
                            if (!CompareStencil(
                                    ctx.frontFace ? om.stencilFront.StencilFunc : om.stencilBack.StencilFunc,
                                    maskedRef, maskedVal))
                            {
                                continue;
                            }
                        }

                        if (om.depthEnabled && !TestDepth(om, qx[q], qy[q], quadDepth[q]))
                        {
                            continue;
                        }

                        depthPassMask |= (1u << q);
                    }
                    if (depthPassMask == 0)
                    {
                        // all active pixels failed — apply stencil fail ops, skip PS
                        if (om.stencilEnabled)
                        {
                            const D3D11_DEPTH_STENCILOP_DESC* stencilOps = ctx.frontFace ? &om.stencilFront : &om.stencilBack;
                            for (Int q = 0; q < 4; ++q)
                            {
                                if (!(activeMask & (1u << q)))
                                {
                                    continue;
                                }
                                Uint8 oldStencil = ReadStencil(om, qx[q], qy[q]);
                                Uint8 maskedRef = om.stencilRef & om.stencilReadMask;
                                Uint8 maskedVal = oldStencil & om.stencilReadMask;
                                D3D11_STENCIL_OP op;
                                if (!CompareStencil(
                                        ctx.frontFace ? om.stencilFront.StencilFunc : om.stencilBack.StencilFunc,
                                        maskedRef, maskedVal))
                                {
                                    op = stencilOps->StencilFailOp;
                                }
                                else
                                {
                                    op = stencilOps->StencilDepthFailOp;
                                }
                                Uint8 result = ApplyStencilOp(op, oldStencil, om.stencilRef);
                                Uint8 written = (result & om.stencilWriteMask) | (oldStencil & ~om.stencilWriteMask);
                                WriteStencil(om, qx[q], qy[q], written);
                            }
                        }
                        w0_q += ctx.w0_dx * 2;
                        w1_q += ctx.w1_dx * 2;
                        w2_q += ctx.w2_dx * 2;
                        continue;
                    }
                }

                SW_PSQuadInput qin{};
                qin.activeMask = activeMask;
                for (Int q = 0; q < 4; ++q)
                {
                    Float pxC = static_cast<Float>(qx[q]) + 0.5f;
                    Float pyC = static_cast<Float>(qy[q]) + 0.5f;
                    qin.pixels[q].pos = { pxC, pyC, quadDepth[q], quadPerspW[q] };
                    if (ctx.svPosRegPS >= 0)
                    {
                        qin.pixels[q].v[ctx.svPosRegPS] = qin.pixels[q].pos;
                    }
                    qin.pixels[q].isFrontFace = ctx.frontFace ? 1u : 0u;

                    for (Int vi = 0; vi < ctx.numVaryings; ++vi)
                    {
                        Int psR = ctx.varyings[vi].psInReg;
                        qin.pixels[q].v[psR] = InterpolateVarying(
                            ctx.v0pw[vi], ctx.v1pw[vi], ctx.v2pw[vi],
                            quadB0[q], quadB1[q], quadB2[q], quadInvPerspW[q]);
                    }
                }

                SW_PSQuadOutput qout{};
                ctx.psQuadFn(&qin, &qout, ctx.psRes);

                for (Int q = 0; q < 4; ++q)
                {
                    if (!(activeMask & (1u << q)))
                    {
                        continue;
                    }

                    Float depth = qin.pixels[q].pos.z;
                    const D3D11_DEPTH_STENCILOP_DESC* stencilOps = ctx.frontFace ? &om.stencilFront : &om.stencilBack;

                    if (ctx.earlyZ)
                    {
                        if (qout.pixels[q].discarded)
                        {
                            continue;
                        }
                        if (!(depthPassMask & (1u << q)))
                        {
                            // early-Z failed — apply stencil fail ops
                            if (om.stencilEnabled)
                            {
                                Uint8 oldStencil = ReadStencil(om, qx[q], qy[q]);
                                Uint8 maskedRef = om.stencilRef & om.stencilReadMask;
                                Uint8 maskedVal = oldStencil & om.stencilReadMask;
                                D3D11_STENCIL_OP op;
                                if (!CompareStencil(
                                        ctx.frontFace ? om.stencilFront.StencilFunc : om.stencilBack.StencilFunc,
                                        maskedRef, maskedVal))
                                {
                                    op = stencilOps->StencilFailOp;
                                }
                                else
                                {
                                    op = stencilOps->StencilDepthFailOp;
                                }
                                Uint8 result = ApplyStencilOp(op, oldStencil, om.stencilRef);
                                Uint8 written = (result & om.stencilWriteMask) | (oldStencil & ~om.stencilWriteMask);
                                WriteStencil(om, qx[q], qy[q], written);
                            }
                            continue;
                        }
                    }
                    else
                    {
                        if (qout.pixels[q].discarded)
                        {
                            continue;
                        }

                        if (om.stencilEnabled)
                        {
                            Uint8 oldStencil = ReadStencil(om, qx[q], qy[q]);
                            Uint8 maskedRef = om.stencilRef & om.stencilReadMask;
                            Uint8 maskedVal = oldStencil & om.stencilReadMask;
                            if (!CompareStencil(
                                    ctx.frontFace ? om.stencilFront.StencilFunc : om.stencilBack.StencilFunc,
                                    maskedRef, maskedVal))
                            {
                                Uint8 result = ApplyStencilOp(stencilOps->StencilFailOp, oldStencil, om.stencilRef);
                                Uint8 written = (result & om.stencilWriteMask) | (oldStencil & ~om.stencilWriteMask);
                                WriteStencil(om, qx[q], qy[q], written);
                                continue;
                            }
                        }

                        if (om.depthEnabled)
                        {
                            Float testDepth = qout.pixels[q].depthWritten ? qout.pixels[q].oDepth : depth;
                            if (!TestDepth(om, qx[q], qy[q], testDepth))
                            {
                                if (om.stencilEnabled)
                                {
                                    Uint8 oldStencil = ReadStencil(om, qx[q], qy[q]);
                                    Uint8 result = ApplyStencilOp(stencilOps->StencilDepthFailOp, oldStencil, om.stencilRef);
                                    Uint8 written = (result & om.stencilWriteMask) | (oldStencil & ~om.stencilWriteMask);
                                    WriteStencil(om, qx[q], qy[q], written);
                                }
                                continue;
                            }
                            depth = testDepth;
                        }
                    }

                    if (om.depthEnabled && om.depthWriteMask == D3D11_DEPTH_WRITE_MASK_ALL)
                    {
                        WriteDepth(om, qx[q], qy[q], depth);
                    }

                    if (om.stencilEnabled)
                    {
                        Uint8 oldStencil = ReadStencil(om, qx[q], qy[q]);
                        const D3D11_DEPTH_STENCILOP_DESC* stencilOps = ctx.frontFace ? &om.stencilFront : &om.stencilBack;
                        Uint8 result = ApplyStencilOp(stencilOps->StencilPassOp, oldStencil, om.stencilRef);
                        Uint8 written = (result & om.stencilWriteMask) | (oldStencil & ~om.stencilWriteMask);
                        WriteStencil(om, qx[q], qy[q], written);
                    }

                    WritePixel(om, qx[q], qy[q], qout.pixels[q]);
                }

                w0_q += ctx.w0_dx * 2;
                w1_q += ctx.w1_dx * 2;
                w2_q += ctx.w2_dx * 2;
            }

            w0_qRowStart += ctx.w0_dy * 2;
            w1_qRowStart += ctx.w1_dy * 2;
            w2_qRowStart += ctx.w2_dy * 2;
        }
        return;
    }

    Fixed28_4 w0_pixRowStart = w0_tile + ctx.w0_dx * (tMinX - tileX) + ctx.w0_dy * (tMinY - tileY);
    Fixed28_4 w1_pixRowStart = w1_tile + ctx.w1_dx * (tMinX - tileX) + ctx.w1_dy * (tMinY - tileY);
    Fixed28_4 w2_pixRowStart = w2_tile + ctx.w2_dx * (tMinX - tileX) + ctx.w2_dy * (tMinY - tileY);

    OMState& om = *ctx.om;
    psIn.isFrontFace = ctx.frontFace ? 1u : 0u;
    for (Int py = tMinY; py < tMaxY; ++py)
    {
        Fixed28_4 w0 = w0_pixRowStart;
        Fixed28_4 w1 = w1_pixRowStart;
        Fixed28_4 w2 = w2_pixRowStart;

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

                Bool earlyZPassed = true;
                if (ctx.earlyZ)
                {
                    if (om.stencilEnabled)
                    {
                        Uint8 oldStencil = ReadStencil(om, px, py);
                        Uint8 maskedRef = om.stencilRef & om.stencilReadMask;
                        Uint8 maskedVal = oldStencil & om.stencilReadMask;
                        const D3D11_DEPTH_STENCILOP_DESC* stencilOps = ctx.frontFace ? &om.stencilFront : &om.stencilBack;
                        if (!CompareStencil(
                                ctx.frontFace ? om.stencilFront.StencilFunc : om.stencilBack.StencilFunc,
                                maskedRef, maskedVal))
                        {
                            Uint8 result = ApplyStencilOp(stencilOps->StencilFailOp, oldStencil, om.stencilRef);
                            Uint8 written = (result & om.stencilWriteMask) | (oldStencil & ~om.stencilWriteMask);
                            WriteStencil(om, px, py, written);
                            earlyZPassed = false;
                        }
                        else if (om.depthEnabled && !TestDepth(om, px, py, depth))
                        {
                            Uint8 result = ApplyStencilOp(stencilOps->StencilDepthFailOp, oldStencil, om.stencilRef);
                            Uint8 written = (result & om.stencilWriteMask) | (oldStencil & ~om.stencilWriteMask);
                            WriteStencil(om, px, py, written);
                            earlyZPassed = false;
                        }
                    }
                    else if (om.depthEnabled && !TestDepth(om, px, py, depth))
                    {
                        earlyZPassed = false;
                    }

                    if (!earlyZPassed)
                    {
                        w0 += ctx.w0_dx;
                        w1 += ctx.w1_dx;
                        w2 += ctx.w2_dx;
                        continue;
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

                    psIn.v[psR] = InterpolateVarying(
                        ctx.v0pw[vi], ctx.v1pw[vi], ctx.v2pw[vi],
                        b0, b1, b2, invPerspW);
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
                        Uint8 oldStencil = ReadStencil(om, px, py);
                        const D3D11_DEPTH_STENCILOP_DESC* stencilOps = ctx.frontFace ? &om.stencilFront : &om.stencilBack;

                        Uint8 maskedRef = om.stencilRef & om.stencilReadMask;
                        Uint8 maskedVal = oldStencil & om.stencilReadMask;
                        if (!CompareStencil(
                                ctx.frontFace ? om.stencilFront.StencilFunc : om.stencilBack.StencilFunc,
                                maskedRef, maskedVal))
                        {
                            Uint8 result = ApplyStencilOp(stencilOps->StencilFailOp, oldStencil, om.stencilRef);
                            Uint8 written = (result & om.stencilWriteMask) | (oldStencil & ~om.stencilWriteMask);
                            WriteStencil(om, px, py, written);
                            w0 += ctx.w0_dx;
                            w1 += ctx.w1_dx;
                            w2 += ctx.w2_dx;
                            continue;
                        }

                        if (om.depthEnabled && !TestDepth(om, px, py, testDepth))
                        {
                            Uint8 result = ApplyStencilOp(stencilOps->StencilDepthFailOp, oldStencil, om.stencilRef);
                            Uint8 written = (result & om.stencilWriteMask) | (oldStencil & ~om.stencilWriteMask);
                            WriteStencil(om, px, py, written);
                            w0 += ctx.w0_dx;
                            w1 += ctx.w1_dx;
                            w2 += ctx.w2_dx;
                            continue;
                        }
                    }
                    else if (om.depthEnabled && !TestDepth(om, px, py, testDepth))
                    {
                        w0 += ctx.w0_dx;
                        w1 += ctx.w1_dx;
                        w2 += ctx.w2_dx;
                        continue;
                    }

                    depth = testDepth;
                }

                if (om.depthEnabled && om.depthWriteMask == D3D11_DEPTH_WRITE_MASK_ALL)
                {
                    WriteDepth(om, px, py, depth);
                }

                if (om.stencilEnabled)
                {
                    Uint8 oldStencil = ReadStencil(om, px, py);
                    const D3D11_DEPTH_STENCILOP_DESC* stencilOps = ctx.frontFace ? &om.stencilFront : &om.stencilBack;
                    Uint8 result = ApplyStencilOp(stencilOps->StencilPassOp, oldStencil, om.stencilRef);
                    Uint8 written = (result & om.stencilWriteMask) | (oldStencil & ~om.stencilWriteMask);
                    WriteStencil(om, px, py, written);
                }

                WritePixel(om, px, py, psOut);
            }

            w0 += ctx.w0_dx;
            w1 += ctx.w1_dx;
            w2 += ctx.w2_dx;
        }

        w0_pixRowStart += ctx.w0_dy;
        w1_pixRowStart += ctx.w1_dy;
        w2_pixRowStart += ctx.w2_dy;
    }

    if (ctx.hiZ && ctx.om->depthEnabled && ctx.om->depthWriteMask == D3D11_DEPTH_WRITE_MASK_ALL)
    {
        OMState& om = *ctx.om;
        Int gCol = tileX / ctx.hiZTileSize;
        Int gRow = tileY / ctx.hiZTileSize;
        Int scanMaxX = std::min(tileX + ctx.tileStepX, ctx.hiZWidth);
        Int scanMaxY = std::min(tileY + ctx.tileStepY, ctx.hiZHeight);
        Float maxZ = 0.f;
        for (Int sy = tileY; sy < scanMaxY; ++sy)
        {
            for (Int sx = tileX; sx < scanMaxX; ++sx)
            {
                Uint8* p = om.dsvData + (Uint64)sy * om.dsvRowPitch + (Uint64)sx * om.dsvPixStride;
                maxZ = std::max(maxZ, ReadDepthValue(om.dsvFmt, p));
            }
        }
        ctx.hiZ[gRow * ctx.hiZTilesX + gCol] = maxZ;
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
    if (inCount < 3) 
    { 
        return 0; 
    }

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
    if (inCount < 3) 
    { 
        return 0; 
    }

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
                        Float guardBandK, Bool depthClip, Int numClipDist,
                        ScratchArena& arena)
{
    Bool anyNear = false;
    Bool anyOther = false;
    for (Int v = 0; v < 3; ++v)
    {
        const SW_float4& p = in[v].pos;
        if (p.w < NearEpsilon) 
        { 
            anyNear = true; 
            break; 
        }

        Float Kw = guardBandK * p.w;
        if (p.x < -Kw || p.x > Kw || p.y < -Kw || p.y > Kw
            || (depthClip && p.z > p.w))
        {
            anyOther = true;
        }
        for (Int c = 0; c < numClipDist; ++c)
        {
            if (in[v].clipDist[c] < 0.f) 
            { 
                anyOther = true; 
            }
        }
    }

    if (!anyNear && !anyOther)
    {
        out[0][0] = in[0];
        out[0][1] = in[1];
        out[0][2] = in[2];
        return 1;
    }

    SW_VSOutput* bufA = arena.Alloc<SW_VSOutput>(MaxClipVerts);
    SW_VSOutput* bufB = arena.Alloc<SW_VSOutput>(MaxClipVerts);

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

            if (aIn) 
            { 
                dst[outCount++] = a; 
            }
            if (aIn != bIn)
            {
                Float t = dA / (dA - dB);
                dst[outCount++] = LerpVertex(a, b, t);
            }
        }
        count = outCount;
        std::swap(src, dst);
        if (count < 3) 
        { 
            return 0; 
        }
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
            if (count < 3) 
            { 
                return 0; 
            }
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
            if (count < 3) 
            { 
                return 0; 
            }
        }
    }

    for (Int c = 0; c < numClipDist; ++c)
    {
        Int outCount = ClipPolygonAgainstClipDist(src, count, dst, c);
        count = outCount;
        std::swap(src, dst);
        if (count < 3) 
        { 
            return 0; 
        }
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
            if (tri[v].clipDist[c] < 0.f) 
            { 
                needsClip = true; 
                break; 
            }
        }

        if (needsClip) 
        { 
            break; 
        }
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
            ScratchArena arena;
            using TriVerts = SW_VSOutput[3];
            TriVerts* clipped = arena.Alloc<TriVerts>(MaxClipVerts - 2);
            Int n = ClipTriangle(tri, clipped, _config.guardBandK, depthClip, numClipDist, arena);
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

    //https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm
    // Post-clipped vertex positions are snapped to 28.4 fixed point, to uniformly distribute
    // precision across the RenderTarget area. Rasterizer operations such as face culling occur
    // on fixed point snapped positions, while attribute interpolator setup uses positions that
    // have been converted back to floating point from the fixed point snapped positions.
    auto toFixed = [](Float v) -> Fixed28_4 { return Fixed28_4::FromFloat(v); };

    Fixed28_4 fx[3], fy[3];
    for (Int v = 0; v < 3; ++v)
    {
        fx[v] = toFixed(screenX[v]);
        fy[v] = toFixed(screenY[v]);
        screenX[v] = fx[v].ToFloat();
        screenY[v] = fy[v].ToFloat();
    }

    Fixed28_4 fixedArea2 = (fx[1] - fx[0]) * (fy[2] - fy[0]) - (fy[1] - fy[0]) * (fx[2] - fx[0]);
    if (fixedArea2 == 0)
    {
        return;
    }

    const Bool frontFace = rsDesc.FrontCounterClockwise ? (fixedArea2 > 0) : (fixedArea2 < 0);

    if (rsDesc.CullMode == D3D11_CULL_BACK  && !frontFace)
    {
        return;
    }

    if (rsDesc.CullMode == D3D11_CULL_FRONT &&  frontFace)
    {
        return;
    }

    Float depthBias = 0.f;
    if (rsDesc.DepthBias != 0 || rsDesc.SlopeScaledDepthBias != 0.f)
    {
        //https://learn.microsoft.com/en-us/windows/win32/direct3d11/d3d10-graphics-programming-guide-output-merger-stage-depth-bias
        Float edgeArea = (screenX[1] - screenX[0]) * (screenY[2] - screenY[0])
                       - (screenY[1] - screenY[0]) * (screenX[2] - screenX[0]);
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

    if (minX >= maxX || minY >= maxY) 
    { 
        return; 
    }

    auto edgeFn = [](Fixed28_4 ax, Fixed28_4 ay, Fixed28_4 bx, Fixed28_4 by, Fixed28_4 px, Fixed28_4 py) -> Fixed28_4
    {
        return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
    };

    //https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm#3.4.2.1%20Top-Left%20Rule
    auto isTopLeft = [&](Int a, Int b) -> Bool
    {
        Fixed28_4 edgeY = fy[b] - fy[a];
        Fixed28_4 edgeX = fx[b] - fx[a];
        Bool isTop  = (edgeY == 0 && edgeX > 0);
        Bool isLeft = (edgeY < 0);
        return isTop || isLeft;
    };

    Fixed28_4 bias0 = isTopLeft(1, 2) ? Fixed28_4(0) : Fixed28_4(-1);
    Fixed28_4 bias1 = isTopLeft(2, 0) ? Fixed28_4(0) : Fixed28_4(-1);
    Fixed28_4 bias2 = isTopLeft(0, 1) ? Fixed28_4(0) : Fixed28_4(-1);

    Float invArea2 = 1.f / static_cast<Float>(fixedArea2);
    Int svPosRegPS = FindSVPositionInput(psReflection);

    VaryingMap varyings[D3D11_VS_OUTPUT_REGISTER_COUNT];
    Int numVaryings = 0;
    for (const auto& psIn : psReflection.inputs)
    {
        if (psIn.svType != 0) 
        { 
            continue; 
        }

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

    if (om.activeRTCount == 0)
    {
        return;
    }

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

    Fixed28_4 tileStartX = Fixed28_4(static_cast<Int64>(tileMinX) * 16 + 8);
    Fixed28_4 tileStartY = Fixed28_4(static_cast<Int64>(tileMinY) * 16 + 8);

    Fixed28_4 w0_dx = -(fy[2] - fy[1]) * 16;
    Fixed28_4 w1_dx = -(fy[0] - fy[2]) * 16;
    Fixed28_4 w2_dx = -(fy[1] - fy[0]) * 16;

    Fixed28_4 w0_dy = (fx[2] - fx[1]) * 16;
    Fixed28_4 w1_dy = (fx[0] - fx[2]) * 16;
    Fixed28_4 w2_dy = (fx[1] - fx[0]) * 16;

    Fixed28_4 w0_tileOrig = edgeFn(fx[1], fy[1], fx[2], fy[2], tileStartX, tileStartY) + bias0;
    Fixed28_4 w1_tileOrig = edgeFn(fx[2], fy[2], fx[0], fy[0], tileStartX, tileStartY) + bias1;
    Fixed28_4 w2_tileOrig = edgeFn(fx[0], fy[0], fx[1], fy[1], tileStartX, tileStartY) + bias2;

    Fixed28_4 w0_tileStepX = w0_dx * TILE;
    Fixed28_4 w1_tileStepX = w1_dx * TILE;
    Fixed28_4 w2_tileStepX = w2_dx * TILE;

    Fixed28_4 w0_tileStepY = w0_dy * TILE;
    Fixed28_4 w1_tileStepY = w1_dy * TILE;
    Fixed28_4 w2_tileStepY = w2_dy * TILE;

    Fixed28_4 w0_cornerDx = w0_dx * (TILE - 1);
    Fixed28_4 w1_cornerDx = w1_dx * (TILE - 1);
    Fixed28_4 w2_cornerDx = w2_dx * (TILE - 1);

    Fixed28_4 w0_cornerDy = w0_dy * (TILE - 1);
    Fixed28_4 w1_cornerDy = w1_dy * (TILE - 1);
    Fixed28_4 w2_cornerDy = w2_dy * (TILE - 1);

    Int numTilesX = (maxX - tileMinX + tileStepX - 1) / tileStepX;
    Int numTilesY = (maxY - tileMinY + tileStepY - 1) / tileStepY;
    Int totalTiles = numTilesX * numTilesY;

    if (om.dsvData && !_hiZ.Matches(om.dsvData))
    {
        ClearHiZ(om.dsvData, om.dsvWidth, om.dsvHeight, 1.0f);
    }

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

    D3D11PixelShaderSW* psSW = state.ps;
    tctx.quadMode  = psReflection.needsQuad && psSW && psSW->GetQuadFn();
    tctx.psQuadFn  = tctx.quadMode ? psSW->GetQuadFn() : nullptr;
    if (tctx.quadMode && (psReflection.usesDiscard || psReflection.writesSVDepth || psReflection.usesUAVs))
    {
        tctx.earlyZ = false;
    }

    tctx.hiZ        = _hiZ.Matches(om.dsvData) ? _hiZ.data.data() : nullptr;
    tctx.hiZTilesX  = _hiZ.tilesX;
    tctx.hiZWidth   = _hiZ.width;
    tctx.hiZHeight  = _hiZ.height;
    tctx.hiZTileSize = _hiZ.tileSize;

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
    if (state.numViewports == 0) 
    { 
        return; 
    }

    const D3D11_VIEWPORT& vp = state.viewports[0];
    for (Int v = 0; v < 2; ++v)
    {
        if (endpts[v].pos.w <= 0.f) 
        { 
            return;
        }
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

    if (om.activeRTCount == 0) 
    { 
        return; 
    }
    Int svPosRegPS = FindSVPositionInput(psReflection);

    VaryingMap varyings[D3D11_VS_OUTPUT_REGISTER_COUNT];
    Int numVaryings = 0;
    for (const auto& psIn : psReflection.inputs)
    {
        if (psIn.svType != 0) 
        { 
            continue; 
        }

        Int vsOutReg = FindSemanticRegister(vsReflection.outputs, psIn.name, psIn.semanticIndex);
        if (vsOutReg >= 0)
        {
            varyings[numVaryings++] = { vsOutReg, static_cast<Int>(psIn.reg) };
        }
    }

    Float dx = screenX[1] - screenX[0];
    Float dy = screenY[1] - screenY[0];
    Int steps = static_cast<Int>(std::max(std::fabs(dx), std::fabs(dy)));
    if (steps == 0) 
    { 
        steps = 1;
    }

    Float xInc = dx / steps;
    Float yInc = dy / steps;
    for (Int s = 0; s <= steps; ++s)
    {
        Float fx = screenX[0] + xInc * s;
        Float fy = screenY[0] + yInc * s;
        Int px = static_cast<Int>(fx);
        Int py = static_cast<Int>(fy);

        if (px < vpMinX || px >= vpMaxX || py < vpMinY || py >= vpMaxY) 
        { 
            continue; 
        }
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
    if (state.numViewports == 0) 
    { 
        return; 
    }

    const D3D11_VIEWPORT& vp = state.viewports[0];
    if (point.pos.w <= 0.f) 
    { 
        return; 
    }

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

    if (px < vpMinX || px >= vpMaxX || py < vpMinY || py >= vpMaxY) 
    { 
        return; 
    }

    if (om.activeRTCount == 0) 
    { 
        return; 
    }

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
        if (pi.svType != 0) 
        { 
            continue; 
        }

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
    const UINT* indices, Uint vertexCount, Int baseVertex,
    D3D11SW_PIPELINE_STATE& state)
{
    SW_PSFn psFn = state.ps->GetJitFn();
    if (!vs.vsFn || !psFn) 
    { 
        return; 
    }

    const D3D11SW_ParsedShader& vsRefl = *vs.vsReflection;
    const D3D11SW_ParsedShader& psRefl = state.ps->GetReflection();

    SW_Resources psRes{};
    BuildStageResources(psRes, state.psCBs, state.psCBOffsets, state.psSRVs, state.psSamplers);
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
    Uint vertexCount, Uint startVertex,
    Uint instanceCount, Uint startInstance,
    D3D11SW_PIPELINE_STATE& state)
{
    if (!state.vs || !state.ps) 
    {
        return;
    }

    VertexState vs = InitVS(state);
    OMState om = InitOM(state);

    for (Uint inst = 0; inst < instanceCount; ++inst)
    {
        vs.instanceID = startInstance + inst;
        if (vs.cacheEnabled) 
        { 
            vs.cache.clear(); 
        }
        DrawInternal(vs, om, nullptr, vertexCount, static_cast<INT>(startVertex), state);
    }
}

void SWRasterizer::DrawIndexed(
    Uint indexCount, Uint startIndex, Int baseVertex,
    Uint instanceCount, Uint startInstance,
    D3D11SW_PIPELINE_STATE& state)
{
    if (!state.vs || !state.ps) 
    { 
        return; 
    }

    VertexState vs = InitVS(state);
    OMState om = InitOM(state);
    std::vector<Uint> indices(indexCount);
    for (UINT i = 0; i < indexCount; ++i)
    {
        indices[i] = FetchIndex(vs, startIndex + i);
    }

    for (Uint inst = 0; inst < instanceCount; ++inst)
    {
        vs.instanceID = startInstance + inst;
        if (vs.cacheEnabled) 
        { 
            vs.cache.clear(); 
        }

        DrawInternal(vs, om, indices.data(), indexCount, baseVertex, state);
    }
}

}
