#include "core/rasterizer/output_merger.h"

namespace d3dsw {

OMState InitOM(const SWPipelineState& state)
{
    OMState om{};
    om.depthEnabled       = state.depthStencilDesc.depthEnable;
    om.depthFunc          = state.depthStencilDesc.depthFunc;
    om.depthWriteMask     = state.depthStencilDesc.depthWriteMask;
    om.stencilEnabled     = state.depthStencilDesc.stencilEnable;
    om.stencilReadMask    = state.depthStencilDesc.stencilReadMask;
    om.stencilWriteMask   = state.depthStencilDesc.stencilWriteMask;
    om.stencilFront       = state.depthStencilDesc.frontFace;
    om.stencilBack        = state.depthStencilDesc.backFace;
    om.stencilRef         = state.stencilRef;

    om.dsvData        = state.depthStencilView.data;
    om.dsvFmt         = state.depthStencilView.format;
    om.dsvRowPitch    = state.depthStencilView.rowPitch;
    om.dsvPixStride   = state.depthStencilView.pixStride;
    om.dsvSampleCount = state.depthStencilView.sampleCount;
    om.dsvWidth       = (om.dsvPixStride > 0 && om.dsvSampleCount > 0)
                            ? static_cast<Int>(om.dsvRowPitch / (om.dsvPixStride * om.dsvSampleCount))
                            : 0;
    om.dsvHeight      = static_cast<Int>(state.depthStencilView.numRows);
    if (!om.dsvData)
    {
        om.depthEnabled = false;
    }

    if (!om.dsvData || !FormatHasStencil(om.dsvFmt))
    {
        om.stencilEnabled = false;
    }

    om.activeRTCount = 0;
    for (Uint rt = 0; rt < state.numRenderTargets; ++rt)
    {
        const SWRenderTargetSnapshot& src = state.renderTargets[rt];
        if (!src.data)
        {
            continue;
        }

        RTInfo& info     = om.rtInfos[om.activeRTCount++];
        info.data        = src.data;
        info.fmt         = src.format;
        info.rowPitch    = src.rowPitch;
        info.pixStride   = src.pixStride;
        info.slicePitch  = src.slicePitch;
        info.sampleCount = src.sampleCount;
        info.blendDesc   = state.blendDesc.renderTarget[rt];
    }

    om.blendFactor  = state.blendFactor;
    om.rtArrayIndex = 0;

    om.sampleCount   = 1;
    om.sampleQuality = 0;
    if (om.activeRTCount > 0)
    {
        om.sampleCount   = om.rtInfos[0].sampleCount;
        om.sampleQuality = state.renderTargets[0].sampleQuality;
    }
    else if (om.dsvData)
    {
        om.sampleCount   = om.dsvSampleCount;
        om.sampleQuality = state.depthStencilView.sampleQuality;
    }
    om.sampleMask = state.sampleMask;

    om.alphaTestEnable = state.alphaTestEnable;
    om.alphaTestFunc   = state.alphaTestFunc;
    om.alphaRef        = state.alphaRef;
    om.fogDesc         = state.fogDesc;

    return om;
}

}
