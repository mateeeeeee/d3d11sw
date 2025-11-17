#include <new>
#include "device.h"
#include "context/context.h"
#include "resources/buffer.h"

namespace d3d11sw {


template<typename T, typename... ArgsT>
HRESULT D3D11DeviceSW::MakeAndInit(T** ppOut, ArgsT&&... args)
{
	try
	{
		T* obj = new T(this);
		HRESULT hr = obj->Init(std::forward<ArgsT>(args)...);
		if (FAILED(hr)) { delete obj; return hr; }
		*ppOut = obj;
		return S_OK;
	}
	catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
	catch (...) { return E_FAIL; }
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(ID3D11Device5))
    {
        *ppv = static_cast<ID3D11Device5*>(this);
    }
    else if (riid == __uuidof(ID3D11Device4))
    {
        *ppv = static_cast<ID3D11Device4*>(this);
    }
    else if (riid == __uuidof(ID3D11Device3))
    {
        *ppv = static_cast<ID3D11Device3*>(this);
    }
    else if (riid == __uuidof(ID3D11Device2))
    {
        *ppv = static_cast<ID3D11Device2*>(this);
    }
    else if (riid == __uuidof(ID3D11Device1))
    {
        *ppv = static_cast<ID3D11Device1*>(this);
    }
    else if (riid == __uuidof(ID3D11Device))
    {
        *ppv = static_cast<ID3D11Device*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

D3D11DeviceSW::D3D11DeviceSW()
{
}

D3D11DeviceSW::~D3D11DeviceSW()
{
}

void D3D11DeviceSW::SetImmediateContext(D3D11DeviceContextSW* ctx)
{
    _immediateContext = ctx;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateBuffer(
    const D3D11_BUFFER_DESC*      pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData,
    ID3D11Buffer**                ppBuffer)
{
    if (!pDesc)
    {
        return E_INVALIDARG;
    }

    if (!ppBuffer)
    {
        //https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11device-createbuffer
        //[out, optional] ppBuffer
        //Type: ID3D11Buffer**
        //Address of a pointer to the ID3D11Buffer interface for the buffer object created. Set this parameter to NULL to validate the other input parameters (S_FALSE indicates a pass).
        return S_FALSE;
    }
    *ppBuffer = nullptr;

    D3D11BufferSW* buf = nullptr;
    HRESULT hr = MakeAndInit(&buf, pDesc, pInitialData);
    if (FAILED(hr)) 
    {
        return hr;
    }

    *ppBuffer = buf;
    return S_OK;
}
HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateTexture1D(
    const D3D11_TEXTURE1D_DESC* pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData,
    ID3D11Texture1D** ppTexture1D)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateTexture2D(
    const D3D11_TEXTURE2D_DESC* pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData,
    ID3D11Texture2D** ppTexture2D)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateTexture3D(
    const D3D11_TEXTURE3D_DESC* pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData,
    ID3D11Texture3D** ppTexture3D)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateShaderResourceView(
    ID3D11Resource* pResource,
    const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc,
    ID3D11ShaderResourceView** ppSRView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateUnorderedAccessView(
    ID3D11Resource* pResource,
    const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc,
    ID3D11UnorderedAccessView** ppUAView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateRenderTargetView(
    ID3D11Resource* pResource,
    const D3D11_RENDER_TARGET_VIEW_DESC* pDesc,
    ID3D11RenderTargetView** ppRTView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateDepthStencilView(
    ID3D11Resource* pResource,
    const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc,
    ID3D11DepthStencilView** ppDepthStencilView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateInputLayout(
    const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs,
    UINT NumElements,
    const void* pShaderBytecodeWithInputSignature,
    SIZE_T BytecodeLength,
    ID3D11InputLayout** ppInputLayout)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateVertexShader(
    const void* pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D11ClassLinkage* pClassLinkage,
    ID3D11VertexShader** ppVertexShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateGeometryShader(
    const void* pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D11ClassLinkage* pClassLinkage,
    ID3D11GeometryShader** ppGeometryShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateGeometryShaderWithStreamOutput(
    const void* pShaderBytecode,
    SIZE_T BytecodeLength,
    const D3D11_SO_DECLARATION_ENTRY* pSODeclaration,
    UINT NumEntries,
    const UINT* pBufferStrides,
    UINT NumStrides,
    UINT RasterizedStream,
    ID3D11ClassLinkage* pClassLinkage,
    ID3D11GeometryShader** ppGeometryShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreatePixelShader(
    const void* pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D11ClassLinkage* pClassLinkage,
    ID3D11PixelShader** ppPixelShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateHullShader(
    const void* pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D11ClassLinkage* pClassLinkage,
    ID3D11HullShader** ppHullShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateDomainShader(
    const void* pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D11ClassLinkage* pClassLinkage,
    ID3D11DomainShader** ppDomainShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateComputeShader(
    const void* pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D11ClassLinkage* pClassLinkage,
    ID3D11ComputeShader** ppComputeShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateClassLinkage(
    ID3D11ClassLinkage** ppLinkage)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateBlendState(
    const D3D11_BLEND_DESC* pBlendStateDesc,
    ID3D11BlendState** ppBlendState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateDepthStencilState(
    const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc,
    ID3D11DepthStencilState** ppDepthStencilState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateRasterizerState(
    const D3D11_RASTERIZER_DESC* pRasterizerDesc,
    ID3D11RasterizerState** ppRasterizerState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateSamplerState(
    const D3D11_SAMPLER_DESC* pSamplerDesc,
    ID3D11SamplerState** ppSamplerState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateQuery(
    const D3D11_QUERY_DESC* pQueryDesc,
    ID3D11Query** ppQuery)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreatePredicate(
    const D3D11_QUERY_DESC* pPredicateDesc,
    ID3D11Predicate** ppPredicate)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateCounter(
    const D3D11_COUNTER_DESC* pCounterDesc,
    ID3D11Counter** ppCounter)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateDeferredContext(
    UINT ContextFlags,
    ID3D11DeviceContext** ppDeferredContext)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::OpenSharedResource(
    HANDLE hResource,
    REFIID ReturnedInterface,
    void** ppResource)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CheckFormatSupport(
    DXGI_FORMAT Format,
    UINT* pFormatSupport)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CheckMultisampleQualityLevels(
    DXGI_FORMAT Format,
    UINT SampleCount,
    UINT* pNumQualityLevels)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE D3D11DeviceSW::CheckCounterInfo(
    D3D11_COUNTER_INFO* pCounterInfo)
{
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CheckCounter(
    const D3D11_COUNTER_DESC* pDesc,
    D3D11_COUNTER_TYPE* pType,
    UINT* pActiveCounters,
    LPSTR szName,
    UINT* pNameLength,
    LPSTR szUnits,
    UINT* pUnitsLength,
    LPSTR szDescription,
    UINT* pDescriptionLength)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CheckFeatureSupport(
    D3D11_FEATURE Feature,
    void* pFeatureSupportData,
    UINT FeatureSupportDataSize)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::GetPrivateData(
    REFGUID guid,
    UINT* pDataSize,
    void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::SetPrivateData(
    REFGUID guid,
    UINT DataSize,
    const void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::SetPrivateDataInterface(
    REFGUID guid,
    const IUnknown* pData)
{
    return E_NOTIMPL;
}

D3D_FEATURE_LEVEL STDMETHODCALLTYPE D3D11DeviceSW::GetFeatureLevel()
{
    return D3D_FEATURE_LEVEL_11_1;
}

UINT STDMETHODCALLTYPE D3D11DeviceSW::GetCreationFlags()
{
    return 0;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::GetDeviceRemovedReason()
{
    return S_OK;
}

void STDMETHODCALLTYPE D3D11DeviceSW::GetImmediateContext(
    ID3D11DeviceContext** ppImmediateContext)
{
    if (ppImmediateContext && _immediateContext)
    {
        *ppImmediateContext = static_cast<ID3D11DeviceContext*>(_immediateContext);
        _immediateContext->AddRef();
    }
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::SetExceptionMode(
    UINT RaiseFlags)
{
    return S_OK;
}

UINT STDMETHODCALLTYPE D3D11DeviceSW::GetExceptionMode()
{
    return 0;
}

// ---- ID3D11Device1 ----

void STDMETHODCALLTYPE D3D11DeviceSW::GetImmediateContext1(ID3D11DeviceContext1** ppImmediateContext)
{
    if (ppImmediateContext && _immediateContext)
    {
        *ppImmediateContext = static_cast<ID3D11DeviceContext1*>(_immediateContext);
        _immediateContext->AddRef();
    }
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateDeferredContext1(UINT ContextFlags, ID3D11DeviceContext1** ppDeferredContext)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateBlendState1(const D3D11_BLEND_DESC1* pBlendStateDesc, ID3D11BlendState1** ppBlendState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateRasterizerState1(const D3D11_RASTERIZER_DESC1* pRasterizerDesc, ID3D11RasterizerState1** ppRasterizerState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateDeviceContextState(UINT Flags, const D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, REFIID EmulatedInterface, D3D_FEATURE_LEVEL* pChosenFeatureLevel, ID3DDeviceContextState** ppContextState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::OpenSharedResource1(HANDLE hResource, REFIID returnedInterface, void** ppResource)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::OpenSharedResourceByName(LPCWSTR lpName, DWORD dwDesiredAccess, REFIID returnedInterface, void** ppResource)
{
    return E_NOTIMPL;
}

// ---- ID3D11Device2 ----

void STDMETHODCALLTYPE D3D11DeviceSW::GetImmediateContext2(ID3D11DeviceContext2** ppImmediateContext)
{
    if (ppImmediateContext && _immediateContext)
    {
        *ppImmediateContext = static_cast<ID3D11DeviceContext2*>(_immediateContext);
        _immediateContext->AddRef();
    }
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateDeferredContext2(UINT ContextFlags, ID3D11DeviceContext2** ppDeferredContext)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE D3D11DeviceSW::GetResourceTiling(ID3D11Resource* pTiledResource, UINT* pNumTilesForEntireResource, D3D11_PACKED_MIP_DESC* pPackedMipDesc, D3D11_TILE_SHAPE* pStandardTileShapeForNonPackedMips, UINT* pNumSubresourceTilings, UINT FirstSubresourceTilingToGet, D3D11_SUBRESOURCE_TILING* pSubresourceTilingsForNonPackedMips)
{
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CheckMultisampleQualityLevels1(DXGI_FORMAT Format, UINT SampleCount, UINT Flags, UINT* pNumQualityLevels)
{
    return E_NOTIMPL;
}

// ---- ID3D11Device3 ----

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateTexture2D1(const D3D11_TEXTURE2D_DESC1* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D1** ppTexture2D)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateTexture3D1(const D3D11_TEXTURE3D_DESC1* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture3D1** ppTexture3D)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateRasterizerState2(const D3D11_RASTERIZER_DESC2* pRasterizerDesc, ID3D11RasterizerState2** ppRasterizerState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateShaderResourceView1(ID3D11Resource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC1* pDesc, ID3D11ShaderResourceView1** ppSRView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateUnorderedAccessView1(ID3D11Resource* pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC1* pDesc, ID3D11UnorderedAccessView1** ppUAView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateRenderTargetView1(ID3D11Resource* pResource, const D3D11_RENDER_TARGET_VIEW_DESC1* pDesc, ID3D11RenderTargetView1** ppRTView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateQuery1(const D3D11_QUERY_DESC1* pQueryDesc, ID3D11Query1** ppQuery)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE D3D11DeviceSW::GetImmediateContext3(ID3D11DeviceContext3** ppImmediateContext)
{
    if (ppImmediateContext && _immediateContext)
    {
        *ppImmediateContext = static_cast<ID3D11DeviceContext3*>(_immediateContext);
        _immediateContext->AddRef();
    }
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateDeferredContext3(UINT ContextFlags, ID3D11DeviceContext3** ppDeferredContext)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE D3D11DeviceSW::WriteToSubresource(ID3D11Resource* pDstResource, UINT DstSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch)
{
}

void STDMETHODCALLTYPE D3D11DeviceSW::ReadFromSubresource(void* pDstData, UINT DstRowPitch, UINT DstDepthPitch, ID3D11Resource* pSrcResource, UINT SrcSubresource, const D3D11_BOX* pSrcBox)
{
}

// ---- ID3D11Device4 ----

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::RegisterDeviceRemovedEvent(HANDLE hEvent, DWORD* pdwCookie)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE D3D11DeviceSW::UnregisterDeviceRemoved(DWORD dwCookie)
{
}

// ---- ID3D11Device5 ----

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::OpenSharedFence(HANDLE hFence, REFIID ReturnedInterface, void** ppFence)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateFence(UINT64 InitialValue, D3D11_FENCE_FLAG Flags, REFIID ReturnedInterface, void** ppFence)
{
    return E_NOTIMPL;
}

}
