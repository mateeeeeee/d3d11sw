#include "context.h"
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
#include "context/context_util.h"

namespace d3d11sw {

template<typename T>
static void SetSlot(T*& slot, T* value)
{
    if (value)
    {
        value->AddRef();
    }
    if (slot)
    {
        slot->Release();
    }
    slot = value;
}

template<typename T, Usize N>
static void SetSlots(T* (&slots)[N], UINT start, UINT count, T* const* values)
{
    for (UINT i = 0; i < count; i++)
    {
        SetSlot(slots[start + i], values ? values[i] : nullptr);
    }
}

template<typename T, Usize N>
static void GetSlots(T* (&slots)[N], UINT start, UINT count, T** out)
{
    if (!out)
    {
        return;
    }

    for (UINT i = 0; i < count; i++)
    {
        out[i] = slots[start + i];
        if (out[i]) 
        {
            out[i]->AddRef();
        }
    }
}

template<typename Fn>
static void RunOnSWResource(ID3D11Resource* pResource, Fn&& fn)
{
    if (!pResource) 
    {
        return;
    }

    D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
    pResource->GetType(&dim);
    switch (dim)
    {
    case D3D11_RESOURCE_DIMENSION_BUFFER:
        fn(static_cast<D3D11BufferSW*>(static_cast<ID3D11Buffer*>(pResource)));
        break;
    case D3D11_RESOURCE_DIMENSION_TEXTURE1D:
        fn(static_cast<D3D11Texture1DSW*>(static_cast<ID3D11Texture1D*>(pResource)));
        break;
    case D3D11_RESOURCE_DIMENSION_TEXTURE2D:
        fn(static_cast<D3D11Texture2DSW*>(static_cast<ID3D11Texture2D1*>(pResource)));
        break;
    case D3D11_RESOURCE_DIMENSION_TEXTURE3D:
        fn(static_cast<D3D11Texture3DSW*>(static_cast<ID3D11Texture3D1*>(pResource)));
        break;
    default:
        break;
    }
}


HRESULT STDMETHODCALLTYPE D3D11DeviceContextSW::QueryInterface(REFIID riid, void** ppv)
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

D3D11DeviceContextSW::D3D11DeviceContextSW(ID3D11Device* device)
    : DeviceChildImpl(device)
{
    _state.blendFactor[0] = _state.blendFactor[1] =
    _state.blendFactor[2] = _state.blendFactor[3] = 1.0f;
    _state.sampleMask = 0xFFFFFFFF;
}

D3D11DeviceContextSW::~D3D11DeviceContextSW()
{
    _state.ReleaseAll();
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::VSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers)
{
    SetSlots(_state.vsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::PSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView*const* ppShaderResourceViews)
{
    SetSlots(_state.psSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW*const*>(ppShaderResourceViews));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::PSSetShader(ID3D11PixelShader* pPixelShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
{
    SetSlot(_state.ps, static_cast<D3D11PixelShaderSW*>(pPixelShader));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::PSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState*const* ppSamplers)
{
    SetSlots(_state.psSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW*const*>(ppSamplers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::VSSetShader(ID3D11VertexShader* pVertexShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
{
    SetSlot(_state.vs, static_cast<D3D11VertexShaderSW*>(pVertexShader));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::PSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers)
{
    SetSlots(_state.psCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::IASetInputLayout(ID3D11InputLayout* pInputLayout)
{
    SetSlot(_state.inputLayout, static_cast<D3D11InputLayoutSW*>(pInputLayout));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::IASetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppVertexBuffers, const UINT* pStrides, const UINT* pOffsets)
{
    for (UINT i = 0; i < NumBuffers; i++)
    {
        SetSlot(_state.vertexBuffers[StartSlot + i], ppVertexBuffers ? static_cast<D3D11BufferSW*>(ppVertexBuffers[i]) : nullptr);
        _state.vbStrides[StartSlot + i] = ppVertexBuffers && pStrides ? pStrides[i] : 0;
        _state.vbOffsets[StartSlot + i] = ppVertexBuffers && pOffsets ? pOffsets[i] : 0;
    }
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::IASetIndexBuffer(ID3D11Buffer* pIndexBuffer, DXGI_FORMAT Format, UINT Offset)
{
    SetSlot(_state.indexBuffer, static_cast<D3D11BufferSW*>(pIndexBuffer));
    _state.indexFormat = Format;
    _state.indexOffset = Offset;
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY Topology)
{
    _state.topology = Topology;
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::GSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers)
{
    SetSlots(_state.gsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::GSSetShader(ID3D11GeometryShader* pShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
{
    SetSlot(_state.gs, static_cast<D3D11GeometryShaderSW*>(pShader));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::VSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView*const* ppShaderResourceViews)
{
    SetSlots(_state.vsSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW*const*>(ppShaderResourceViews));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::VSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState*const* ppSamplers)
{
    SetSlots(_state.vsSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW*const*>(ppSamplers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::SetPredication(ID3D11Predicate* pPredicate, BOOL PredicateValue)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::GSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView*const* ppShaderResourceViews)
{
    SetSlots(_state.gsSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW*const*>(ppShaderResourceViews));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::GSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState*const* ppSamplers)
{
    SetSlots(_state.gsSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW*const*>(ppSamplers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::OMSetRenderTargets(UINT NumViews, ID3D11RenderTargetView*const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView)
{
    for (UINT i = NumViews; i < _state.numRenderTargets; i++)
    {
        SetSlot(_state.renderTargets[i], static_cast<D3D11RenderTargetViewSW*>(nullptr));
    }
    _state.numRenderTargets = NumViews;
    for (UINT i = 0; i < NumViews; i++)
    {
        SetSlot(_state.renderTargets[i], ppRenderTargetViews ? static_cast<D3D11RenderTargetViewSW*>(ppRenderTargetViews[i]) : nullptr);
    }
    SetSlot(_state.depthStencilView, static_cast<D3D11DepthStencilViewSW*>(pDepthStencilView));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::OMSetRenderTargetsAndUnorderedAccessViews(UINT NumRTVs, ID3D11RenderTargetView*const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView*const* ppUnorderedAccessViews, const UINT* pUAVInitialCounts)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::OMSetBlendState(ID3D11BlendState* pBlendState, const FLOAT BlendFactor[4], UINT SampleMask)
{
    SetSlot(_state.blendState, static_cast<D3D11BlendStateSW*>(pBlendState));
    if (BlendFactor)
    {
        std::memcpy(_state.blendFactor, BlendFactor, sizeof(FLOAT) * 4);
    }
    else
    {
        _state.blendFactor[0] = _state.blendFactor[1] =
        _state.blendFactor[2] = _state.blendFactor[3] = 1.0f;
    }
    _state.sampleMask = SampleMask;
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::OMSetDepthStencilState(ID3D11DepthStencilState* pDepthStencilState, UINT StencilRef)
{
    SetSlot(_state.depthStencilState, static_cast<D3D11DepthStencilStateSW*>(pDepthStencilState));
    _state.stencilRef = StencilRef;
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::SOSetTargets(UINT NumBuffers, ID3D11Buffer*const* ppSOTargets, const UINT* pOffsets)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::RSSetState(ID3D11RasterizerState* pRasterizerState)
{
    SetSlot(_state.rsState, static_cast<D3D11RasterizerStateSW*>(pRasterizerState));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::RSSetViewports(UINT NumViewports, const D3D11_VIEWPORT* pViewports)
{
    _state.numViewports = NumViewports;
    if (pViewports)
    {
        std::memcpy(_state.viewports, pViewports, NumViewports * sizeof(D3D11_VIEWPORT));
    }
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::RSSetScissorRects(UINT NumRects, const D3D11_RECT* pRects)
{
    _state.numScissorRects = NumRects;
    if (pRects)
    {
        std::memcpy(_state.scissorRects, pRects, NumRects * sizeof(D3D11_RECT));
    }
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::HSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView*const* ppShaderResourceViews)
{
    SetSlots(_state.hsSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW*const*>(ppShaderResourceViews));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::HSSetShader(ID3D11HullShader* pHullShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
{
    SetSlot(_state.hs, static_cast<D3D11HullShaderSW*>(pHullShader));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::HSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState*const* ppSamplers)
{
    SetSlots(_state.hsSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW*const*>(ppSamplers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::HSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers)
{
    SetSlots(_state.hsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::DSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView*const* ppShaderResourceViews)
{
    SetSlots(_state.dsSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW*const*>(ppShaderResourceViews));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::DSSetShader(ID3D11DomainShader* pDomainShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
{
    SetSlot(_state.ds, static_cast<D3D11DomainShaderSW*>(pDomainShader));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::DSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState*const* ppSamplers)
{
    SetSlots(_state.dsSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW*const*>(ppSamplers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::DSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers)
{
    SetSlots(_state.dsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::CSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView*const* ppShaderResourceViews)
{
    SetSlots(_state.csSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW*const*>(ppShaderResourceViews));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::CSSetUnorderedAccessViews(UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView*const* ppUnorderedAccessViews, const UINT* pUAVInitialCounts)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::CSSetShader(ID3D11ComputeShader* pComputeShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
{
    SetSlot(_state.cs, static_cast<D3D11ComputeShaderSW*>(pComputeShader));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::CSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState*const* ppSamplers)
{
    SetSlots(_state.csSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW*const*>(ppSamplers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::CSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers)
{
    SetSlots(_state.csCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::SetResourceMinLOD(ID3D11Resource* pResource, FLOAT MinLOD)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::Draw(UINT VertexCount, UINT StartVertexLocation)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::DrawAuto()
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::DrawIndexedInstancedIndirect(ID3D11Buffer* pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::DrawInstancedIndirect(ID3D11Buffer* pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::DispatchIndirect(ID3D11Buffer* pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
}

HRESULT STDMETHODCALLTYPE D3D11DeviceContextSW::Map(ID3D11Resource* pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource)
{
    if (!pResource)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = E_NOTIMPL;
    RunOnSWResource(pResource, [&](auto* res) 
    {
        if (!pMappedResource)
        {
            D3D11_RESOURCE_DIMENSION dim = D3D11_RESOURCE_DIMENSION_UNKNOWN;
            res->GetType(&dim);
            if (dim == D3D11_RESOURCE_DIMENSION_BUFFER || res->GetUsage() != D3D11_USAGE_DEFAULT)
            {
                hr = E_INVALIDARG;
                return;
            }
            hr = S_OK;
            return;
        }

        auto layout                 = res->GetSubresourceLayout(Subresource);
        pMappedResource->pData      = static_cast<Uint8*>(res->GetDataPtr()) + layout.Offset;
        pMappedResource->RowPitch   = layout.RowPitch;
        pMappedResource->DepthPitch = layout.DepthPitch;
        hr = S_OK;
    });
    return hr;
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::Unmap(ID3D11Resource* pResource, UINT Subresource)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::CopySubresourceRegion(ID3D11Resource* pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource* pSrcResource, UINT SrcSubresource, const D3D11_BOX* pSrcBox)
{
    RunOnSWResource(pSrcResource, [&](auto* src) 
    {
        RunOnSWResource(pDstResource, [&](auto* dst) 
        {
            auto srcLayout = src->GetSubresourceLayout(SrcSubresource);
            auto dstLayout = dst->GetSubresourceLayout(DstSubresource);

            const Uint8* srcBase = static_cast<const Uint8*>(src->GetDataPtr()) + srcLayout.Offset;
            Uint8*       dstBase = static_cast<Uint8*>(dst->GetDataPtr())       + dstLayout.Offset;

            UINT bx0         = pSrcBox ? pSrcBox->left  / srcLayout.BlockSize : 0;
            UINT by0         = pSrcBox ? pSrcBox->top   / srcLayout.BlockSize : 0;
            UINT bz0         = pSrcBox ? pSrcBox->front : 0;
            UINT copyBlocksX = pSrcBox ? (pSrcBox->right  - pSrcBox->left)  / srcLayout.BlockSize : srcLayout.RowPitch / srcLayout.PixelStride;
            UINT copyBlocksY = pSrcBox ? (pSrcBox->bottom - pSrcBox->top)   / srcLayout.BlockSize : srcLayout.NumRows;
            UINT copySlices  = pSrcBox ? (pSrcBox->back   - pSrcBox->front)                       : srcLayout.NumSlices;
            UINT copyBytes   = copyBlocksX * srcLayout.PixelStride;

            UINT dbx = DstX / dstLayout.BlockSize;
            UINT dby = DstY / dstLayout.BlockSize;

            for (UINT z = 0; z < copySlices; ++z)
            {
                for (UINT y = 0; y < copyBlocksY; ++y)
                {
                    std::memcpy(
                    dstBase + (UINT64)(DstZ + bz0 + z) * dstLayout.DepthPitch + (UINT64)(dby + y) * dstLayout.RowPitch + dbx * dstLayout.PixelStride,
                    srcBase + (UINT64)(bz0  + z)       * srcLayout.DepthPitch + (UINT64)(by0 + y) * srcLayout.RowPitch + bx0 * srcLayout.PixelStride,
                    copyBytes);
                }
            }
        });
    });
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::CopyResource(ID3D11Resource* pDstResource, ID3D11Resource* pSrcResource)
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

void STDMETHODCALLTYPE D3D11DeviceContextSW::UpdateSubresource(ID3D11Resource* pDstResource, UINT DstSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch)
{
    if (!pSrcData) 
    {
        return;
    }

    RunOnSWResource(pDstResource, [&](auto* dst) 
    {
        auto         layout  = dst->GetSubresourceLayout(DstSubresource);
        Uint8*       dstBase = static_cast<Uint8*>(dst->GetDataPtr()) + layout.Offset;
        const Uint8* src     = static_cast<const Uint8*>(pSrcData);
        if (!pDstBox)
        {
            for (UINT z = 0; z < layout.NumSlices; ++z)
            {
                for (UINT y = 0; y < layout.NumRows; ++y)
                {
                    std::memcpy(
                        dstBase + (UINT64)z * layout.DepthPitch + (UINT64)y * layout.RowPitch,
                        src     + (UINT64)z * SrcDepthPitch     + (UINT64)y * SrcRowPitch,
                        layout.RowPitch);
                }
            }
        }
        else
        {
            UINT bx0         = pDstBox->left  / layout.BlockSize;
            UINT by0         = pDstBox->top   / layout.BlockSize;
            UINT bz0         = pDstBox->front;
            UINT copyBlocksX = (pDstBox->right  - pDstBox->left)  / layout.BlockSize;
            UINT copyBlocksY = (pDstBox->bottom - pDstBox->top)   / layout.BlockSize;
            UINT copySlices  =  pDstBox->back   - pDstBox->front;
            UINT copyBytes   = copyBlocksX * layout.PixelStride;

            for (UINT z = 0; z < copySlices; ++z)
            {
                for (UINT y = 0; y < copyBlocksY; ++y)
                {
                    std::memcpy(
                        dstBase + (UINT64)(bz0 + z) * layout.DepthPitch + (UINT64)(by0 + y) * layout.RowPitch + bx0 * layout.PixelStride,
                        src     + (UINT64)z          * SrcDepthPitch     + (UINT64)y          * SrcRowPitch,
                        copyBytes);
                }
            }
        }
    });
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::CopyStructureCount(ID3D11Buffer* pDstBuffer, UINT DstAlignedByteOffset, ID3D11UnorderedAccessView* pSrcView)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::ClearRenderTargetView(ID3D11RenderTargetView* pRenderTargetView, const FLOAT ColorRGBA[4])
{
    if (!pRenderTargetView)
    {
        return;
    }

    D3D11RenderTargetViewSW* rtv    = static_cast<D3D11RenderTargetViewSW*>(pRenderTargetView);
    D3D11SW_SUBRESOURCE_LAYOUT layout = rtv->GetLayout();

    UINT8 pixel[16];
    PackRTVColor(rtv->GetFormat(), ColorRGBA, pixel);
    ForEachPixel(rtv->GetDataPtr(), layout, [&pixel, &layout](UINT8* px)
    {
        std::memcpy(px, pixel, layout.PixelStride);
    });
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::ClearUnorderedAccessViewUint(ID3D11UnorderedAccessView* pUnorderedAccessView, const UINT Values[4])
{
    if (!pUnorderedAccessView)
    {
        return;
    }

    D3D11UnorderedAccessViewSW* uav = static_cast<D3D11UnorderedAccessViewSW*>(pUnorderedAccessView);
    D3D11SW_SUBRESOURCE_LAYOUT layout = uav->GetLayout();

    UINT8 pixel[16];
    PackUAVUint(uav->GetFormat(), Values, pixel);
    ForEachPixel(uav->GetDataPtr(), layout, [&pixel, &layout](UINT8* px)
    {
        std::memcpy(px, pixel, layout.PixelStride);
    });
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::ClearUnorderedAccessViewFloat(ID3D11UnorderedAccessView* pUnorderedAccessView, const FLOAT Values[4])
{
    if (!pUnorderedAccessView)
    {
        return;
    }

    D3D11UnorderedAccessViewSW* uav    = static_cast<D3D11UnorderedAccessViewSW*>(pUnorderedAccessView);
    D3D11SW_SUBRESOURCE_LAYOUT layout = uav->GetLayout();

    UINT8 pixel[16];
    PackRTVColor(uav->GetFormat(), Values, pixel);
    ForEachPixel(uav->GetDataPtr(), layout, [&pixel, &layout](UINT8* px)
    {
        std::memcpy(px, pixel, layout.PixelStride);
    });
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::ClearDepthStencilView(ID3D11DepthStencilView* pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
{
    if (!pDepthStencilView)
    {
        return;
    }

    D3D11DepthStencilViewSW* dsv = static_cast<D3D11DepthStencilViewSW*>(pDepthStencilView);
    UINT8*  data   = dsv->GetDataPtr();
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
            ForEachPixel(data, layout, [Depth](UINT8* px)
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
            UINT16 d16 = (UINT16)(std::clamp(Depth, 0.f, 1.f) * 65535.f + 0.5f);
            ForEachPixel(data, layout, [d16](UINT8* px)
            {
                std::memcpy(px, &d16, 2);
            });
            break;
        }
        case DXGI_FORMAT_D24_UNORM_S8_UINT:
        {
            Bool clearDepth   = (ClearFlags & D3D11_CLEAR_DEPTH)   != 0;
            Bool clearStencil = (ClearFlags & D3D11_CLEAR_STENCIL) != 0;
            UINT32 d24 = std::min((UINT32)(std::clamp(Depth, 0.f, 1.f) * 16777215.f + 0.5f), 0xFFFFFFu);
            if (clearDepth && clearStencil)
            {
                UINT32 packed = (d24 << 8) | Stencil;
                ForEachPixel(data, layout, [packed](UINT8* px)
                {
                    std::memcpy(px, &packed, 4);
                });
            }
            else if (clearDepth)
            {
                ForEachPixel(data, layout, [d24](UINT8* px)
                {
                    UINT32 v; std::memcpy(&v, px, 4);
                    v = (d24 << 8) | (v & 0xFFu);
                    std::memcpy(px, &v, 4);
                });
            }
            else if (clearStencil)
            {
                ForEachPixel(data, layout, [Stencil](UINT8* px)
                {
                    UINT32 v; std::memcpy(&v, px, 4);
                    v = (v & 0xFFFFFF00u) | Stencil;
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
                ForEachPixel(data, layout, [Depth](UINT8* px)
                {
                    std::memcpy(px, &Depth, 4);
                });
            }
            if (clearStencil)
            {
                ForEachPixel(data, layout, [Stencil](UINT8* px)
                {
                    px[4] = Stencil;
                });
            }
            break;
        }
        default:
            break;
    }
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::GenerateMips(ID3D11ShaderResourceView* pShaderResourceView)
{
}

FLOAT STDMETHODCALLTYPE D3D11DeviceContextSW::GetResourceMinLOD(ID3D11Resource* pResource)
{
    return 0.0f;
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::ResolveSubresource(ID3D11Resource* pDstResource, UINT DstSubresource, ID3D11Resource* pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::Begin(ID3D11Asynchronous* pAsync)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::End(ID3D11Asynchronous* pAsync)
{
}

HRESULT STDMETHODCALLTYPE D3D11DeviceContextSW::GetData(ID3D11Asynchronous* pAsync, void* pData, UINT DataSize, UINT GetDataFlags)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::ExecuteCommandList(ID3D11CommandList* pCommandList, BOOL RestoreContextState)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::ClearState()
{
    _state.ReleaseAll();
    _state = {};
    _state.blendFactor[0] = _state.blendFactor[1] =
    _state.blendFactor[2] = _state.blendFactor[3] = 1.0f;
    _state.sampleMask = 0xFFFFFFFF;
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::Flush()
{
}

D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE D3D11DeviceContextSW::GetType()
{
    return D3D11_DEVICE_CONTEXT_IMMEDIATE;
}

UINT STDMETHODCALLTYPE D3D11DeviceContextSW::GetContextFlags()
{
    return 0;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceContextSW::FinishCommandList(BOOL RestoreDeferredContextState, ID3D11CommandList** ppCommandList)
{
    return DXGI_ERROR_INVALID_CALL;
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::VSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
    GetSlots(_state.vsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::PSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
    GetSlots(_state.psSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW**>(ppShaderResourceViews));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::PSGetShader(ID3D11PixelShader** ppPixelShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
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

void STDMETHODCALLTYPE D3D11DeviceContextSW::PSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
    GetSlots(_state.psSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW**>(ppSamplers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::VSGetShader(ID3D11VertexShader** ppVertexShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
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

void STDMETHODCALLTYPE D3D11DeviceContextSW::PSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
    GetSlots(_state.psCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::IAGetInputLayout(ID3D11InputLayout** ppInputLayout)
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

void STDMETHODCALLTYPE D3D11DeviceContextSW::IAGetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppVertexBuffers, UINT* pStrides, UINT* pOffsets)
{
    for (UINT i = 0; i < NumBuffers; i++)
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

void STDMETHODCALLTYPE D3D11DeviceContextSW::IAGetIndexBuffer(ID3D11Buffer** pIndexBuffer, DXGI_FORMAT* Format, UINT* Offset)
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

void STDMETHODCALLTYPE D3D11DeviceContextSW::IAGetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY* pTopology)
{
    if (pTopology)
    {
        *pTopology = _state.topology;
    }
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::GSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
    GetSlots(_state.gsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::GSGetShader(ID3D11GeometryShader** ppGeometryShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
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

void STDMETHODCALLTYPE D3D11DeviceContextSW::VSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
    GetSlots(_state.vsSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW**>(ppShaderResourceViews));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::VSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
    GetSlots(_state.vsSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW**>(ppSamplers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::GetPredication(ID3D11Predicate** ppPredicate, BOOL* pPredicateValue)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::GSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
    GetSlots(_state.gsSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW**>(ppShaderResourceViews));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::GSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
    GetSlots(_state.gsSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW**>(ppSamplers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::OMGetRenderTargets(UINT NumViews, ID3D11RenderTargetView** ppRenderTargetViews, ID3D11DepthStencilView** ppDepthStencilView)
{
    if (ppRenderTargetViews)
    {
        for (UINT i = 0; i < NumViews; i++)
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

void STDMETHODCALLTYPE D3D11DeviceContextSW::OMGetRenderTargetsAndUnorderedAccessViews(UINT NumRTVs, ID3D11RenderTargetView** ppRenderTargetViews, ID3D11DepthStencilView** ppDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView** ppUnorderedAccessViews)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::OMGetBlendState(ID3D11BlendState** ppBlendState, FLOAT BlendFactor[4], UINT* pSampleMask)
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
        std::memcpy(BlendFactor, _state.blendFactor, sizeof(FLOAT) * 4);
    }
    if (pSampleMask)
    {
        *pSampleMask = _state.sampleMask;
    }
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::OMGetDepthStencilState(ID3D11DepthStencilState** ppDepthStencilState, UINT* pStencilRef)
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

void STDMETHODCALLTYPE D3D11DeviceContextSW::SOGetTargets(UINT NumBuffers, ID3D11Buffer** ppSOTargets)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::RSGetState(ID3D11RasterizerState** ppRasterizerState)
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

void STDMETHODCALLTYPE D3D11DeviceContextSW::RSGetViewports(UINT* pNumViewports, D3D11_VIEWPORT* pViewports)
{
    if (pNumViewports)
    {
        if (pViewports)
        {
            UINT copyCount = *pNumViewports < _state.numViewports ? *pNumViewports : _state.numViewports;
            std::memcpy(pViewports, _state.viewports, copyCount * sizeof(D3D11_VIEWPORT));
        }
        *pNumViewports = _state.numViewports;
    }
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::RSGetScissorRects(UINT* pNumRects, D3D11_RECT* pRects)
{
    if (pNumRects)
    {
        if (pRects)
        {
            UINT copyCount = *pNumRects < _state.numScissorRects ? *pNumRects : _state.numScissorRects;
            std::memcpy(pRects, _state.scissorRects, copyCount * sizeof(D3D11_RECT));
        }
        *pNumRects = _state.numScissorRects;
    }
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::HSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
    GetSlots(_state.hsSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW**>(ppShaderResourceViews));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::HSGetShader(ID3D11HullShader** ppHullShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
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

void STDMETHODCALLTYPE D3D11DeviceContextSW::HSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
    GetSlots(_state.hsSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW**>(ppSamplers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::HSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
    GetSlots(_state.hsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::DSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
    GetSlots(_state.dsSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW**>(ppShaderResourceViews));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::DSGetShader(ID3D11DomainShader** ppDomainShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
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

void STDMETHODCALLTYPE D3D11DeviceContextSW::DSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
    GetSlots(_state.dsSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW**>(ppSamplers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::DSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
    GetSlots(_state.dsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::CSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
    GetSlots(_state.csSRVs, StartSlot, NumViews, reinterpret_cast<D3D11ShaderResourceViewSW**>(ppShaderResourceViews));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::CSGetUnorderedAccessViews(UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView** ppUnorderedAccessViews)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::CSGetShader(ID3D11ComputeShader** ppComputeShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
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

void STDMETHODCALLTYPE D3D11DeviceContextSW::CSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
    GetSlots(_state.csSamplers, StartSlot, NumSamplers, reinterpret_cast<D3D11SamplerStateSW**>(ppSamplers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::CSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
    GetSlots(_state.csCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

// ---- ID3D11DeviceContext1 ----

void STDMETHODCALLTYPE D3D11DeviceContextSW::CopySubresourceRegion1(ID3D11Resource* pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource* pSrcResource, UINT SrcSubresource, const D3D11_BOX* pSrcBox, UINT CopyFlags)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::UpdateSubresource1(ID3D11Resource* pDstResource, UINT DstSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch, UINT CopyFlags)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::DiscardResource(ID3D11Resource* pResource)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::DiscardView(ID3D11View* pResourceView)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::VSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    D3D11SW_TODO("store pFirstConstant/pNumConstants per slot for partial CB binding");
    SetSlots(_state.vsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::HSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    D3D11SW_TODO("store pFirstConstant/pNumConstants per slot for partial CB binding");
    SetSlots(_state.hsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::DSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    D3D11SW_TODO("store pFirstConstant/pNumConstants per slot for partial CB binding");
    SetSlots(_state.dsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::GSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    D3D11SW_TODO("store pFirstConstant/pNumConstants per slot for partial CB binding");
    SetSlots(_state.gsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::PSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    D3D11SW_TODO("store pFirstConstant/pNumConstants per slot for partial CB binding");
    SetSlots(_state.psCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::CSSetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers, const UINT* pFirstConstant, const UINT* pNumConstants)
{
    D3D11SW_TODO("store pFirstConstant/pNumConstants per slot for partial CB binding");
    SetSlots(_state.csCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW*const*>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::VSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants)
{
    GetSlots(_state.vsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::HSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants)
{
    GetSlots(_state.hsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::DSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants)
{
    GetSlots(_state.dsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::GSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants)
{
    GetSlots(_state.gsCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::PSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants)
{
    GetSlots(_state.psCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::CSGetConstantBuffers1(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers, UINT* pFirstConstant, UINT* pNumConstants)
{
    GetSlots(_state.csCBs, StartSlot, NumBuffers, reinterpret_cast<D3D11BufferSW**>(ppConstantBuffers));
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::SwapDeviceContextState(ID3DDeviceContextState* pState, ID3DDeviceContextState** ppPreviousState)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::ClearView(ID3D11View* pView, const FLOAT Color[4], const D3D11_RECT* pRect, UINT NumRects)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::DiscardView1(ID3D11View* pResourceView, const D3D11_RECT* pRects, UINT NumRects)
{
}

// ---- ID3D11DeviceContext2 ----

HRESULT STDMETHODCALLTYPE D3D11DeviceContextSW::UpdateTileMappings(ID3D11Resource* pTiledResource, UINT NumTiledResourceRegions, const D3D11_TILED_RESOURCE_COORDINATE* pTiledResourceRegionStartCoordinates, const D3D11_TILE_REGION_SIZE* pTiledResourceRegionSizes, ID3D11Buffer* pTilePool, UINT NumRanges, const UINT* pRangeFlags, const UINT* pTilePoolStartOffsets, const UINT* pRangeTileCounts, UINT Flags)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceContextSW::CopyTileMappings(ID3D11Resource* pDestTiledResource, const D3D11_TILED_RESOURCE_COORDINATE* pDestRegionStartCoordinate, ID3D11Resource* pSourceTiledResource, const D3D11_TILED_RESOURCE_COORDINATE* pSourceRegionStartCoordinate, const D3D11_TILE_REGION_SIZE* pTileRegionSize, UINT Flags)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::CopyTiles(ID3D11Resource* pTiledResource, const D3D11_TILED_RESOURCE_COORDINATE* pTileRegionStartCoordinate, const D3D11_TILE_REGION_SIZE* pTileRegionSize, ID3D11Buffer* pBuffer, UINT64 BufferStartOffsetInBytes, UINT Flags)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::UpdateTiles(ID3D11Resource* pDestTiledResource, const D3D11_TILED_RESOURCE_COORDINATE* pDestTileRegionStartCoordinate, const D3D11_TILE_REGION_SIZE* pDestTileRegionSize, const void* pSourceTileData, UINT Flags)
{
}

HRESULT STDMETHODCALLTYPE D3D11DeviceContextSW::ResizeTilePool(ID3D11Buffer* pTilePool, UINT64 NewSizeInBytes)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::TiledResourceBarrier(ID3D11DeviceChild* pTiledResourceOrViewAccessBeforeBarrier, ID3D11DeviceChild* pTiledResourceOrViewAccessAfterBarrier)
{
}

BOOL STDMETHODCALLTYPE D3D11DeviceContextSW::IsAnnotationEnabled()
{
    return FALSE;
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::SetMarkerInt(LPCWSTR pLabel, INT Data)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::BeginEventInt(LPCWSTR pLabel, INT Data)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::EndEvent()
{
}

// ---- ID3D11DeviceContext3 ----

void STDMETHODCALLTYPE D3D11DeviceContextSW::Flush1(D3D11_CONTEXT_TYPE ContextType, HANDLE hEvent)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::SetHardwareProtectionState(BOOL HwProtectionEnable)
{
}

void STDMETHODCALLTYPE D3D11DeviceContextSW::GetHardwareProtectionState(BOOL* pHwProtectionEnable)
{
}

// ---- ID3D11DeviceContext4 ----

HRESULT STDMETHODCALLTYPE D3D11DeviceContextSW::Signal(ID3D11Fence* pFence, UINT64 Value)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceContextSW::Wait(ID3D11Fence* pFence, UINT64 Value)
{
    return E_NOTIMPL;
}

}
