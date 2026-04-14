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

void BuildStageResources(
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

        D3D11SW_SUBRESOURCE_LAYOUT layout = srv->GetLayout();
        SW_SRV& tex = res.srv[i];
        tex.data        = srv->GetDataPtr();
        tex.format      = srv->GetFormat();
        tex.mipLevels   = srv->GetViewMipCount();
        tex.stride      = srv->GetStride();

        for (Uint m = 0; m < tex.mipLevels && m < SW_MAX_MIP_LEVELS; ++m)
        {
            D3D11SW_SUBRESOURCE_LAYOUT ml = srv->GetMipLayout(m);
            tex.mips[m].width     = ml.PixelStride > 0 ? (ml.RowPitch / ml.PixelStride) * ml.BlockSize : 0;
            tex.mips[m].height    = ml.NumRows * ml.BlockSize;
            tex.mips[m].depth     = ml.NumSlices;
            tex.mips[m].rowPitch  = ml.RowPitch;
            tex.mips[m].slicePitch = ml.DepthPitch;
            D3D11SW_SUBRESOURCE_LAYOUT m0 = srv->GetMipLayout(0);
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


} 
