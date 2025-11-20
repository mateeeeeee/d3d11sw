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

    relA(vsSamplers); relA(psSamplers); relA(gsSamplers);
    relA(hsSamplers); relA(dsSamplers); relA(csSamplers);

    rel(rsState);
    relA(renderTargets);
    rel(depthStencilView);
    rel(blendState);
    rel(depthStencilState);
}


} 
