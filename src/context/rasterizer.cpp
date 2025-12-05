#include "context/rasterizer.h"
#include "context/context_util.h"
#include "states/rasterizer_state.h"
#include "util/env.h"
#include <algorithm>
#include <atomic>
#include <cstring>
#include <latch>
#include <semaphore>
#include <thread>
#include <vector>

namespace d3d11sw {

Rasterizer::Config Rasterizer::Config::FromEnv()
{
    Config cfg;
    cfg.tiling       = GetEnvBool("D3D11SW_TILING", true);
    cfg.tileSize     = GetEnvInt("D3D11SW_TILE_SIZE", 16);
    cfg.tileThreads  = GetEnvInt("D3D11SW_TILE_THREADS", -1);
    if (cfg.tileSize < 4 || (cfg.tileSize & (cfg.tileSize - 1)) != 0)
    {
        cfg.tileSize = 16;
    }
    if (cfg.tileThreads == -1)
    {
        cfg.tileThreads = static_cast<Int>(std::thread::hardware_concurrency());
        if (cfg.tileThreads < 1) { cfg.tileThreads = 1; }
    }
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

Rasterizer::Rasterizer() : _config(Config::FromEnv()) {}
Rasterizer::~Rasterizer() = default;

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
    Float vpMinDepth, vpMaxDepth;

    const SW_float4* v0pw;
    const SW_float4* v1pw;
    const SW_float4* v2pw;
    const VaryingMap* varyings;
    Int numVaryings;
    Int svPosRegPS;

    OutputMerger* om;

    SW_PSFn psFn;
    SW_Resources* psRes;

    Int tileMinX, tileMinY;
    Int minX, minY, maxX, maxY;
    Int tileStepX, tileStepY;
    Int numTilesX;
    Bool useTiling;
};

static void ProcessOneTile(const TileContext& ctx, Uint32 tileIdx,
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

    OutputMerger& om = *ctx.om;
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
                depth = std::clamp(depth, ctx.vpMinDepth, ctx.vpMaxDepth);

                if (om.DepthEnabled())
                {
                    if (!om.TestDepth(px, py, depth))
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

                    Float ax = (b0 * ctx.v0pw[vi].x + b1 * ctx.v1pw[vi].x + b2 * ctx.v2pw[vi].x) * invPerspW;
                    Float ay = (b0 * ctx.v0pw[vi].y + b1 * ctx.v1pw[vi].y + b2 * ctx.v2pw[vi].y) * invPerspW;
                    Float az = (b0 * ctx.v0pw[vi].z + b1 * ctx.v1pw[vi].z + b2 * ctx.v2pw[vi].z) * invPerspW;
                    Float aw = (b0 * ctx.v0pw[vi].w + b1 * ctx.v1pw[vi].w + b2 * ctx.v2pw[vi].w) * invPerspW;

                    psIn.v[psR] = { ax, ay, az, aw };
                }

                psOut = {};
                ctx.psFn(&psIn, &psOut, ctx.psRes);

                if (om.DepthEnabled() && om.DepthWriteEnabled())
                {
                    Float writeD = psOut.depthWritten ? psOut.oDepth : depth;
                    om.WriteDepth(px, py, writeD);
                }

                om.WritePixel(px, py, psOut);
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

Int Rasterizer::FindSVPositionInput(const D3D11SW_ParsedShader& shader)
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

Int Rasterizer::FindSemanticRegister(const std::vector<D3D11SW_ShaderSignatureElement>& sig,
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

void Rasterizer::RasterizeTriangle(
    const SW_VSOutput tri[3],
    const D3D11SW_ParsedShader& vsReflection,
    const D3D11SW_ParsedShader& psReflection,
    SW_PSFn psFn,
    SW_Resources& psRes,
    OutputMerger& om,
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

    if (!om.HasRenderTargets()) { return; }

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

void Rasterizer::RasterizeLine(
    const SW_VSOutput endpts[2],
    const D3D11SW_ParsedShader& vsReflection,
    const D3D11SW_ParsedShader& psReflection,
    SW_PSFn psFn,
    SW_Resources& psRes,
    OutputMerger& om,
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

    if (!om.HasRenderTargets()) { return; }

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

        om.WritePixel(px, py, psOut);
    }
}

void Rasterizer::RasterizePoint(
    const SW_VSOutput& point,
    const D3D11SW_ParsedShader& vsReflection,
    const D3D11SW_ParsedShader& psReflection,
    SW_PSFn psFn,
    SW_Resources& psRes,
    OutputMerger& om,
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

    if (!om.HasRenderTargets()) { return; }

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

    om.WritePixel(px, py, psOut);
}

}
