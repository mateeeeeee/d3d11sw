#include "device.h"
#include "context/context.h"

MarsDevice::MarsDevice()
{
}

MarsDevice::~MarsDevice()
{
}

void MarsDevice::SetImmediateContext(MarsDeviceContext* ctx)
{
    m_immediateContext = ctx;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateBuffer(
    const D3D11_BUFFER_DESC* pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData,
    ID3D11Buffer** ppBuffer)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateTexture1D(
    const D3D11_TEXTURE1D_DESC* pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData,
    ID3D11Texture1D** ppTexture1D)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateTexture2D(
    const D3D11_TEXTURE2D_DESC* pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData,
    ID3D11Texture2D** ppTexture2D)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateTexture3D(
    const D3D11_TEXTURE3D_DESC* pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData,
    ID3D11Texture3D** ppTexture3D)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateShaderResourceView(
    ID3D11Resource* pResource,
    const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc,
    ID3D11ShaderResourceView** ppSRView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateUnorderedAccessView(
    ID3D11Resource* pResource,
    const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc,
    ID3D11UnorderedAccessView** ppUAView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateRenderTargetView(
    ID3D11Resource* pResource,
    const D3D11_RENDER_TARGET_VIEW_DESC* pDesc,
    ID3D11RenderTargetView** ppRTView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateDepthStencilView(
    ID3D11Resource* pResource,
    const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc,
    ID3D11DepthStencilView** ppDepthStencilView)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateInputLayout(
    const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs,
    UINT NumElements,
    const void* pShaderBytecodeWithInputSignature,
    SIZE_T BytecodeLength,
    ID3D11InputLayout** ppInputLayout)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateVertexShader(
    const void* pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D11ClassLinkage* pClassLinkage,
    ID3D11VertexShader** ppVertexShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateGeometryShader(
    const void* pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D11ClassLinkage* pClassLinkage,
    ID3D11GeometryShader** ppGeometryShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateGeometryShaderWithStreamOutput(
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

HRESULT STDMETHODCALLTYPE MarsDevice::CreatePixelShader(
    const void* pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D11ClassLinkage* pClassLinkage,
    ID3D11PixelShader** ppPixelShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateHullShader(
    const void* pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D11ClassLinkage* pClassLinkage,
    ID3D11HullShader** ppHullShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateDomainShader(
    const void* pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D11ClassLinkage* pClassLinkage,
    ID3D11DomainShader** ppDomainShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateComputeShader(
    const void* pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D11ClassLinkage* pClassLinkage,
    ID3D11ComputeShader** ppComputeShader)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateClassLinkage(
    ID3D11ClassLinkage** ppLinkage)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateBlendState(
    const D3D11_BLEND_DESC* pBlendStateDesc,
    ID3D11BlendState** ppBlendState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateDepthStencilState(
    const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc,
    ID3D11DepthStencilState** ppDepthStencilState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateRasterizerState(
    const D3D11_RASTERIZER_DESC* pRasterizerDesc,
    ID3D11RasterizerState** ppRasterizerState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateSamplerState(
    const D3D11_SAMPLER_DESC* pSamplerDesc,
    ID3D11SamplerState** ppSamplerState)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateQuery(
    const D3D11_QUERY_DESC* pQueryDesc,
    ID3D11Query** ppQuery)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreatePredicate(
    const D3D11_QUERY_DESC* pPredicateDesc,
    ID3D11Predicate** ppPredicate)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateCounter(
    const D3D11_COUNTER_DESC* pCounterDesc,
    ID3D11Counter** ppCounter)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CreateDeferredContext(
    UINT ContextFlags,
    ID3D11DeviceContext** ppDeferredContext)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::OpenSharedResource(
    HANDLE hResource,
    REFIID ReturnedInterface,
    void** ppResource)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CheckFormatSupport(
    DXGI_FORMAT Format,
    UINT* pFormatSupport)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::CheckMultisampleQualityLevels(
    DXGI_FORMAT Format,
    UINT SampleCount,
    UINT* pNumQualityLevels)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE MarsDevice::CheckCounterInfo(
    D3D11_COUNTER_INFO* pCounterInfo)
{
}

HRESULT STDMETHODCALLTYPE MarsDevice::CheckCounter(
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

HRESULT STDMETHODCALLTYPE MarsDevice::CheckFeatureSupport(
    D3D11_FEATURE Feature,
    void* pFeatureSupportData,
    UINT FeatureSupportDataSize)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::GetPrivateData(
    REFGUID guid,
    UINT* pDataSize,
    void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::SetPrivateData(
    REFGUID guid,
    UINT DataSize,
    const void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDevice::SetPrivateDataInterface(
    REFGUID guid,
    const IUnknown* pData)
{
    return E_NOTIMPL;
}

D3D_FEATURE_LEVEL STDMETHODCALLTYPE MarsDevice::GetFeatureLevel()
{
    return D3D_FEATURE_LEVEL_11_1;
}

UINT STDMETHODCALLTYPE MarsDevice::GetCreationFlags()
{
    return 0;
}

HRESULT STDMETHODCALLTYPE MarsDevice::GetDeviceRemovedReason()
{
    return S_OK;
}

void STDMETHODCALLTYPE MarsDevice::GetImmediateContext(
    ID3D11DeviceContext** ppImmediateContext)
{
    if (ppImmediateContext && m_immediateContext)
    {
        *ppImmediateContext = static_cast<ID3D11DeviceContext*>(m_immediateContext);
        m_immediateContext->AddRef();
    }
}

HRESULT STDMETHODCALLTYPE MarsDevice::SetExceptionMode(
    UINT RaiseFlags)
{
    return S_OK;
}

UINT STDMETHODCALLTYPE MarsDevice::GetExceptionMode()
{
    return 0;
}
