#include "context/output_merger.h"
#include "views/render_target_view.h"
#include "views/depth_stencil_view.h"
#include "states/blend_state.h"
#include "states/depth_stencil_state.h"

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
    om.dsvWidth  = (dsv && om.dsvPixStride > 0) ? static_cast<Int>(dsv->GetLayout().RowPitch / om.dsvPixStride) : 0;
    om.dsvHeight = dsv ? static_cast<Int>(dsv->GetLayout().NumRows) : 0;
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

}
