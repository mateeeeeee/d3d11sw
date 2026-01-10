#include "context/output_merger.h"
#include "context/blend_util.h"
#include "context/context_util.h"
#include "views/render_target_view.h"
#include "views/depth_stencil_view.h"
#include "states/blend_state.h"
#include "states/depth_stencil_state.h"
#include <cstring>

namespace d3d11sw {

OMState InitOM(D3D11SW_PIPELINE_STATE& state)
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
    if (!dsv)
    {
        om.depthEnabled = false;
    }

    if (!dsv || !FormatHasStencil(om.dsvFmt))
    {
        om.stencilEnabled = false;
    }

    D3D11_BLEND_DESC1 bsDesc{};
    Bool haveBlendState = false;
    if (state.blendState)
    {
        state.blendState->GetDesc1(&bsDesc);
        haveBlendState = true;
    }

    om.activeRTCount = 0;
    for (Uint rt = 0; rt < state.numRenderTargets; ++rt)
    {
        D3D11RenderTargetViewSW* rtv = state.renderTargets[rt];
        if (!rtv)
        {
            continue;
        }

        RTInfo& info    = om.rtInfos[om.activeRTCount++];
        info.data       = rtv->GetDataPtr();
        info.fmt        = rtv->GetFormat();
        info.rowPitch   = rtv->GetLayout().RowPitch;
        info.pixStride  = rtv->GetLayout().PixelStride;
        info.blendDesc  = {};
        info.blendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (haveBlendState)
        {
            info.blendDesc = bsDesc.RenderTarget[rt];
        }
    }

    om.blendFactor = state.blendFactor;
    return om;
}

Bool TestDepth(const OMState& om, Int px, Int py, Float depth)
{
    Uint8* dsvPx = om.dsvData + (Uint64)py * om.dsvRowPitch + (Uint64)px * om.dsvPixStride;
    Float existing = ReadDepthValue(om.dsvFmt, dsvPx);
    return CompareDepth(om.depthFunc, depth, existing);
}

void WriteDepth(OMState& om, Int px, Int py, Float depth)
{
    Uint8* dsvPx = om.dsvData + (Uint64)py * om.dsvRowPitch + (Uint64)px * om.dsvPixStride;
    WriteDepthValue(om.dsvFmt, dsvPx, depth);
}

UINT8 ReadStencil(const OMState& om, Int px, Int py)
{
    Uint8* dsvPx = om.dsvData + (Uint64)py * om.dsvRowPitch + (Uint64)px * om.dsvPixStride;
    return ReadStencilValue(om.dsvFmt, dsvPx);
}

void WriteStencil(OMState& om, Int px, Int py, Uint8 val)
{
    Uint8* dsvPx = om.dsvData + (Uint64)py * om.dsvRowPitch + (Uint64)px * om.dsvPixStride;
    WriteStencilValue(om.dsvFmt, dsvPx, val);
}

void WritePixel(OMState& om, Int px, Int py, const SW_PSOutput& psOut)
{
    for (Uint rt = 0; rt < om.activeRTCount; ++rt)
    {
        BlendAndWrite(om, px, py, rt, psOut.oC[rt], psOut.oC[1]);
    }
}

void BlendAndWrite(const OMState& om, Int px, Int py, Uint rtIdx,
                   const SW_float4& color, const SW_float4& src1Color)
{
    const RTInfo& info = om.rtInfos[rtIdx];
    Uint8* rtvPx = info.data + (Uint64)py * info.rowPitch + (Uint64)px * info.pixStride;
    if (info.blendDesc.LogicOpEnable)
    {
        Float srcColorF[4] = { color.x, color.y, color.z, color.w };
        Uint8 srcPacked[16];
        PackRTVColor(info.fmt, srcColorF, srcPacked);

        Uint8 result[16];
        for (Uint b = 0; b < info.pixStride; ++b)
        {
            result[b] = ApplyLogicOp(info.blendDesc.LogicOp, srcPacked[b], rtvPx[b]);
        }
        Uint8 writeMask = info.blendDesc.RenderTargetWriteMask;
        if (writeMask != D3D11_COLOR_WRITE_ENABLE_ALL)
        {
            Uint bytesPerComp = info.pixStride / 4;
            if (bytesPerComp == 0) { bytesPerComp = 1; }
            for (Uint c = 0; c < 4; ++c)
            {
                if (!(writeMask & (1 << c)))
                {
                    for (Uint b = c * bytesPerComp; b < (c + 1) * bytesPerComp && b < info.pixStride; ++b)
                    {
                        result[b] = rtvPx[b];
                    }
                }
            }
        }

        std::memcpy(rtvPx, result, info.pixStride);
        return;
    }

    Float srcColor[4] = { color.x, color.y, color.z, color.w };
    Float src1[4] = { src1Color.x, src1Color.y, src1Color.z, src1Color.w };
    Float finalColor[4] = {};
    if (info.blendDesc.BlendEnable)
    {
        Float dstColor[4];
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

    Uint8 writeMask = info.blendDesc.RenderTargetWriteMask;
    if (writeMask != D3D11_COLOR_WRITE_ENABLE_ALL)
    {
        Float existing[4];
        UnpackColor(info.fmt, rtvPx, existing);
        if (!(writeMask & D3D11_COLOR_WRITE_ENABLE_RED))   { finalColor[0] = existing[0]; }
        if (!(writeMask & D3D11_COLOR_WRITE_ENABLE_GREEN)) { finalColor[1] = existing[1]; }
        if (!(writeMask & D3D11_COLOR_WRITE_ENABLE_BLUE))  { finalColor[2] = existing[2]; }
        if (!(writeMask & D3D11_COLOR_WRITE_ENABLE_ALPHA)) { finalColor[3] = existing[3]; }
    }

    Uint8 packed[16];
    PackRTVColor(info.fmt, finalColor, packed);
    std::memcpy(rtvPx, packed, info.pixStride);
}

}
