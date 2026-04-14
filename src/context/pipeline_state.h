#pragma once

#include "common/common.h"
#include "shaders/shader_abi.h"

namespace d3d11sw {

class D3D11InputLayoutSW;
class D3D11BufferSW;
class D3D11VertexShaderSW;
class D3D11PixelShaderSW;
class D3D11GeometryShaderSW;
class D3D11HullShaderSW;
class D3D11DomainShaderSW;
class D3D11ComputeShaderSW;
class D3D11ShaderResourceViewSW;
class D3D11UnorderedAccessViewSW;
class D3D11SamplerStateSW;
class D3D11RasterizerStateSW;
class D3D11RenderTargetViewSW;
class D3D11DepthStencilViewSW;
class D3D11BlendStateSW;
class D3D11DepthStencilStateSW;


struct D3D11SW_PIPELINE_STATE
{
    //IA
    D3D11InputLayoutSW*  inputLayout;
    D3D11BufferSW*       indexBuffer;
    DXGI_FORMAT          indexFormat;
    Uint                 indexOffset;
    D3D11BufferSW*       vertexBuffers[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    Uint                 vbStrides[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    Uint                 vbOffsets[D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];
    D3D11_PRIMITIVE_TOPOLOGY topology;

    //Shaders
    D3D11VertexShaderSW*   vs;
    D3D11PixelShaderSW*    ps;
    D3D11GeometryShaderSW* gs;
    D3D11HullShaderSW*     hs;
    D3D11DomainShaderSW*   ds;
    D3D11ComputeShaderSW*  cs;

    //Constant buffers
    D3D11BufferSW* vsCBs[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    D3D11BufferSW* psCBs[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    D3D11BufferSW* gsCBs[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    D3D11BufferSW* hsCBs[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    D3D11BufferSW* dsCBs[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    D3D11BufferSW* csCBs[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    Uint vsCBOffsets[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    Uint psCBOffsets[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    Uint gsCBOffsets[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    Uint hsCBOffsets[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    Uint dsCBOffsets[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];
    Uint csCBOffsets[D3D11_COMMONSHADER_CONSTANT_BUFFER_API_SLOT_COUNT];

    //SRVs
    D3D11ShaderResourceViewSW* vsSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    D3D11ShaderResourceViewSW* psSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    D3D11ShaderResourceViewSW* gsSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    D3D11ShaderResourceViewSW* hsSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    D3D11ShaderResourceViewSW* dsSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];
    D3D11ShaderResourceViewSW* csSRVs[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT];

    //UAVs
    D3D11UnorderedAccessViewSW* csUAVs[D3D11_1_UAV_SLOT_COUNT];
    D3D11UnorderedAccessViewSW* psUAVs[D3D11_1_UAV_SLOT_COUNT];

    //Samplers
    D3D11SamplerStateSW* vsSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    D3D11SamplerStateSW* psSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    D3D11SamplerStateSW* gsSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    D3D11SamplerStateSW* hsSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    D3D11SamplerStateSW* dsSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];
    D3D11SamplerStateSW* csSamplers[D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT];

    //RS
    D3D11RasterizerStateSW* rsState;
    D3D11_VIEWPORT          viewports[D3D11_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1];
    Uint                    numViewports;
    D3D11_RECT              scissorRects[D3D11_VIEWPORT_AND_SCISSORRECT_MAX_INDEX + 1];
    Uint                    numScissorRects;

    //OM
    D3D11RenderTargetViewSW*  renderTargets[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
    Uint                      numRenderTargets;
    D3D11DepthStencilViewSW*  depthStencilView;
    D3D11BlendStateSW*        blendState;
    Float                     blendFactor[4];
    Uint                      sampleMask;
    D3D11DepthStencilStateSW* depthStencilState;
    Uint                      stencilRef;

    //SO
    struct SOTarget
    {
        D3D11BufferSW* buffer      = nullptr;
        Uint32         writeOffset = 0;
        Uint32         vertexCount = 0;
    };
    SOTarget soTargets[D3D11_SO_BUFFER_SLOT_COUNT];

    void ReleaseAll();
};

void BuildStageResources(SW_Resources& res,
                         D3D11BufferSW* const* cbs,
                         const Uint* cbOffsets,
                         D3D11ShaderResourceViewSW* const* srvs,
                         D3D11SamplerStateSW* const* samplers);

} 
