#include "pipeline_state.h"
#include "core/pipeline/sw_pipeline_state.h"
#include "d3d11/misc/input_layout.h"
#include "d3d11/shaders/vertex_shader.h"
#include "d3d11/shaders/pixel_shader.h"
#include "d3d11/shaders/geometry_shader.h"
#include "d3d11/shaders/hull_shader.h"
#include "d3d11/shaders/domain_shader.h"
#include "d3d11/shaders/compute_shader.h"
#include "d3d11/states/blend_state.h"
#include "d3d11/states/depth_stencil_state.h"
#include "d3d11/states/rasterizer_state.h"
#include "d3d11/states/sampler_state.h"
#include "d3d11/views/render_target_view.h"
#include "d3d11/views/depth_stencil_view.h"
#include "d3d11/views/shader_resource_view.h"
#include "d3d11/views/unordered_access_view.h"
#include "d3d11/resources/buffer.h"
#include <algorithm>
#include <cstring>

namespace d3dsw {


void D3DSW_PIPELINE_STATE::ReleaseAll()
{
    auto rel  = [](auto*& p)   { if (p) { p->Release(); p = nullptr; } };
    auto relA = [&](auto& arr) { for (auto& p : arr) rel(p); };

    rel(inputLayout);
    rel(indexBuffer);
    relA(vertexBuffers);

    rel(vs); rel(ps); rel(gs); rel(hs); rel(ds); rel(cs);

    relA(vsCBs); relA(psCBs); relA(gsCBs);
    relA(hsCBs); relA(dsCBs); relA(csCBs);

    relA(vsSRVs); relA(psSRVs); relA(gsSRVs);
    relA(hsSRVs); relA(dsSRVs); relA(csSRVs);

    relA(csUAVs);
    relA(psUAVs);

    relA(vsSamplers); relA(psSamplers); relA(gsSamplers);
    relA(hsSamplers); relA(dsSamplers); relA(csSamplers);

    rel(rsState);
    relA(renderTargets);
    rel(depthStencilView);
    rel(blendState);
    rel(depthStencilState);

    for (auto& t : soTargets) { rel(t.buffer); }
}

static void BuildStageResources(
    SW_Resources& res,
    D3D11BufferSW* const* cbs,
    const Uint* cbOffsets,
    D3D11ShaderResourceViewSW* const* srvs,
    D3D11SamplerStateSW* const* samplers)
{
    for (Uint i = 0; i < SW_MAX_CBUFS; ++i)
    {
        if (cbs[i])
        {
            res.cb[i] = static_cast<const SW_float4*>(cbs[i]->GetDataPtr()) + cbOffsets[i];
        }
    }

    for (Uint i = 0; i < SW_MAX_TEXTURES; ++i)
    {
        D3D11ShaderResourceViewSW* srv = srvs[i];
        if (!srv) 
        { 
            continue; 
        }

        D3DSW_SUBRESOURCE_LAYOUT layout = srv->GetLayout();
        SW_SRV& tex = res.srv[i];
        tex.data        = srv->GetDataPtr();
        tex.format      = srv->GetFormat();
        tex.mipLevels   = srv->GetViewMipCount();
        tex.stride      = srv->GetStride();
        tex.sampleCount = layout.SampleCount;

        for (Uint m = 0; m < tex.mipLevels && m < SW_MAX_MIP_LEVELS; ++m)
        {
            D3DSW_SUBRESOURCE_LAYOUT ml = srv->GetMipLayout(m);
            tex.mips[m].width     = ml.PixelStride > 0 ? (ml.RowPitch / (ml.PixelStride * ml.SampleCount)) * ml.BlockSize : 0;
            tex.mips[m].height    = ml.NumRows * ml.BlockSize;
            tex.mips[m].depth     = ml.NumSlices;
            tex.mips[m].rowPitch  = ml.RowPitch;
            tex.mips[m].slicePitch = ml.DepthPitch;
            D3DSW_SUBRESOURCE_LAYOUT m0 = srv->GetMipLayout(0);
            tex.mipOffsets[m] = static_cast<unsigned>(ml.Offset - m0.Offset);
        }
    }

    for (Uint i = 0; i < SW_MAX_SAMPLERS; ++i)
    {
        D3D11SamplerStateSW* smp = samplers[i];
        if (!smp) 
        { 
            continue; 
        }

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
        res.smp[i].borderColor[0]  = desc.BorderColor[0];
        res.smp[i].borderColor[1]  = desc.BorderColor[1];
        res.smp[i].borderColor[2]  = desc.BorderColor[2];
        res.smp[i].borderColor[3]  = desc.BorderColor[3];
        res.smp[i].maxAnisotropy   = desc.MaxAnisotropy;
    }
}

static void BuildStageUAVs(SW_Resources& res, D3D11UnorderedAccessViewSW* const* uavs)
{
    for (Uint i = 0; i < SW_MAX_UAVS; ++i)
    {
        D3D11UnorderedAccessViewSW* uavSW = uavs[i];
        if (!uavSW)
        {
            continue;
        }

        D3D11_UNORDERED_ACCESS_VIEW_DESC1 desc{};
        uavSW->GetDesc1(&desc);
        D3DSW_SUBRESOURCE_LAYOUT layout = uavSW->GetLayout();

        SW_UAV& uav  = res.uav[i];
        uav.data      = uavSW->GetDataPtr();
        uav.format    = desc.Format;
        uav.dimension = static_cast<D3D11_UAV_DIMENSION>(desc.ViewDimension);
        uav.counter   = (uavSW->GetFlags() & (D3D11_BUFFER_UAV_FLAG_APPEND | D3D11_BUFFER_UAV_FLAG_COUNTER))
                        ? uavSW->GetCounter() : nullptr;
        if (desc.ViewDimension == D3D11_UAV_DIMENSION_BUFFER)
        {
            uav.elementCount = desc.Buffer.NumElements;
            uav.stride       = layout.PixelStride;
        }
        else
        {
            uav.width        = layout.PixelStride > 0 ? layout.RowPitch / layout.PixelStride : 0;
            uav.height       = layout.NumRows;
            uav.depth        = layout.NumSlices;
            uav.rowPitch     = layout.RowPitch;
            uav.slicePitch   = layout.DepthPitch;
            uav.stride       = layout.PixelStride;
            uav.elementCount = uav.width * uav.height;
        }
    }
}

static_assert(sizeof(D3DSW_PIPELINE_STATISTICS) == sizeof(SWPipelineStatistics),
              "stats layout must match so BuildSWPipelineState can reinterpret the pointer");void BuildSWPipelineState(D3DSW_PIPELINE_STATE& in, SWPipelineState& out)
{
    out = {};

    // IA — input layout
    if (in.inputLayout)
    {
        const auto& elements = in.inputLayout->GetElements();
        out.numInputElements = std::min<Uint32>(static_cast<Uint32>(elements.size()), SW_MAX_INPUT_ELEMENTS);
        for (Uint32 i = 0; i < out.numInputElements; ++i)
        {
            const D3D11_INPUT_ELEMENT_DESC& d = elements[i];
            SWInputElement& e = out.inputElements[i];
            const char* name = d.SemanticName ? d.SemanticName : "";
            std::strncpy(e.semanticName, name, SW_SEMANTIC_NAME_MAX - 1);
            e.semanticName[SW_SEMANTIC_NAME_MAX - 1] = '\0';
            e.semanticIndex        = d.SemanticIndex;
            e.format               = d.Format;
            e.inputSlot            = d.InputSlot;
            e.alignedByteOffset    = d.AlignedByteOffset;
            e.inputSlotClass       = static_cast<SWInputSlotClass>(d.InputSlotClass);
            e.instanceDataStepRate = d.InstanceDataStepRate;
        }
    }

    // IA — index buffer
    if (in.indexBuffer)
    {
        out.indexBuffer.data   = static_cast<const Uint8*>(in.indexBuffer->GetDataPtr());
        out.indexBuffer.format = in.indexFormat;
        out.indexBuffer.offset = in.indexOffset;
    }

    // IA — vertex buffers
    for (Uint32 i = 0; i < SW_MAX_VERTEX_BUFFERS && i < D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT; ++i)
    {
        if (in.vertexBuffers[i])
        {
            out.vertexBuffers[i].data   = static_cast<const Uint8*>(in.vertexBuffers[i]->GetDataPtr());
            out.vertexBuffers[i].stride = in.vbStrides[i];
            out.vertexBuffers[i].offset = in.vbOffsets[i];
        }
    }

    out.topology = static_cast<SWTopology>(in.topology);

    // Shaders
    if (in.vs) { out.vs.fn = reinterpret_cast<void*>(in.vs->GetJitFn()); out.vs.reflection = &in.vs->GetReflection(); }
    if (in.ps) { out.ps.fn = reinterpret_cast<void*>(in.ps->GetJitFn()); out.ps.reflection = &in.ps->GetReflection(); }
    if (in.gs) { out.gs.fn = reinterpret_cast<void*>(in.gs->GetJitFn()); out.gs.reflection = &in.gs->GetReflection(); }
    if (in.hs) { out.hs.fn = reinterpret_cast<void*>(in.hs->GetCPFn()); out.hs.reflection = &in.hs->GetReflection();
        Uint32 forkCount = in.hs->GetForkPhaseCount();
        for (Uint32 i = 0; i < forkCount && i < SW_MAX_HS_FORK_PHASES; ++i) { out.hsForkFns[i] = in.hs->GetForkFn(i); }
        Uint32 joinCount = in.hs->GetJoinPhaseCount();
        for (Uint32 i = 0; i < joinCount && i < SW_MAX_HS_JOIN_PHASES; ++i) { out.hsJoinFns[i] = in.hs->GetJoinFn(i); }
    }
    if (in.ds) { out.ds.fn = reinterpret_cast<void*>(in.ds->GetJitFn()); out.ds.reflection = &in.ds->GetReflection(); }
    if (in.cs) { out.cs.fn = reinterpret_cast<void*>(in.cs->GetJitFn()); out.cs.reflection = &in.cs->GetReflection(); }

    // Per-stage resources (CB/SRV/sampler)
    BuildStageResources(out.vsRes, in.vsCBs, in.vsCBOffsets, in.vsSRVs, in.vsSamplers);
    BuildStageResources(out.psRes, in.psCBs, in.psCBOffsets, in.psSRVs, in.psSamplers);
    BuildStageResources(out.gsRes, in.gsCBs, in.gsCBOffsets, in.gsSRVs, in.gsSamplers);
    BuildStageResources(out.hsRes, in.hsCBs, in.hsCBOffsets, in.hsSRVs, in.hsSamplers);
    BuildStageResources(out.dsRes, in.dsCBs, in.dsCBOffsets, in.dsSRVs, in.dsSamplers);
    BuildStageResources(out.csRes, in.csCBs, in.csCBOffsets, in.csSRVs, in.csSamplers);

    // UAVs
    BuildStageUAVs(out.psRes, in.psUAVs);
    BuildStageUAVs(out.csRes, in.csUAVs);

    // GS stream output
    if (in.gs && in.gs->HasSO())
    {
        out.gsHasSO             = true;
        out.gsRasterizedStream  = in.gs->GetRasterizedStream();
        const auto& soDecls     = in.gs->GetSODecls();
        out.gsStreamOutDeclCount = std::min<Uint32>(static_cast<Uint32>(soDecls.size()), SW_MAX_SO_DECLARATIONS);
        for (Uint32 i = 0; i < out.gsStreamOutDeclCount; ++i)
        {
            const SODecl& sd = soDecls[i];
            SWStreamOutDeclaration& d = out.gsStreamOutDecls[i];
            std::memcpy(d.semanticName, sd.semanticName, SW_SEMANTIC_NAME_MAX);
            d.semanticIndex  = sd.semanticIndex;
            d.stream         = sd.stream;
            d.startComponent = sd.startComponent;
            d.componentCount = sd.componentCount;
            d.outputSlot     = sd.outputSlot;
        }
        const Uint32* strides = in.gs->GetSOStrides();
        for (Uint i = 0; i < SW_MAX_SO_TARGETS; ++i)
        {
            out.gsStreamOutStrides[i] = strides[i];
        }
    }

    // RS
    D3D11_RASTERIZER_DESC rsDesc{};
    rsDesc.FillMode               = D3D11_FILL_SOLID;
    rsDesc.CullMode               = D3D11_CULL_BACK;
    rsDesc.FrontCounterClockwise  = FALSE;
    rsDesc.DepthClipEnable        = TRUE;
    if (in.rsState)
    {
        in.rsState->GetDesc(&rsDesc);
    }
    out.rasterizerDesc.fillMode              = static_cast<SWFillMode>(rsDesc.FillMode);
    out.rasterizerDesc.cullMode              = static_cast<SWCullMode>(rsDesc.CullMode);
    out.rasterizerDesc.frontCCW              = rsDesc.FrontCounterClockwise ? true : false;
    out.rasterizerDesc.depthBias             = rsDesc.DepthBias;
    out.rasterizerDesc.depthBiasClamp        = rsDesc.DepthBiasClamp;
    out.rasterizerDesc.slopeScaledDepthBias  = rsDesc.SlopeScaledDepthBias;
    out.rasterizerDesc.depthClipEnable       = rsDesc.DepthClipEnable ? true : false;
    out.rasterizerDesc.scissorEnable         = rsDesc.ScissorEnable ? true : false;
    out.rasterizerDesc.multisampleEnable     = rsDesc.MultisampleEnable ? true : false;
    out.rasterizerDesc.antialiasedLineEnable = rsDesc.AntialiasedLineEnable ? true : false;

    // Viewports
    out.numViewports = std::min<Uint32>(in.numViewports, SW_MAX_VIEWPORTS);
    for (Uint32 i = 0; i < out.numViewports; ++i)
    {
        out.viewports[i].topLeftX = in.viewports[i].TopLeftX;
        out.viewports[i].topLeftY = in.viewports[i].TopLeftY;
        out.viewports[i].width    = in.viewports[i].Width;
        out.viewports[i].height   = in.viewports[i].Height;
        out.viewports[i].minDepth = in.viewports[i].MinDepth;
        out.viewports[i].maxDepth = in.viewports[i].MaxDepth;
    }

    // Scissor rects
    out.numScissorRects = std::min<Uint32>(in.numScissorRects, SW_MAX_SCISSOR_RECTS);
    for (Uint32 i = 0; i < out.numScissorRects; ++i)
    {
        out.scissorRects[i].left   = in.scissorRects[i].left;
        out.scissorRects[i].top    = in.scissorRects[i].top;
        out.scissorRects[i].right  = in.scissorRects[i].right;
        out.scissorRects[i].bottom = in.scissorRects[i].bottom;
    }

    // OM — render targets
    out.numRenderTargets = std::min<Uint32>(in.numRenderTargets, SW_MAX_RENDER_TARGETS);
    for (Uint32 i = 0; i < out.numRenderTargets; ++i)
    {
        D3D11RenderTargetViewSW* rtv = in.renderTargets[i];
        if (!rtv) { continue; }
        SWRenderTargetSnapshot& s = out.renderTargets[i];
        D3DSW_SUBRESOURCE_LAYOUT layout = rtv->GetLayout();
        s.data          = rtv->GetDataPtr();
        s.format        = rtv->GetFormat();
        s.rowPitch      = layout.RowPitch;
        s.pixStride     = layout.PixelStride;
        s.slicePitch    = layout.DepthPitch;
        s.sampleCount   = rtv->GetSampleCount();
        s.sampleQuality = rtv->GetSampleQuality();
        s.arraySize     = 1;
    }

    // OM — depth stencil
    if (in.depthStencilView)
    {
        D3D11DepthStencilViewSW* dsv = in.depthStencilView;
        D3DSW_SUBRESOURCE_LAYOUT layout = dsv->GetLayout();
        out.depthStencilView.data          = dsv->GetDataPtr();
        out.depthStencilView.format        = dsv->GetFormat();
        out.depthStencilView.rowPitch      = layout.RowPitch;
        out.depthStencilView.numRows       = layout.NumRows;
        out.depthStencilView.pixStride     = layout.PixelStride;
        out.depthStencilView.sampleCount   = dsv->GetSampleCount();
        out.depthStencilView.sampleQuality = dsv->GetSampleQuality();
    }

    // OM — blend
    for (Uint i = 0; i < 8; ++i)
    {
        out.blendDesc.renderTarget[i].renderTargetWriteMask = 0x0F;
    }
    if (in.blendState)
    {
        D3D11_BLEND_DESC1 bsDesc{};
        in.blendState->GetDesc1(&bsDesc);
        out.blendDesc.alphaToCoverage  = bsDesc.AlphaToCoverageEnable ? true : false;
        out.blendDesc.independentBlend = bsDesc.IndependentBlendEnable ? true : false;
        for (Uint i = 0; i < 8; ++i)
        {
            const D3D11_RENDER_TARGET_BLEND_DESC1& d = bsDesc.RenderTarget[i];
            SWRenderTargetBlendDesc& r = out.blendDesc.renderTarget[i];
            r.blendEnable           = d.BlendEnable ? true : false;
            r.logicOpEnable         = d.LogicOpEnable ? true : false;
            r.srcBlend              = static_cast<SWBlend>(d.SrcBlend);
            r.destBlend             = static_cast<SWBlend>(d.DestBlend);
            r.blendOp               = static_cast<SWBlendOp>(d.BlendOp);
            r.srcBlendAlpha         = static_cast<SWBlend>(d.SrcBlendAlpha);
            r.destBlendAlpha        = static_cast<SWBlend>(d.DestBlendAlpha);
            r.blendOpAlpha          = static_cast<SWBlendOp>(d.BlendOpAlpha);
            r.logicOp               = static_cast<SWLogicOp>(d.LogicOp);
            r.renderTargetWriteMask = d.RenderTargetWriteMask;
        }
    }
    for (Uint i = 0; i < 4; ++i)
    {
        out.blendFactor[i] = in.blendFactor[i];
    }
    out.sampleMask = in.sampleMask;

    // OM — depth-stencil
    out.depthStencilDesc.depthEnable     = true;
    out.depthStencilDesc.depthWriteMask  = SWDepthWriteMask::All;
    out.depthStencilDesc.depthFunc       = SWComparison::Less;
    out.depthStencilDesc.stencilEnable   = false;
    out.depthStencilDesc.stencilReadMask = 0xFF;
    out.depthStencilDesc.stencilWriteMask = 0xFF;
    if (in.depthStencilState)
    {
        D3D11_DEPTH_STENCIL_DESC dsDesc{};
        in.depthStencilState->GetDesc(&dsDesc);
        out.depthStencilDesc.depthEnable     = dsDesc.DepthEnable ? true : false;
        out.depthStencilDesc.depthWriteMask  = static_cast<SWDepthWriteMask>(dsDesc.DepthWriteMask);
        out.depthStencilDesc.depthFunc       = static_cast<SWComparison>(dsDesc.DepthFunc);
        out.depthStencilDesc.stencilEnable   = dsDesc.StencilEnable ? true : false;
        out.depthStencilDesc.stencilReadMask = dsDesc.StencilReadMask;
        out.depthStencilDesc.stencilWriteMask = dsDesc.StencilWriteMask;
        out.depthStencilDesc.frontFace.failOp      = static_cast<SWStencilOp>(dsDesc.FrontFace.StencilFailOp);
        out.depthStencilDesc.frontFace.depthFailOp = static_cast<SWStencilOp>(dsDesc.FrontFace.StencilDepthFailOp);
        out.depthStencilDesc.frontFace.passOp      = static_cast<SWStencilOp>(dsDesc.FrontFace.StencilPassOp);
        out.depthStencilDesc.frontFace.func        = static_cast<SWComparison>(dsDesc.FrontFace.StencilFunc);
        out.depthStencilDesc.backFace.failOp       = static_cast<SWStencilOp>(dsDesc.BackFace.StencilFailOp);
        out.depthStencilDesc.backFace.depthFailOp  = static_cast<SWStencilOp>(dsDesc.BackFace.StencilDepthFailOp);
        out.depthStencilDesc.backFace.passOp       = static_cast<SWStencilOp>(dsDesc.BackFace.StencilPassOp);
        out.depthStencilDesc.backFace.func         = static_cast<SWComparison>(dsDesc.BackFace.StencilFunc);
    }
    out.stencilRef = static_cast<Uint8>(in.stencilRef);

    // SO
    for (Uint32 i = 0; i < SW_MAX_SO_TARGETS; ++i)
    {
        if (!in.soTargets[i].buffer) { continue; }
        out.soTargets[i].data        = static_cast<Uint8*>(in.soTargets[i].buffer->GetDataPtr());
        D3D11_BUFFER_DESC bdesc{};
        in.soTargets[i].buffer->GetDesc(&bdesc);
        out.soTargets[i].sizeBytes   = bdesc.ByteWidth;
        out.soTargets[i].writeOffset = in.soTargets[i].writeOffset;
        out.soTargets[i].vertexCount = in.soTargets[i].vertexCount;
    }

    out.stats = &in.stats;
}


} 
