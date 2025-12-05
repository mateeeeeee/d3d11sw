#include "context/vertex_pipeline.h"
#include "misc/input_layout.h"
#include "shaders/vertex_shader.h"
#include "views/shader_resource_view.h"
#include "states/sampler_state.h"
#include "resources/buffer.h"
#include "util/env.h"
#include <cstring>

namespace d3d11sw {

static void UnpackVertexElement(DXGI_FORMAT fmt, const UINT8* src, SW_float4& out)
{
    out = {0.f, 0.f, 0.f, 0.f};
    switch (fmt)
    {
        case DXGI_FORMAT_R32G32B32A32_FLOAT:
            std::memcpy(&out, src, 16);
            break;
        case DXGI_FORMAT_R32G32B32_FLOAT:
            std::memcpy(&out, src, 12);
            out.w = 0.f;
            break;
        case DXGI_FORMAT_R32G32_FLOAT:
            std::memcpy(&out, src, 8);
            break;
        case DXGI_FORMAT_R32_FLOAT:
            std::memcpy(&out.x, src, 4);
            break;
        default:
            break;
    }
}

VertexPipeline::VertexPipeline(const D3D11SW_PIPELINE_STATE& state)
    : _vsFn(state.vs ? state.vs->GetJitFn() : nullptr)
    , _vsReflection(state.vs ? &state.vs->GetReflection() : nullptr)
    , _vsRes{}
    , _state(&state)
    , _cacheEnabled(GetEnvBool("D3D11SW_VERTEX_CACHE", false))
{
    if (state.vs)
    {
        BuildStageResources(_vsRes, state.vsCBs, state.vsSRVs, state.vsSamplers);
    }
}

SW_VSOutput VertexPipeline::RunVS(UINT vertIdx)
{
    if (_cacheEnabled)
    {
        auto it = _cache.find(vertIdx);
        if (it != _cache.end()) { return it->second; }
    }

    SW_VSInput vsIn{};
    FetchVertex(vsIn, vertIdx);

    SW_VSOutput vsOut{};
    _vsFn(&vsIn, &vsOut, &_vsRes);

    if (_cacheEnabled)
    {
        _cache[vertIdx] = vsOut;
    }

    return vsOut;
}

void VertexPipeline::FetchVertex(SW_VSInput& vsIn, UINT vertexIndex)
{
    if (!_state->inputLayout)
    {
        return;
    }

    const auto& elements = _state->inputLayout->GetElements();
    for (const auto& elem : elements)
    {
        UINT slot = elem.InputSlot;
        D3D11BufferSW* vb = _state->vertexBuffers[slot];
        if (!vb)
        {
            continue;
        }

        UINT stride = _state->vbStrides[slot];
        UINT offset = _state->vbOffsets[slot] + elem.AlignedByteOffset + vertexIndex * stride;
        const UINT8* src = static_cast<const UINT8*>(vb->GetDataPtr()) + offset;

        SW_float4 value{};
        UnpackVertexElement(elem.Format, src, value);
        for (const auto& inp : _vsReflection->inputs)
        {
            if (inp.semanticIndex == elem.SemanticIndex &&
                std::strcmp(inp.name, elem.SemanticName) == 0)
            {
                vsIn.v[inp.reg] = value;
                break;
            }
        }
    }
}

UINT VertexPipeline::FetchIndex(UINT location) const
{
    if (!_state->indexBuffer)
    {
        return 0;
    }

    const UINT8* base = static_cast<const UINT8*>(_state->indexBuffer->GetDataPtr());
    if (_state->indexFormat == DXGI_FORMAT_R16_UINT)
    {
        UINT16 idx;
        std::memcpy(&idx, base + _state->indexOffset + location * 2, 2);
        return idx;
    }
    else
    {
        UINT32 idx;
        std::memcpy(&idx, base + _state->indexOffset + location * 4, 4);
        return idx;
    }
}

}
