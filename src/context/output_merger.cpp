#include "context/output_merger.h"
#include "context/context_util.h"
#include "context/pipeline_state.h"
#include "states/blend_state.h"
#include "states/depth_stencil_state.h"
#include "views/render_target_view.h"
#include "views/depth_stencil_view.h"

namespace d3d11sw {

OutputMerger::OutputMerger(D3D11SW_PIPELINE_STATE& state)
{
    _depthEnabled = true;
    _depthFunc = D3D11_COMPARISON_LESS;
    _depthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    _stencilEnabled = false;
    _stencilReadMask = 0xFF;
    _stencilWriteMask = 0xFF;
    _stencilRef = 0;
    _stencilFront = {};
    _stencilBack = {};
    _stencilFrontFunc = D3D11_COMPARISON_ALWAYS;
    _stencilBackFunc = D3D11_COMPARISON_ALWAYS;
    if (state.depthStencilState)
    {
        D3D11_DEPTH_STENCIL_DESC dsDesc{};
        state.depthStencilState->GetDesc(&dsDesc);
        _depthEnabled   = dsDesc.DepthEnable ? true : false;
        _depthFunc      = dsDesc.DepthFunc;
        _depthWriteMask = dsDesc.DepthWriteMask;
        _stencilEnabled   = dsDesc.StencilEnable ? true : false;
        _stencilReadMask  = dsDesc.StencilReadMask;
        _stencilWriteMask = dsDesc.StencilWriteMask;
        _stencilFront     = dsDesc.FrontFace;
        _stencilBack      = dsDesc.BackFace;
        _stencilFrontFunc = dsDesc.FrontFace.StencilFunc;
        _stencilBackFunc  = dsDesc.BackFace.StencilFunc;
    }
    _stencilRef = static_cast<UINT8>(state.stencilRef);

    D3D11DepthStencilViewSW* dsv = state.depthStencilView;
    _dsvData     = dsv ? dsv->GetDataPtr() : nullptr;
    _dsvFmt      = dsv ? dsv->GetFormat()  : DXGI_FORMAT_UNKNOWN;
    _dsvRowPitch = dsv ? dsv->GetLayout().RowPitch : 0;
    _dsvPixStride = dsv ? DepthPixelStride(_dsvFmt) : 0;
    if (!dsv) { _depthEnabled = false; }
    if (!dsv || !FormatHasStencil(_dsvFmt)) { _stencilEnabled = false; }

    D3D11_BLEND_DESC1 bsDesc{};
    Bool haveBlendState = false;
    if (state.blendState)
    {
        state.blendState->GetDesc1(&bsDesc);
        haveBlendState = true;
    }

    _activeRTCount = 0;
    for (UINT rt = 0; rt < state.numRenderTargets; ++rt)
    {
        D3D11RenderTargetViewSW* rtv = state.renderTargets[rt];
        if (!rtv) { continue; }
        RTInfo& info    = _rtInfos[_activeRTCount++];
        info.data       = rtv->GetDataPtr();
        info.fmt        = rtv->GetFormat();
        info.rowPitch   = rtv->GetLayout().RowPitch;
        info.pixStride  = rtv->GetLayout().PixelStride;
        info.blendDesc  = {};
        info.blendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if (haveBlendState) { info.blendDesc = bsDesc.RenderTarget[rt]; }
    }

    _blendFactor = state.blendFactor;
}

void OutputMerger::WritePixel(Int px, Int py, const SW_PSOutput& psOut)
{
    for (UINT rt = 0; rt < _activeRTCount; ++rt)
    {
        BlendAndWrite(px, py, rt, psOut.oC[rt]);
    }
}

void OutputMerger::BlendAndWrite(Int px, Int py, UINT rtIdx, const SW_float4& color)
{
    const RTInfo& info = _rtInfos[rtIdx];
    UINT8* rtvPx = info.data + (UINT64)py * info.rowPitch + (UINT64)px * info.pixStride;

    FLOAT srcColor[4] = { color.x, color.y, color.z, color.w };
    FLOAT finalColor[4];
    if (info.blendDesc.BlendEnable)
    {
        FLOAT dstColor[4];
        UnpackColor(info.fmt, rtvPx, dstColor);
        for (Int c = 0; c < 3; ++c)
        {
            Float sf = ComputeBlendFactor(info.blendDesc.SrcBlend, srcColor, dstColor, _blendFactor, c);
            Float df = ComputeBlendFactor(info.blendDesc.DestBlend, srcColor, dstColor, _blendFactor, c);
            finalColor[c] = ComputeBlendOp(info.blendDesc.BlendOp, srcColor[c] * sf, dstColor[c] * df);
        }
        {
            Float sf = ComputeBlendFactor(info.blendDesc.SrcBlendAlpha, srcColor, dstColor, _blendFactor, 3);
            Float df = ComputeBlendFactor(info.blendDesc.DestBlendAlpha, srcColor, dstColor, _blendFactor, 3);
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

}
