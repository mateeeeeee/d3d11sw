#include "pipeline_state.h"
#include "misc/input_layout.h"
#include "shaders/vertex_shader.h"
#include "shaders/pixel_shader.h"
#include "shaders/geometry_shader.h"
#include "shaders/hull_shader.h"
#include "shaders/domain_shader.h"
#include "shaders/compute_shader.h"
#include "states/blend_state.h"
#include "states/depth_stencil_state.h"
#include "states/rasterizer_state.h"
#include "states/sampler_state.h"
#include "views/render_target_view.h"
#include "views/depth_stencil_view.h"
#include "views/shader_resource_view.h"
#include "views/unordered_access_view.h"
#include "resources/buffer.h"

namespace d3d11sw {


void D3D11SW_PIPELINE_STATE::ReleaseAll()
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

    relA(vsSamplers); relA(psSamplers); relA(gsSamplers);
    relA(hsSamplers); relA(dsSamplers); relA(csSamplers);

    rel(rsState);
    relA(renderTargets);
    rel(depthStencilView);
    rel(blendState);
    rel(depthStencilState);
}

void BuildStageResources(
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


} 
