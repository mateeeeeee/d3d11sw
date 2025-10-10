#include "context.h"

MarsDeviceContext::MarsDeviceContext(ID3D11Device* device)
    : DeviceChildImpl(device)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::VSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::PSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView*const* ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::PSSetShader(ID3D11PixelShader* pPixelShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::PSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState*const* ppSamplers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::VSSetShader(ID3D11VertexShader* pVertexShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::PSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::IASetInputLayout(ID3D11InputLayout* pInputLayout)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::IASetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppVertexBuffers, const UINT* pStrides, const UINT* pOffsets)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::IASetIndexBuffer(ID3D11Buffer* pIndexBuffer, DXGI_FORMAT Format, UINT Offset)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY Topology)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::GSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::GSSetShader(ID3D11GeometryShader* pShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::VSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView*const* ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::VSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState*const* ppSamplers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::SetPredication(ID3D11Predicate* pPredicate, BOOL PredicateValue)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::GSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView*const* ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::GSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState*const* ppSamplers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::OMSetRenderTargets(UINT NumViews, ID3D11RenderTargetView*const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::OMSetRenderTargetsAndUnorderedAccessViews(UINT NumRTVs, ID3D11RenderTargetView*const* ppRenderTargetViews, ID3D11DepthStencilView* pDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView*const* ppUnorderedAccessViews, const UINT* pUAVInitialCounts)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::OMSetBlendState(ID3D11BlendState* pBlendState, const FLOAT BlendFactor[4], UINT SampleMask)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::OMSetDepthStencilState(ID3D11DepthStencilState* pDepthStencilState, UINT StencilRef)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::SOSetTargets(UINT NumBuffers, ID3D11Buffer*const* ppSOTargets, const UINT* pOffsets)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::RSSetState(ID3D11RasterizerState* pRasterizerState)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::RSSetViewports(UINT NumViewports, const D3D11_VIEWPORT* pViewports)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::RSSetScissorRects(UINT NumRects, const D3D11_RECT* pRects)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::HSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView*const* ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::HSSetShader(ID3D11HullShader* pHullShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::HSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState*const* ppSamplers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::HSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::DSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView*const* ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::DSSetShader(ID3D11DomainShader* pDomainShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::DSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState*const* ppSamplers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::DSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::CSSetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView*const* ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::CSSetUnorderedAccessViews(UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView*const* ppUnorderedAccessViews, const UINT* pUAVInitialCounts)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::CSSetShader(ID3D11ComputeShader* pComputeShader, ID3D11ClassInstance*const* ppClassInstances, UINT NumClassInstances)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::CSSetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState*const* ppSamplers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::CSSetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer*const* ppConstantBuffers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::SetResourceMinLOD(ID3D11Resource* pResource, FLOAT MinLOD)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::Draw(UINT VertexCount, UINT StartVertexLocation)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation, INT BaseVertexLocation, UINT StartInstanceLocation)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation, UINT StartInstanceLocation)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::DrawAuto()
{
}

void STDMETHODCALLTYPE MarsDeviceContext::DrawIndexedInstancedIndirect(ID3D11Buffer* pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::DrawInstancedIndirect(ID3D11Buffer* pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::Dispatch(UINT ThreadGroupCountX, UINT ThreadGroupCountY, UINT ThreadGroupCountZ)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::DispatchIndirect(ID3D11Buffer* pBufferForArgs, UINT AlignedByteOffsetForArgs)
{
}

HRESULT STDMETHODCALLTYPE MarsDeviceContext::Map(ID3D11Resource* pResource, UINT Subresource, D3D11_MAP MapType, UINT MapFlags, D3D11_MAPPED_SUBRESOURCE* pMappedResource)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE MarsDeviceContext::Unmap(ID3D11Resource* pResource, UINT Subresource)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::CopySubresourceRegion(ID3D11Resource* pDstResource, UINT DstSubresource, UINT DstX, UINT DstY, UINT DstZ, ID3D11Resource* pSrcResource, UINT SrcSubresource, const D3D11_BOX* pSrcBox)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::CopyResource(ID3D11Resource* pDstResource, ID3D11Resource* pSrcResource)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::UpdateSubresource(ID3D11Resource* pDstResource, UINT DstSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::CopyStructureCount(ID3D11Buffer* pDstBuffer, UINT DstAlignedByteOffset, ID3D11UnorderedAccessView* pSrcView)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::ClearRenderTargetView(ID3D11RenderTargetView* pRenderTargetView, const FLOAT ColorRGBA[4])
{
}

void STDMETHODCALLTYPE MarsDeviceContext::ClearUnorderedAccessViewUint(ID3D11UnorderedAccessView* pUnorderedAccessView, const UINT Values[4])
{
}

void STDMETHODCALLTYPE MarsDeviceContext::ClearUnorderedAccessViewFloat(ID3D11UnorderedAccessView* pUnorderedAccessView, const FLOAT Values[4])
{
}

void STDMETHODCALLTYPE MarsDeviceContext::ClearDepthStencilView(ID3D11DepthStencilView* pDepthStencilView, UINT ClearFlags, FLOAT Depth, UINT8 Stencil)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::GenerateMips(ID3D11ShaderResourceView* pShaderResourceView)
{
}

FLOAT STDMETHODCALLTYPE MarsDeviceContext::GetResourceMinLOD(ID3D11Resource* pResource)
{
    return 0.0f;
}

void STDMETHODCALLTYPE MarsDeviceContext::ResolveSubresource(ID3D11Resource* pDstResource, UINT DstSubresource, ID3D11Resource* pSrcResource, UINT SrcSubresource, DXGI_FORMAT Format)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::Begin(ID3D11Asynchronous* pAsync)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::End(ID3D11Asynchronous* pAsync)
{
}

HRESULT STDMETHODCALLTYPE MarsDeviceContext::GetData(ID3D11Asynchronous* pAsync, void* pData, UINT DataSize, UINT GetDataFlags)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE MarsDeviceContext::ExecuteCommandList(ID3D11CommandList* pCommandList, BOOL RestoreContextState)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::ClearState()
{
}

void STDMETHODCALLTYPE MarsDeviceContext::Flush()
{
}

D3D11_DEVICE_CONTEXT_TYPE STDMETHODCALLTYPE MarsDeviceContext::GetType()
{
    return D3D11_DEVICE_CONTEXT_IMMEDIATE;
}

UINT STDMETHODCALLTYPE MarsDeviceContext::GetContextFlags()
{
    return 0;
}

HRESULT STDMETHODCALLTYPE MarsDeviceContext::FinishCommandList(BOOL RestoreDeferredContextState, ID3D11CommandList** ppCommandList)
{
    return DXGI_ERROR_INVALID_CALL;
}

void STDMETHODCALLTYPE MarsDeviceContext::VSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::PSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::PSGetShader(ID3D11PixelShader** ppPixelShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::PSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::VSGetShader(ID3D11VertexShader** ppVertexShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::PSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::IAGetInputLayout(ID3D11InputLayout** ppInputLayout)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::IAGetVertexBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppVertexBuffers, UINT* pStrides, UINT* pOffsets)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::IAGetIndexBuffer(ID3D11Buffer** pIndexBuffer, DXGI_FORMAT* Format, UINT* Offset)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::GSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::GSGetShader(ID3D11GeometryShader** ppGeometryShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::IAGetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY* pTopology)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::VSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::VSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::GetPredication(ID3D11Predicate** ppPredicate, BOOL* pPredicateValue)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::GSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::GSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::OMGetRenderTargets(UINT NumViews, ID3D11RenderTargetView** ppRenderTargetViews, ID3D11DepthStencilView** ppDepthStencilView)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::OMGetRenderTargetsAndUnorderedAccessViews(UINT NumRTVs, ID3D11RenderTargetView** ppRenderTargetViews, ID3D11DepthStencilView** ppDepthStencilView, UINT UAVStartSlot, UINT NumUAVs, ID3D11UnorderedAccessView** ppUnorderedAccessViews)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::OMGetBlendState(ID3D11BlendState** ppBlendState, FLOAT BlendFactor[4], UINT* pSampleMask)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::OMGetDepthStencilState(ID3D11DepthStencilState** ppDepthStencilState, UINT* pStencilRef)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::SOGetTargets(UINT NumBuffers, ID3D11Buffer** ppSOTargets)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::RSGetState(ID3D11RasterizerState** ppRasterizerState)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::RSGetViewports(UINT* pNumViewports, D3D11_VIEWPORT* pViewports)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::RSGetScissorRects(UINT* pNumRects, D3D11_RECT* pRects)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::HSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::HSGetShader(ID3D11HullShader** ppHullShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::HSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::HSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::DSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::DSGetShader(ID3D11DomainShader** ppDomainShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::DSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::DSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::CSGetShaderResources(UINT StartSlot, UINT NumViews, ID3D11ShaderResourceView** ppShaderResourceViews)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::CSGetUnorderedAccessViews(UINT StartSlot, UINT NumUAVs, ID3D11UnorderedAccessView** ppUnorderedAccessViews)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::CSGetShader(ID3D11ComputeShader** ppComputeShader, ID3D11ClassInstance** ppClassInstances, UINT* pNumClassInstances)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::CSGetSamplers(UINT StartSlot, UINT NumSamplers, ID3D11SamplerState** ppSamplers)
{
}

void STDMETHODCALLTYPE MarsDeviceContext::CSGetConstantBuffers(UINT StartSlot, UINT NumBuffers, ID3D11Buffer** ppConstantBuffers)
{
}
