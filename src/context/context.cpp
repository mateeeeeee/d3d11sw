#include "context.h"
#include "common/log.h"
#include "context/depth_stencil_util.h"
#include "resources/subresource_layout.h"
#include "resources/buffer.h"
#include "resources/texture1d.h"
#include "resources/texture2d.h"
#include "resources/texture3d.h"
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
#include "views/view_util.h"
#include "util/format.h"
#include "context/context_util.h"
#include "context/dispatcher.h"
#include "misc/query.h"

namespace d3d11sw {


template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11DeviceContext4))
    {
        *ppv = static_cast<ID3D11DeviceContext4*>(this);
    }
    else if (riid == __uuidof(ID3D11DeviceContext3))
    {
        *ppv = static_cast<ID3D11DeviceContext3*>(this);
    }
    else if (riid == __uuidof(ID3D11DeviceContext2))
    {
        *ppv = static_cast<ID3D11DeviceContext2*>(this);
    }
    else if (riid == __uuidof(ID3D11DeviceContext1))
    {
        *ppv = static_cast<ID3D11DeviceContext1*>(this);
    }
    else if (riid == __uuidof(ID3D11DeviceContext))
    {
        *ppv = static_cast<ID3D11DeviceContext*>(this);
    }
    else if (riid == __uuidof(ID3D11DeviceChild))
    {
        *ppv = static_cast<ID3D11DeviceChild*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

template<Bool IsDebug>
D3D11DeviceContextSWImpl<IsDebug>::D3D11DeviceContextSWImpl(ID3D11Device* device)
    : DeviceChildImpl(device)
{
    _state.blendFactor[0] = _state.blendFactor[1] =
    _state.blendFactor[2] = _state.blendFactor[3] = 1.0f;
    _state.sampleMask = 0xFFFFFFFF;
}

template<Bool IsDebug>
D3D11DeviceContextSWImpl<IsDebug>::~D3D11DeviceContextSWImpl()
{
    _state.ReleaseAll();
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::VSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers)
{
    SetSlots(_state.vsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
    for (Uint i = 0; i < NumBuffers; ++i)
    {
        _state.vsCBOffsets[StartSlot + i] = 0;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::PSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView*const* ppShaderResourceViews)
{
    SetSlots(_state.psSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW*const*>(ppShaderResourceViews));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::PSSetShader(ID3D11PixelShader* pPixelShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
{
    SetSlot(_state.ps, static_cast<D3D11PixelShaderSW*>(pPixelShader));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::PSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState*const* ppSamplers)
{
    SetSlots(_state.psSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW*const*>(ppSamplers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::VSSetShader(ID3D11VertexShader* pVertexShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
{
    SetSlot(_state.vs, static_cast<D3D11VertexShaderSW*>(pVertexShader));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::PSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers)
{
    SetSlots(_state.psCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
    for (Uint i = 0; i < NumBuffers; ++i)
    {
        _state.psCBOffsets[StartSlot + i] = 0;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::IASetInputLayout(ID3D11InputLayout* pInputLayout)
{
    SetSlot(_state.inputLayout, static_cast<D3D11InputLayoutSW*>(pInputLayout));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::IASetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppVertexBuffers, const UINT* pStrides, const UINT* pOffsets)
{
    for (Uint i = 0; i < NumBuffers; i++)
    {
        SetSlot(_state.vertexBuffers[StartSlot + i], ppVertexBuffers ? static_cast<D3D11BufferSW*>(ppVertexBuffers[i]) : nullptr);
        _state.vbStrides[StartSlot + i] = ppVertexBuffers && pStrides ? pStrides[i] : 0;
        _state.vbOffsets[StartSlot + i] = ppVertexBuffers && pOffsets ? pOffsets[i] : 0;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::IASetIndexBuffer(ID3D11Buffer* pIndexBuffer, DXGI_FORMAT Format, UINT Offset)
{
    SetSlot(_state.indexBuffer, static_cast<D3D11BufferSW*>(pIndexBuffer));
    _state.indexFormat = Format;
    _state.indexOffset = Offset;
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY Topology)
{
    _state.topology = Topology;
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::GSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers)
{
    SetSlots(_state.gsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
    for (Uint i = 0; i < NumBuffers; ++i)
    {
        _state.gsCBOffsets[StartSlot + i] = 0;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::GSSetShader(ID3D11GeometryShader* pShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
{
    SetSlot(_state.gs, static_cast<D3D11GeometryShaderSW*>(pShader));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::VSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView*const* ppShaderResourceViews)
{
    SetSlots(_state.vsSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW*const*>(ppShaderResourceViews));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::VSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState*const* ppSamplers)
{
    SetSlots(_state.vsSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW*const*>(ppSamplers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::SetPredication(ID3D11Predicate* pPredicate, BOOL PredicateValue)
{
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::GSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView*const* ppShaderResourceViews)
{
    SetSlots(_state.gsSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW*const*>(ppShaderResourceViews));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::GSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState*const* ppSamplers)
{
    SetSlots(_state.gsSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW*const*>(ppSamplers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::OMSetRenderTargets(UINT NumViews, ID3D11RenderTargetView*const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView)
{
    if (NumViews > D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT)
    {
        DebugMsg("OMSetRenderTargets: NumViews ({}) exceeds D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT ({})", NumViews, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT);
        return;
    }
    for (Uint i = NumViews; i < _state.numRenderTargets; i++)
    {
        SetSlot(_state.renderTargets[i], static_cast<D3D11RenderTargetViewSW*>(nullptr));
    }
    _state.numRenderTargets = NumViews;
    for (Uint i = 0; i < NumViews; i++)
    {
        SetSlot(_state.renderTargets[i], ppRenderTargetViews ? static_cast<D3D11RenderTargetViewSW*>(ppRenderTargetViews[i]) : nullptr);
    }
    SetSlot(_state.depthStencilView, static_cast<D3D11DepthStencilViewSW*>(pDepthStencilView));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::OMSetRenderTargetsAndUnorderedAccessViews(UINT NumRTVs, ID3D11RenderTargetView*const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView*const* ppUnorderedAccessViews, const UINT* pUAVInitialCounts)
{
    if (NumRTVs != D3D11_KEEP_RENDER_TARGETS_AND_DEPTH_STENCIL)
    {
        for (Uint i = NumRTVs; i < _state.numRenderTargets; i++)
        {
            SetSlot(_state.renderTargets[i], static_cast<D3D11RenderTargetViewSW*>(nullptr));
        }
        _state.numRenderTargets = NumRTVs;
        for (Uint i = 0; i < NumRTVs; i++)
        {
            SetSlot(_state.renderTargets[i], ppRenderTargetViews ? static_cast<D3D11RenderTargetViewSW*>(ppRenderTargetViews[i]) : nullptr);
        }
        SetSlot(_state.depthStencilView, static_cast<D3D11DepthStencilViewSW*>(pDepthStencilView));
    }

    if (NumUAVs != D3D11_KEEP_UNORDERED_ACCESS_VIEWS)
    {
        for (Uint i = 0; i < NumUAVs; i++)
        {
            Uint slot = UAVStartSlot + i;
            if (slot < D3D11_1_UAV_SLOT_COUNT)
            {
                SetSlot(_state.psUAVs[slot], ppUnorderedAccessViews ? static_cast<D3D11UnorderedAccessViewSW*>(ppUnorderedAccessViews[i]) : nullptr);
            }
        }
        if (pUAVInitialCounts)
        {
            for (Uint i = 0; i < NumUAVs; ++i)
            {
                if (pUAVInitialCounts[i] != 0xFFFFFFFF)
                {
                    Uint slot = UAVStartSlot + i;
                    if (slot < D3D11_1_UAV_SLOT_COUNT && _state.psUAVs[slot])
                    {
                        *_state.psUAVs[slot]->GetCounter() = pUAVInitialCounts[i];
                    }
                }
            }
        }
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::OMSetBlendState(ID3D11BlendState* pBlendState, const FLOAT BlendFactor[4], UINT SampleMask)
{
    SetSlot(_state.blendState, static_cast<D3D11BlendStateSW*>(pBlendState));
    if (BlendFactor)
    {
        std::memcpy(_state.blendFactor, BlendFactor, sizeof(Float) * 4);
    }
    else
    {
        _state.blendFactor[0] = _state.blendFactor[1] =
        _state.blendFactor[2] = _state.blendFactor[3] = 1.0f;
    }
    _state.sampleMask = SampleMask;
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::OMSetDepthStencilState(ID3D11DepthStencilState* pDepthStencilState, UINT StencilRef)
{
    SetSlot(_state.depthStencilState, static_cast<D3D11DepthStencilStateSW*>(pDepthStencilState));
    _state.stencilRef = StencilRef;
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::SOSetTargets(UINT NumBuffers, ID3D11Buffer*const* ppSOTargets, const UINT* pOffsets)
{
    for (Uint i = 0; i < D3D11_SO_BUFFER_SLOT_COUNT; ++i)
    {
        auto& t = _state.soTargets[i];
        if (i < NumBuffers && ppSOTargets && ppSOTargets[i])
        {
            SetSlot(t.buffer, static_cast<D3D11BufferSW*>(ppSOTargets[i]));
            if (pOffsets && pOffsets[i] != (UINT)-1)
            {
                t.writeOffset = pOffsets[i];
                t.vertexCount = 0;
            }
        }
        else
        {
            SetSlot(t.buffer, static_cast<D3D11BufferSW*>(nullptr));
            t.writeOffset = 0;
        }
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::RSSetState(ID3D11RasterizerState* pRasterizerState)
{
    SetSlot(_state.rsState, static_cast<D3D11RasterizerStateSW*>(pRasterizerState));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::RSSetViewports(UINT NumViewports, const D3D11_VIEWPORT* pViewports)
{
    _state.numViewports = NumViewports;
    if (pViewports)
    {
        std::memcpy(_state.viewports, pViewports, NumViewports * sizeof(D3D11_VIEWPORT));
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::RSSetScissorRects(UINT NumRects, const D3D11_RECT* pRects)
{
    _state.numScissorRects = NumRects;
    if (pRects)
    {
        std::memcpy(_state.scissorRects, pRects, NumRects * sizeof(D3D11_RECT));
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::HSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView*const* ppShaderResourceViews)
{
    SetSlots(_state.hsSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW*const*>(ppShaderResourceViews));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::HSSetShader(ID3D11HullShader* pHullShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
{
    SetSlot(_state.hs, static_cast<D3D11HullShaderSW*>(pHullShader));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::HSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState*const* ppSamplers)
{
    SetSlots(_state.hsSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW*const*>(ppSamplers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::HSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers)
{
    SetSlots(_state.hsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
    for (Uint i = 0; i < NumBuffers; ++i)
    {
        _state.hsCBOffsets[StartSlot + i] = 0;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView*const* ppShaderResourceViews)
{
    SetSlots(_state.dsSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW*const*>(ppShaderResourceViews));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DSSetShader(ID3D11DomainShader* pDomainShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
{
    SetSlot(_state.ds, static_cast<D3D11DomainShaderSW*>(pDomainShader));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState*const* ppSamplers)
{
    SetSlots(_state.dsSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW*const*>(ppSamplers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers)
{
    SetSlots(_state.dsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
    for (Uint i = 0; i < NumBuffers; ++i)
    {
        _state.dsCBOffsets[StartSlot + i] = 0;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::CSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView*const* ppShaderResourceViews)
{
    SetSlots(_state.csSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW*const*>(ppShaderResourceViews));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::CSSetUnorderedAccessViews(UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView*const* ppUnorderedAccessViews, const UINT* pUAVInitialCounts)
{
    SetSlots(_state.csUAVs, StartSlot, NumUAVs, reinterpret_cast<D3D11UnorderedAccessViewSW*const*>(ppUnorderedAccessViews));
    if (pUAVInitialCounts)
    {
        for (Uint i = 0; i < NumUAVs; ++i)
        {
            if (pUAVInitialCounts[i] != 0xFFFFFFFF)
            {
                Uint slot = StartSlot + i;
                if (slot < D3D11_1_UAV_SLOT_COUNT && _state.csUAVs[slot])
                {
                    *_state.csUAVs[slot]->GetCounter() = pUAVInitialCounts[i];
                }
            }
        }
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::CSSetShader(ID3D11ComputeShader* pComputeShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
{
    SetSlot(_state.cs, static_cast<D3D11ComputeShaderSW*>(pComputeShader));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::CSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState*const* ppSamplers)
{
    SetSlots(_state.csSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW*const*>(ppSamplers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::CSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers)
{
    SetSlots(_state.csCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
    for (Uint i = 0; i < NumBuffers; ++i)
    {
        _state.csCBOffsets[StartSlot + i] = 0;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::SetResourceMinLOD(ID3D11Resource* pResource, FLOAT MinLOD)
{
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
    _rasterizer.DrawIndexed(IndexCount, StartIndexLocation, BaseVertexLocation, 1, 0, _state);
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::Draw(UINT VertexCount, UINT StartVertexLocation)
{
    _rasterizer.Draw(VertexCount, StartVertexLocation, 1, 0, _state);
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation)
{
    _rasterizer.DrawIndexed(IndexCountPerInstance, StartIndexLocation, BaseVertexLocation, InstanceCount, StartInstanceLocation, _state);
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation)
{
    _rasterizer.Draw(VertexCountPerInstance, StartVertexLocation, InstanceCount, StartInstanceLocation, _state);
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DrawAuto()
{
    Uint32 vertexCount = _state.soTargets[0].vertexCount;
    if (vertexCount > 0)
    {
        _rasterizer.Draw(vertexCount, 0, 1, 0, _state);
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DrawIndexedInstancedIndirect(ID3D11Buffer* pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
    D3D11BufferSW* buf = static_cast<D3D11BufferSW*>(pBufferForArgs);
    D3D11_DRAW_INDEXED_INSTANCED_INDIRECT_ARGS args{};
    std::memcpy(&args, static_cast<Uint8*>(buf->GetDataPtr()) + AlignedByteOffsetForArgs, sizeof(args));
    _rasterizer.DrawIndexed(args.IndexCountPerInstance, args.StartIndexLocation, args.BaseVertexLocation, args.InstanceCount, args.StartInstanceLocation, _state);
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DrawInstancedIndirect(ID3D11Buffer* pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
    D3D11BufferSW* buf = static_cast<D3D11BufferSW*>(pBufferForArgs);
    D3D11_DRAW_INSTANCED_INDIRECT_ARGS args{};
    std::memcpy(&args, static_cast<Uint8*>(buf->GetDataPtr()) + AlignedByteOffsetForArgs, sizeof(args));
    _rasterizer.Draw(args.VertexCountPerInstance, args.StartVertexLocation, args.InstanceCount, args.StartInstanceLocation, _state);
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
    _dispatcher.Dispatch(ThreadGroupCountX, ThreadGroupCountY, ThreadGroupCountZ, _state);
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DispatchIndirect(ID3D11Buffer* pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
    D3D11BufferSW* buf = static_cast<D3D11BufferSW*>(pBufferForArgs);
    Uint8* ptr = static_cast<Uint8*>(buf->GetDataPtr()) + AlignedByteOffsetForArgs;
    UINT counts[3] = {};
    std::memcpy(counts, ptr, sizeof(counts));
    _dispatcher.Dispatch(counts[0], counts[1], counts[2], _state);
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::Map(ID3D11Resource* pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource)
{
    if (!pResource)
    {
        DebugMsg("Map: pResource is null");
        return E_INVALIDARG;
    }

    D3D11_USAGE usage = GetSWResourceUsage(pResource);
    UINT cpuFlags = GetSWResourceCPUAccessFlags(pResource);

    if (usage == D3D11_USAGE_DEFAULT)
    {
        DebugMsg("Map: cannot map a resource with D3D11_USAGE_DEFAULT");
        return E_INVALIDARG;
    }

    if ((MapType == D3D11_MAP_READ || MapType == D3D11_MAP_READ_WRITE) && !(cpuFlags & D3D11_CPU_ACCESS_READ))
    {
        DebugMsg("Map: MAP_READ/READ_WRITE requires D3D11_CPU_ACCESS_READ");
        return E_INVALIDARG;
    }

    if ((MapType == D3D11_MAP_WRITE || MapType == D3D11_MAP_READ_WRITE ||
         MapType == D3D11_MAP_WRITE_DISCARD || MapType == D3D11_MAP_WRITE_NO_OVERWRITE) && !(cpuFlags & D3D11_CPU_ACCESS_WRITE))
    {
        DebugMsg("Map: write map types require D3D11_CPU_ACCESS_WRITE");
        return E_INVALIDARG;
    }

    if ((MapType == D3D11_MAP_WRITE_DISCARD || MapType == D3D11_MAP_WRITE_NO_OVERWRITE) && usage != D3D11_USAGE_DYNAMIC)
    {
        DebugMsg("Map: MAP_WRITE_DISCARD/NO_OVERWRITE requires D3D11_USAGE_DYNAMIC");
        return E_INVALIDARG;
    }

    if (!pMappedResource)
    {
        return S_OK;
    }

    HRESULT hr = E_FAIL;
    RunOnSWResource(pResource, [&](auto* res)
    {
        auto layout                 = res->GetSubresourceLayout(Subresource);
        pMappedResource->pData      = static_cast<Uint8*>(res->GetDataPtr()) + layout.Offset;
        pMappedResource->RowPitch   = layout.RowPitch;
        pMappedResource->DepthPitch = layout.DepthPitch;
        hr = S_OK;
    });
    return hr;
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::Unmap(ID3D11Resource* pResource, UINT Subresource)
{
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::CopySubresourceRegion(ID3D11Resource* pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource* pSrcResource, UINT SrcSubresource, const D3D11_BOX* pSrcBox)
{
    RunOnSWResource(pSrcResource, [&](auto* src) 
    {
        RunOnSWResource(pDstResource, [&](auto* dst) 
        {
            auto srcLayout = src->GetSubresourceLayout(SrcSubresource);
            auto dstLayout = dst->GetSubresourceLayout(DstSubresource);

            const Uint8* srcBase = static_cast<const Uint8*>(src->GetDataPtr()) + srcLayout.Offset;
            Uint8*       dstBase = static_cast<Uint8*>(dst->GetDataPtr())       + dstLayout.Offset;

            Uint bx0         = pSrcBox ? pSrcBox->left  / srcLayout.BlockSize : 0;
            Uint by0         = pSrcBox ? pSrcBox->top   / srcLayout.BlockSize : 0;
            Uint bz0         = pSrcBox ? pSrcBox->front : 0;
            Uint copyBlocksX = pSrcBox ? (pSrcBox->right  - pSrcBox->left)  / srcLayout.BlockSize : srcLayout.RowPitch / srcLayout.PixelStride;
            Uint copyBlocksY = pSrcBox ? (pSrcBox->bottom - pSrcBox->top)   / srcLayout.BlockSize : srcLayout.NumRows;
            Uint copySlices  = pSrcBox ? (pSrcBox->back   - pSrcBox->front)                       : srcLayout.NumSlices;
            Uint copyBytes   = copyBlocksX * srcLayout.PixelStride;

            Uint dbx = DstX / dstLayout.BlockSize;
            Uint dby = DstY / dstLayout.BlockSize;
            for (Uint z = 0; z < copySlices; ++z)
            {
                for (Uint y = 0; y < copyBlocksY; ++y)
                {
                    std::memcpy(
                    dstBase + (Uint64)(DstZ + bz0 + z) * dstLayout.DepthPitch + (Uint64)(dby + y) * dstLayout.RowPitch + dbx * dstLayout.PixelStride,
                    srcBase + (Uint64)(bz0  + z)       * srcLayout.DepthPitch + (Uint64)(by0 + y) * srcLayout.RowPitch + bx0 * srcLayout.PixelStride,
                    copyBytes);
                }
            }
        });
    });
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::CopyResource(ID3D11Resource* pDstResource, ID3D11Resource* pSrcResource)
{
    RunOnSWResource(pSrcResource, [&](auto* src) 
    {
        RunOnSWResource(pDstResource, [&](auto* dst) 
        {
            if (dst->GetDataSize() == src->GetDataSize())
            {
                std::memcpy(dst->GetDataPtr(), src->GetDataPtr(), src->GetDataSize());
            }
        });
    });
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::UpdateSubresource(ID3D11Resource* pDstResource, UINT DstSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch)
{
    if (!pSrcData)
    {
        DebugMsg("UpdateSubresource: pSrcData is null");
        return;
    }

    if (!pDstResource)
    {
        DebugMsg("UpdateSubresource: pDstResource is null");
        return;
    }

    D3D11_USAGE usage = GetSWResourceUsage(pDstResource);
    if (usage == D3D11_USAGE_IMMUTABLE)
    {
        DebugMsg("UpdateSubresource: cannot update a resource with D3D11_USAGE_IMMUTABLE");
        return;
    }

    RunOnSWResource(pDstResource, [&](auto* dst)
    {
        auto         layout  = dst->GetSubresourceLayout(DstSubresource);
        Uint8*       dstBase = static_cast<Uint8*>(dst->GetDataPtr()) + layout.Offset;
        const Uint8* src     = static_cast<const Uint8*>(pSrcData);
        CopySubresourceData(dstBase, layout.RowPitch, layout.DepthPitch,
                            src, SrcRowPitch, SrcDepthPitch,
                            layout, pDstBox);
    });
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::CopyStructureCount(ID3D11Buffer* pDstBuffer, UINT DstAlignedByteOffset, ID3D11UnorderedAccessView* pSrcView)
{
    if (!pDstBuffer || !pSrcView)
    {
        return;
    }
    auto* buf = static_cast<D3D11BufferSW*>(pDstBuffer);
    auto* uav = static_cast<D3D11UnorderedAccessViewSW*>(pSrcView);
    Uint count = *uav->GetCounter();
    std::memcpy(static_cast<Uint8*>(buf->GetDataPtr()) + DstAlignedByteOffset, &count, sizeof(Uint));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::ClearRenderTargetView(ID3D11RenderTargetView* pRenderTargetView, const FLOAT ColorRGBA[4])
{
    if (!pRenderTargetView)
    {
        return;
    }

    D3D11RenderTargetViewSW* rtv = static_cast<D3D11RenderTargetViewSW*>(pRenderTargetView);
    D3D11SW_SUBRESOURCE_LAYOUT layout = rtv->GetLayout();

    Uint8 pixel[16] = {};
    PackColor(rtv->GetFormat(), ColorRGBA, pixel);
    ForEachPixel(rtv->GetDataPtr(), layout, [&pixel, &layout](Uint8* px)
    {
        std::memcpy(px, pixel, layout.PixelStride);
    });
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::ClearUnorderedAccessViewUint(ID3D11UnorderedAccessView* pUnorderedAccessView, const UINT Values[4])
{
    if (!pUnorderedAccessView)
    {
        return;
    }

    D3D11UnorderedAccessViewSW* uav = static_cast<D3D11UnorderedAccessViewSW*>(pUnorderedAccessView);
    D3D11SW_SUBRESOURCE_LAYOUT layout = uav->GetLayout();

    Uint8 pixel[16] = {};
    PackUAVUint(uav->GetFormat(), Values, pixel);
    ForEachPixel(uav->GetDataPtr(), layout, [&pixel, &layout](Uint8* px)
    {
        std::memcpy(px, pixel, layout.PixelStride);
    });
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::ClearUnorderedAccessViewFloat(ID3D11UnorderedAccessView* pUnorderedAccessView, const FLOAT Values[4])
{
    if (!pUnorderedAccessView)
    {
        return;
    }

    D3D11UnorderedAccessViewSW* uav    = static_cast<D3D11UnorderedAccessViewSW*>(pUnorderedAccessView);
    D3D11SW_SUBRESOURCE_LAYOUT layout = uav->GetLayout();

    Uint8 pixel[16] = {};
    PackColor(uav->GetFormat(), Values, pixel);
    ForEachPixel(uav->GetDataPtr(), layout, [&pixel, &layout](Uint8* px)
    {
        std::memcpy(px, pixel, layout.PixelStride);
    });
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::ClearDepthStencilView(ID3D11DepthStencilView* pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
{
    if (!pDepthStencilView)
    {
        return;
    }

    D3D11DepthStencilViewSW* dsv = static_cast<D3D11DepthStencilViewSW*>(pDepthStencilView);
    Uint8* data = dsv->GetDataPtr();
    D3D11SW_SUBRESOURCE_LAYOUT layout = dsv->GetLayout();
    DXGI_FORMAT fmt = dsv->GetFormat();
    switch (fmt)
    {
        case DXGI_FORMAT_D32_FLOAT:
        {
            if (!(ClearFlags & D3D11_CLEAR_DEPTH))
            {
                break;
            }
            ForEachPixel(data, layout, [Depth](Uint8* px)
            {
                std::memcpy(px, &Depth, 4);
            });
            break;
        }
        case DXGI_FORMAT_D16_UNORM:
        {
            if (!(ClearFlags & D3D11_CLEAR_DEPTH))
            {
                break;
            }
            Uint16 d16 = (Uint16)(std::clamp(Depth, 0.f, 1.f) * 65535.f + 0.5f);
            ForEachPixel(data, layout, [d16](Uint8* px)
            {
                std::memcpy(px, &d16, 2);
            });
            break;
        }
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        {
            Bool clearDepth   = (ClearFlags & D3D11_CLEAR_DEPTH)   != 0;
            Bool clearStencil = (ClearFlags & D3D11_CLEAR_STENCIL) != 0;
            Uint32 d24 = std::min((Uint32)(std::clamp(Depth, 0.f, 1.f) * 16777215.f + 0.5f), 0xFFFFFFu);
            if (clearDepth && clearStencil)
            {
                Uint32 packed = ((Uint32)Stencil << 24) | (d24 & 0x00FFFFFFu);
                ForEachPixel(data, layout, [packed](Uint8* px)
                {
                    std::memcpy(px, &packed, 4);
                });
            }
            else if (clearDepth)
            {
                ForEachPixel(data, layout, [d24](Uint8* px)
                {
                        Uint32 v; std::memcpy(&v, px, 4);
                    v = (v & 0xFF000000u) | (d24 & 0x00FFFFFFu);
                    std::memcpy(px, &v, 4);
                });
            }
            else if (clearStencil)
            {
                ForEachPixel(data, layout, [Stencil](Uint8* px)
                {
                    Uint32 v; std::memcpy(&v, px, 4);
                    v = (v & 0x00FFFFFFu) | ((Uint32)Stencil << 24);
                    std::memcpy(px, &v, 4);
                });
            }
            break;
        }
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        {
            Bool clearDepth   = (ClearFlags & D3D11_CLEAR_DEPTH)   != 0;
            Bool clearStencil = (ClearFlags & D3D11_CLEAR_STENCIL) != 0;
            if (clearDepth)
            {
                ForEachPixel(data, layout, [Depth](Uint8* px)
                {
                    std::memcpy(px, &Depth, 4);
                });
            }
            if (clearStencil)
            {
                ForEachPixel(data, layout, [Stencil](Uint8* px)
                {
                    px[4] = Stencil;
                });
            }
            break;
        }
        default:
            break;
    }

    if (ClearFlags & D3D11_CLEAR_DEPTH)
    {
        Int width = static_cast<Int>(layout.RowPitch / DepthPixelStride(fmt));
        Int height = static_cast<Int>(layout.NumRows);
        _rasterizer.ClearHiZ(data, width, height, Depth);
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::GenerateMips(ID3D11ShaderResourceView* pShaderResourceView)
{
    if (!pShaderResourceView)
    {
        return;
    }

    auto* srv = static_cast<D3D11ShaderResourceViewSW*>(pShaderResourceView);
    ID3D11Resource* resource = nullptr;
    srv->GetResource(&resource);
    if (!resource)
    {
        return;
    }

    D3D11SW_RESOURCE_INFO info = GetSWResourceInfo(resource);
    DXGI_FORMAT fmt = srv->GetFormat();
    if (fmt == DXGI_FORMAT_UNKNOWN)
    {
        fmt = info.Format;
    }
    Uint pixStride = GetFormatStride(fmt);
    Uint arraySize = info.ArraySize;
    Bool is3D = (info.Dimension == D3D11_RESOURCE_DIMENSION_TEXTURE3D);

    for (Uint slice = 0; slice < arraySize; ++slice)
    {
        for (Uint mip = 1; mip < info.MipLevels; ++mip)
        {
            Uint srcSub = D3D11CalcSubresource(mip - 1, slice, info.MipLevels);
            Uint dstSub = D3D11CalcSubresource(mip, slice, info.MipLevels);

            D3D11SW_SUBRESOURCE_LAYOUT srcLayout = GetSwSubresourceLayout(resource, srcSub);
            D3D11SW_SUBRESOURCE_LAYOUT dstLayout = GetSwSubresourceLayout(resource, dstSub);

            Uint8* srcBase = static_cast<Uint8*>(GetSwDataPtr(resource, srcSub));
            Uint8* dstBase = static_cast<Uint8*>(GetSwDataPtr(resource, dstSub));

            Uint srcW = pixStride > 0 ? srcLayout.RowPitch / pixStride : 0;
            Uint srcH = srcLayout.NumRows;
            Uint srcD = srcLayout.NumSlices;
            Uint dstW = pixStride > 0 ? dstLayout.RowPitch / pixStride : 0;
            Uint dstH = dstLayout.NumRows;
            Uint dstD = dstLayout.NumSlices;

            for (Uint z = 0; z < dstD; ++z)
            {
                Uint sz0 = std::min(z * 2, srcD - 1);
                Uint sz1 = std::min(z * 2 + 1, srcD - 1);
                Uint numZ = (is3D && srcD > 1) ? 2u : 1u;

                for (Uint y = 0; y < dstH; ++y)
                {
                    Uint sy0 = std::min(y * 2, srcH - 1);
                    Uint sy1 = std::min(y * 2 + 1, srcH - 1);

                    for (Uint x = 0; x < dstW; ++x)
                    {
                        Uint sx0 = std::min(x * 2, srcW - 1);
                        Uint sx1 = std::min(x * 2 + 1, srcW - 1);

                        Float sum[4] = {0, 0, 0, 0};
                        Uint count = 0;

                        Uint zSlices[2] = {sz0, sz1};
                        for (Uint zi = 0; zi < numZ; ++zi)
                        {
                            Uint8* sliceBase = srcBase + (Uint64)zSlices[zi] * srcLayout.DepthPitch;
                            Float c[4];
                            UnpackColor(fmt, sliceBase + (Uint64)sy0 * srcLayout.RowPitch + (Uint64)sx0 * pixStride, c);
                            sum[0] += c[0]; sum[1] += c[1]; sum[2] += c[2]; sum[3] += c[3]; ++count;
                            UnpackColor(fmt, sliceBase + (Uint64)sy0 * srcLayout.RowPitch + (Uint64)sx1 * pixStride, c);
                            sum[0] += c[0]; sum[1] += c[1]; sum[2] += c[2]; sum[3] += c[3]; ++count;
                            UnpackColor(fmt, sliceBase + (Uint64)sy1 * srcLayout.RowPitch + (Uint64)sx0 * pixStride, c);
                            sum[0] += c[0]; sum[1] += c[1]; sum[2] += c[2]; sum[3] += c[3]; ++count;
                            UnpackColor(fmt, sliceBase + (Uint64)sy1 * srcLayout.RowPitch + (Uint64)sx1 * pixStride, c);
                            sum[0] += c[0]; sum[1] += c[1]; sum[2] += c[2]; sum[3] += c[3]; ++count;
                        }

                        Float inv = 1.f / (Float)count;
                        Float avg[4] = {sum[0] * inv, sum[1] * inv, sum[2] * inv, sum[3] * inv};
                        Uint8 packed[16];
                        PackColor(fmt, avg, packed);

                        Uint8* dst = dstBase + (Uint64)z * dstLayout.DepthPitch
                                             + (Uint64)y * dstLayout.RowPitch
                                             + (Uint64)x * pixStride;
                        std::memcpy(dst, packed, pixStride);
                    }
                }
            }
        }
    }

    resource->Release();
}

template<Bool IsDebug>
FLOAT STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::GetResourceMinLOD(ID3D11Resource* pResource)
{
    return 0.0f;
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::ResolveSubresource(ID3D11Resource* pDstResource, UINT DstSubresource, ID3D11Resource* pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format)
{
    //Atm, all MS Textures are forced to have 1 sample, 
    //this implementation should change once we have proper implementation
    CopySubresourceRegion(pDstResource, DstSubresource, 0, 0, 0, pSrcResource, SrcSubresource, nullptr);
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::Begin(ID3D11Asynchronous* pAsync)
{
    if (!pAsync)
    {
        return;
    }

    ID3D11Query* query = nullptr;
    if (SUCCEEDED(pAsync->QueryInterface(__uuidof(ID3D11Query), reinterpret_cast<void**>(&query))))
    {
        static_cast<D3D11QuerySW*>(query)->Begin(&_state.stats);
        query->Release();
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::End(ID3D11Asynchronous* pAsync)
{
    if (!pAsync)
    {
        return;
    }

    ID3D11Query* query = nullptr;
    if (SUCCEEDED(pAsync->QueryInterface(__uuidof(ID3D11Query), reinterpret_cast<void**>(&query))))
    {
        static_cast<D3D11QuerySW*>(query)->End(&_state.stats);
        query->Release();
    }
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::GetData(ID3D11Asynchronous* pAsync, void* pData, UINT DataSize, UINT GetDataFlags)
{
    if (!pAsync)
    {
        return E_INVALIDARG;
    }

    ID3D11Query* query = nullptr;
    if (SUCCEEDED(pAsync->QueryInterface(__uuidof(ID3D11Query), reinterpret_cast<void**>(&query))))
    {
        HRESULT hr = static_cast<D3D11QuerySW*>(query)->GetData(pData, DataSize);
        query->Release();
        return hr;
    }

    return E_INVALIDARG;
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::ExecuteCommandList(ID3D11CommandList* pCommandList, BOOL RestoreContextState)
{
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::ClearState()
{
    _state.ReleaseAll();
    _state = {};
    _state.blendFactor[0] = _state.blendFactor[1] =
    _state.blendFactor[2] = _state.blendFactor[3] = 1.0f;
    _state.sampleMask = 0xFFFFFFFF;
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::Flush()
{
}

template<Bool IsDebug>
D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::GetType()
{
    return D3D11_DEVICE_CONTEXT_IMMEDIATE;
}

template<Bool IsDebug>
UINT STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::GetContextFlags()
{
    return 0;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::FinishCommandList(BOOL RestoreDeferredContextState, ID3D11CommandList** ppCommandList)
{
    return DXGI_ERROR_INVALID_CALL;
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::VSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
    GetSlots(_state.vsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::PSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
    GetSlots(_state.psSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW**>(ppShaderResourceViews));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::PSGetShader(ID3D11PixelShader** ppPixelShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
{
    if (ppPixelShader)
    {
        *ppPixelShader = _state.ps;
        if (_state.ps)
        {
            _state.ps->AddRef();
        }
    }
    if (pNumClassInstances)
    {
        *pNumClassInstances = 0;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::PSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
    GetSlots(_state.psSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW**>(ppSamplers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::VSGetShader(ID3D11VertexShader** ppVertexShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
{
    if (ppVertexShader)
    {
        *ppVertexShader = _state.vs;
        if (_state.vs)
        {
            _state.vs->AddRef();
        }
    }
    if (pNumClassInstances)
    {
        *pNumClassInstances = 0;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::PSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
    GetSlots(_state.psCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::IAGetInputLayout(ID3D11InputLayout** ppInputLayout)
{
    if (ppInputLayout)
    {
        *ppInputLayout = _state.inputLayout;
        if (_state.inputLayout)
        {
            _state.inputLayout->AddRef();
        }
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::IAGetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppVertexBuffers, UINT* pStrides, UINT* pOffsets)
{
    for (Uint i = 0; i < NumBuffers; i++)
    {
        if (ppVertexBuffers)
        {
            ppVertexBuffers[i] = _state.vertexBuffers[StartSlot + i];
            if (ppVertexBuffers[i])
            {
                ppVertexBuffers[i]->AddRef();
            }
        }
        if (pStrides)
        {
            pStrides[i] = _state.vbStrides[StartSlot + i];
        }
        if (pOffsets)
        {
            pOffsets[i] = _state.vbOffsets[StartSlot + i];
        }
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::IAGetIndexBuffer(ID3D11Buffer** pIndexBuffer, DXGI_FORMAT* Format, UINT* Offset)
{
    if (pIndexBuffer)
    {
        *pIndexBuffer = _state.indexBuffer;
        if (_state.indexBuffer)
        {
            _state.indexBuffer->AddRef();
        }
    }
    if (Format)
    {
        *Format = _state.indexFormat;
    }
    if (Offset)
    {
        *Offset = _state.indexOffset;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::IAGetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY* pTopology)
{
    if (pTopology)
    {
        *pTopology = _state.topology;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::GSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
    GetSlots(_state.gsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::GSGetShader(ID3D11GeometryShader** ppGeometryShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
{
    if (ppGeometryShader)
    {
        *ppGeometryShader = _state.gs;
        if (_state.gs)
        {
            _state.gs->AddRef();
        }
    }
    if (pNumClassInstances)
    {
        *pNumClassInstances = 0;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::VSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
    GetSlots(_state.vsSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW**>(ppShaderResourceViews));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::VSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
    GetSlots(_state.vsSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW**>(ppSamplers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::GetPredication(ID3D11Predicate** ppPredicate, BOOL* pPredicateValue)
{
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::GSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
    GetSlots(_state.gsSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW**>(ppShaderResourceViews));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::GSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
    GetSlots(_state.gsSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW**>(ppSamplers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::OMGetRenderTargets(UINT NumViews, ID3D11RenderTargetView** ppRenderTargetViews, ID3D11DepthStencilView** ppDepthStencilView)
{
    if (ppRenderTargetViews)
    {
        for (Uint i = 0; i < NumViews; i++)
        {
            ppRenderTargetViews[i] = i < _state.numRenderTargets ? _state.renderTargets[i] : nullptr;
            if (ppRenderTargetViews[i])
            {
                ppRenderTargetViews[i]->AddRef();
            }
        }
    }
    if (ppDepthStencilView)
    {
        *ppDepthStencilView = _state.depthStencilView;
        if (_state.depthStencilView)
        {
            _state.depthStencilView->AddRef();
        }
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::OMGetRenderTargetsAndUnorderedAccessViews(UINT NumRTVs, ID3D11RenderTargetView** ppRenderTargetViews, ID3D11DepthStencilView** ppDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView** ppUnorderedAccessViews)
{
    if (ppRenderTargetViews)
    {
        for (Uint i = 0; i < NumRTVs; i++)
        {
            ppRenderTargetViews[i] = i < _state.numRenderTargets ? _state.renderTargets[i] : nullptr;
            if (ppRenderTargetViews[i])
            {
                ppRenderTargetViews[i]->AddRef();
            }
        }
    }
    if (ppDepthStencilView)
    {
        *ppDepthStencilView = _state.depthStencilView;
        if (_state.depthStencilView)
        {
            _state.depthStencilView->AddRef();
        }
    }
    if (ppUnorderedAccessViews)
    {
        for (Uint i = 0; i < NumUAVs; i++)
        {
            Uint slot = UAVStartSlot + i;
            ppUnorderedAccessViews[i] = slot < D3D11_1_UAV_SLOT_COUNT ? _state.psUAVs[slot] : nullptr;
            if (ppUnorderedAccessViews[i])
            {
                ppUnorderedAccessViews[i]->AddRef();
            }
        }
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::OMGetBlendState(ID3D11BlendState** ppBlendState, FLOAT BlendFactor[4], UINT* pSampleMask)
{
    if (ppBlendState)
    {
        *ppBlendState = _state.blendState;
        if (_state.blendState)
        {
            _state.blendState->AddRef();
        }
    }
    if (BlendFactor)
    {
        std::memcpy(BlendFactor, _state.blendFactor, sizeof(Float) * 4);
    }
    if (pSampleMask)
    {
        *pSampleMask = _state.sampleMask;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::OMGetDepthStencilState(ID3D11DepthStencilState** ppDepthStencilState, UINT* pStencilRef)
{
    if (ppDepthStencilState)
    {
        *ppDepthStencilState = _state.depthStencilState;
        if (_state.depthStencilState)
        {
            _state.depthStencilState->AddRef();
        }
    }
    if (pStencilRef)
    {
        *pStencilRef = _state.stencilRef;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::SOGetTargets(UINT NumBuffers, ID3D11Buffer** ppSOTargets)
{
    if (!ppSOTargets)
    {
        return;
    }

    for (Uint i = 0; i < NumBuffers; ++i)
    {
        ppSOTargets[i] = (i < D3D11_SO_BUFFER_SLOT_COUNT) ? _state.soTargets[i].buffer : nullptr;
        if (ppSOTargets[i])
        {
            ppSOTargets[i]->AddRef();
        }
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::RSGetState(ID3D11RasterizerState** ppRasterizerState)
{
    if (ppRasterizerState)
    {
        *ppRasterizerState = _state.rsState;
        if (_state.rsState)
        {
            _state.rsState->AddRef();
        }
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::RSGetViewports(UINT* pNumViewports, D3D11_VIEWPORT* pViewports)
{
    if (pNumViewports)
    {
        if (pViewports)
        {
            Uint copyCount = *pNumViewports < _state.numViewports ? *pNumViewports : _state.numViewports;
            std::memcpy(pViewports, _state.viewports, copyCount * sizeof(D3D11_VIEWPORT));
        }
        *pNumViewports = _state.numViewports;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::RSGetScissorRects(UINT* pNumRects, D3D11_RECT* pRects)
{
    if (pNumRects)
    {
        if (pRects)
        {
            Uint copyCount = *pNumRects < _state.numScissorRects ? *pNumRects : _state.numScissorRects;
            std::memcpy(pRects, _state.scissorRects, copyCount * sizeof(D3D11_RECT));
        }
        *pNumRects = _state.numScissorRects;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::HSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
    GetSlots(_state.hsSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW**>(ppShaderResourceViews));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::HSGetShader(ID3D11HullShader** ppHullShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
{
    if (ppHullShader)
    {
        *ppHullShader = _state.hs;
        if (_state.hs)
        {
            _state.hs->AddRef();
        }
    }
    if (pNumClassInstances)
    {
        *pNumClassInstances = 0;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::HSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
    GetSlots(_state.hsSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW**>(ppSamplers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::HSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
    GetSlots(_state.hsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
    GetSlots(_state.dsSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW**>(ppShaderResourceViews));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DSGetShader(ID3D11DomainShader** ppDomainShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
{
    if (ppDomainShader)
    {
        *ppDomainShader = _state.ds;
        if (_state.ds)
        {
            _state.ds->AddRef();
        }
    }
    if (pNumClassInstances)
    {
        *pNumClassInstances = 0;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
    GetSlots(_state.dsSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW**>(ppSamplers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
    GetSlots(_state.dsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::CSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
    GetSlots(_state.csSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW**>(ppShaderResourceViews));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::CSGetUnorderedAccessViews(UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView** ppUnorderedAccessViews)
{
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::CSGetShader(ID3D11ComputeShader** ppComputeShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
{
    if (ppComputeShader)
    {
        *ppComputeShader = _state.cs;
        if (_state.cs)
        {
            _state.cs->AddRef();
        }
    }
    if (pNumClassInstances)
    {
        *pNumClassInstances = 0;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::CSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
    GetSlots(_state.csSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW**>(ppSamplers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::CSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
    GetSlots(_state.csCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

// ---- ID3D11DeviceContext1 ----

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::CopySubresourceRegion1(ID3D11Resource* pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource* pSrcResource, UINT SrcSubresource, const D3D11_BOX* pSrcBox, UINT CopyFlags)
{
    CopySubresourceRegion(pDstResource, DstSubresource, DstX, DstY, DstZ, pSrcResource, SrcSubresource, pSrcBox);
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::UpdateSubresource1(ID3D11Resource* pDstResource, UINT DstSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch, UINT CopyFlags)
{
    UpdateSubresource(pDstResource, DstSubresource, pDstBox, pSrcData, SrcRowPitch, SrcDepthPitch);
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DiscardResource(ID3D11Resource* pResource)
{
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DiscardView(ID3D11View* pResourceView)
{
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::VSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    SetSlots(_state.vsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
    for (Uint i = 0; i < NumBuffers; ++i)
    {
        _state.vsCBOffsets[StartSlot + i] = pFirstConstant ? pFirstConstant[i] : 0;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::HSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    SetSlots(_state.hsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
    for (Uint i = 0; i < NumBuffers; ++i)
    {
        _state.hsCBOffsets[StartSlot + i] = pFirstConstant ? pFirstConstant[i] : 0;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    SetSlots(_state.dsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
    for (Uint i = 0; i < NumBuffers; ++i)
    {
        _state.dsCBOffsets[StartSlot + i] = pFirstConstant ? pFirstConstant[i] : 0;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::GSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    SetSlots(_state.gsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
    for (Uint i = 0; i < NumBuffers; ++i)
    {
        _state.gsCBOffsets[StartSlot + i] = pFirstConstant ? pFirstConstant[i] : 0;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::PSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    SetSlots(_state.psCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
    for (Uint i = 0; i < NumBuffers; ++i)
    {
        _state.psCBOffsets[StartSlot + i] = pFirstConstant ? pFirstConstant[i] : 0;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::CSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    SetSlots(_state.csCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
    for (Uint i = 0; i < NumBuffers; ++i)
    {
        _state.csCBOffsets[StartSlot + i] = pFirstConstant ? pFirstConstant[i] : 0;
    }
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::VSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants)
{
    GetSlots(_state.vsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::HSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants)
{
    GetSlots(_state.hsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants)
{
    GetSlots(_state.dsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::GSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants)
{
    GetSlots(_state.gsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::PSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants)
{
    GetSlots(_state.psCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::CSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants)
{
    GetSlots(_state.csCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::SwapDeviceContextState(ID3DDeviceContextState* pState, ID3DDeviceContextState** ppPreviousState)
{
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::ClearView(ID3D11View* pView, const FLOAT Color[4], const D3D11_RECT* pRect, UINT NumRects)
{
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::DiscardView1(ID3D11View* pResourceView, const D3D11_RECT* pRects, UINT NumRects)
{
}

// ---- ID3D11DeviceContext2 ----

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::UpdateTileMappings(ID3D11Resource* pTiledResource, UINT NumTiledResourceRegions, const D3D11_TILED_RESOURCE_COORDINATE* pTiledResourceRegionStartCoordinates, const D3D11_TILE_REGION_SIZE* pTiledResourceRegionSizes, ID3D11Buffer* pTilePool, UINT NumRanges, const UINT* pRangeFlags, const UINT* pTilePoolStartOffsets, const UINT* pRangeTileCounts, UINT Flags)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::CopyTileMappings(ID3D11Resource* pDestTiledResource, const D3D11_TILED_RESOURCE_COORDINATE* pDestRegionStartCoordinate, ID3D11Resource* pSourceTiledResource, const D3D11_TILED_RESOURCE_COORDINATE* pSourceRegionStartCoordinate, const D3D11_TILE_REGION_SIZE* pTileRegionSize, UINT Flags)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::CopyTiles(ID3D11Resource* pTiledResource, const D3D11_TILED_RESOURCE_COORDINATE* pTileRegionStartCoordinate, const D3D11_TILE_REGION_SIZE* pTileRegionSize, ID3D11Buffer* pBuffer, UINT64 BufferStartOffsetInBytes, UINT Flags)
{
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::UpdateTiles(ID3D11Resource* pDestTiledResource, const D3D11_TILED_RESOURCE_COORDINATE* pDestTileRegionStartCoordinate, const D3D11_TILE_REGION_SIZE* pDestTileRegionSize, const void* pSourceTileData, UINT Flags)
{
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::ResizeTilePool(ID3D11Buffer* pTilePool, UINT64 NewSizeInBytes)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::TiledResourceBarrier(ID3D11DeviceChild* pTiledResourceOrViewAccessBeforeBarrier, ID3D11DeviceChild* pTiledResourceOrViewAccessAfterBarrier)
{
}

template<Bool IsDebug>
BOOL STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::IsAnnotationEnabled()
{
    return FALSE;
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::SetMarkerInt(LPCWSTR pLabel, INT Data)
{
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::BeginEventInt(LPCWSTR pLabel, INT Data)
{
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::EndEvent()
{
}

// ---- ID3D11DeviceContext3 ----

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::Flush1(D3D11_CONTEXT_TYPE ContextType, HANDLE hEvent)
{
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::SetHardwareProtectionState(BOOL HwProtectionEnable)
{
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::GetHardwareProtectionState(BOOL* pHwProtectionEnable)
{
}

// ---- ID3D11DeviceContext4 ----

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::Signal(ID3D11Fence* pFence, UINT64 Value)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceContextSWImpl<IsDebug>::Wait(ID3D11Fence* pFence, UINT64 Value)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
template<typename... ArgsT>
void D3D11DeviceContextSWImpl<IsDebug>::DebugMsg(const Char* fmt, ArgsT&&... args) const
{
    if constexpr (IsDebug) { D3D11SW_WARN(fmt, std::forward<ArgsT>(args)...); }
}

template class D3D11DeviceContextSWImpl<false>;
template class D3D11DeviceContextSWImpl<true>;

}
