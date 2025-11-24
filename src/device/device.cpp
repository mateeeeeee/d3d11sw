#include <new>
#include "device.h"
#include "context/context.h"
#include "resources/buffer.h"
#include "resources/texture1d.h"
#include "resources/texture2d.h"
#include "resources/texture3d.h"
#include "views/render_target_view.h"
#include "views/depth_stencil_view.h"
#include "views/shader_resource_view.h"
#include "views/unordered_access_view.h"
#include "views/view_util.h"
#include "misc/input_layout.h"
#include "shaders/vertex_shader.h"
#include "shaders/pixel_shader.h"
#include "shaders/compute_shader.h"
#include "states/blend_state.h"
#include "states/depth_stencil_state.h"
#include "states/rasterizer_state.h"
#include "states/sampler_state.h"
#include "util/format.h"

namespace d3d11sw {


template<typename T, typename... ArgsT>
HRESULT D3D11DeviceSW::CreateAndInit(T** ppOut, ArgsT&&... args)
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

static UINT GetFormatSupport(DXGI_FORMAT format)
{
    static constexpr UINT kColor =
        D3D11_FORMAT_SUPPORT_BUFFER            |
        D3D11_FORMAT_SUPPORT_TEXTURE1D         |
        D3D11_FORMAT_SUPPORT_TEXTURE2D         |
        D3D11_FORMAT_SUPPORT_TEXTURE3D         |
        D3D11_FORMAT_SUPPORT_TEXTURECUBE       |
        D3D11_FORMAT_SUPPORT_SHADER_LOAD       |
        D3D11_FORMAT_SUPPORT_SHADER_SAMPLE     |
        D3D11_FORMAT_SUPPORT_SHADER_GATHER     |
        D3D11_FORMAT_SUPPORT_MIP               |
        D3D11_FORMAT_SUPPORT_MIP_AUTOGEN       |
        D3D11_FORMAT_SUPPORT_RENDER_TARGET     |
        D3D11_FORMAT_SUPPORT_BLENDABLE         |
        D3D11_FORMAT_SUPPORT_CPU_LOCKABLE;

    static constexpr UINT kDepth =
        D3D11_FORMAT_SUPPORT_TEXTURE2D     |
        D3D11_FORMAT_SUPPORT_DEPTH_STENCIL |
        D3D11_FORMAT_SUPPORT_CPU_LOCKABLE;

    static constexpr UINT kBC =
        D3D11_FORMAT_SUPPORT_TEXTURE1D     |
        D3D11_FORMAT_SUPPORT_TEXTURE2D     |
        D3D11_FORMAT_SUPPORT_TEXTURE3D     |
        D3D11_FORMAT_SUPPORT_TEXTURECUBE   |
        D3D11_FORMAT_SUPPORT_SHADER_LOAD   |
        D3D11_FORMAT_SUPPORT_SHADER_SAMPLE |
        D3D11_FORMAT_SUPPORT_SHADER_GATHER |
        D3D11_FORMAT_SUPPORT_MIP           |
        D3D11_FORMAT_SUPPORT_CPU_LOCKABLE;

    static constexpr UINT kTypeless =
        D3D11_FORMAT_SUPPORT_TEXTURE1D              |
        D3D11_FORMAT_SUPPORT_TEXTURE2D              |
        D3D11_FORMAT_SUPPORT_TEXTURE3D              |
        D3D11_FORMAT_SUPPORT_CPU_LOCKABLE           |
        D3D11_FORMAT_SUPPORT_CAST_WITHIN_BIT_LAYOUT;

    switch (format)
    {
    case DXGI_FORMAT_UNKNOWN: return 0;

    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
        return kDepth;

    case DXGI_FORMAT_BC1_UNORM:     case DXGI_FORMAT_BC1_UNORM_SRGB:
    case DXGI_FORMAT_BC2_UNORM:     case DXGI_FORMAT_BC2_UNORM_SRGB:
    case DXGI_FORMAT_BC3_UNORM:     case DXGI_FORMAT_BC3_UNORM_SRGB:
    case DXGI_FORMAT_BC4_UNORM:     case DXGI_FORMAT_BC4_SNORM:
    case DXGI_FORMAT_BC5_UNORM:     case DXGI_FORMAT_BC5_SNORM:
    case DXGI_FORMAT_BC6H_UF16:     case DXGI_FORMAT_BC6H_SF16:
    case DXGI_FORMAT_BC7_UNORM:     case DXGI_FORMAT_BC7_UNORM_SRGB:
        return kBC;
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_R16_TYPELESS:
        return kTypeless;
    default:
        return kColor;
    }
}

static HRESULT ValidateTextureBindFlags(DXGI_FORMAT format, UINT bindFlags, Bool depthStencilAllowed)
{
    const Bool isDepth = IsDepthFormat(format);
    const Bool isBC    = IsBlockCompressedFormat(format);
    
    if (!depthStencilAllowed && (bindFlags & D3D11_BIND_DEPTH_STENCIL))
    {
        return E_INVALIDARG;
    }

    if (isDepth && (bindFlags & D3D11_BIND_RENDER_TARGET))
    {
        return E_INVALIDARG;
    }

    if (!isDepth && (bindFlags & D3D11_BIND_DEPTH_STENCIL))
    {
        return E_INVALIDARG;
    }

    if (isBC && (bindFlags & (D3D11_BIND_RENDER_TARGET | D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_UNORDERED_ACCESS)))
    {
        return E_INVALIDARG;
    }

    return S_OK;
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

    if (pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL)
    {
        return E_INVALIDARG;
    }

    if ((pDesc->MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) && pDesc->StructureByteStride == 0)
    {
        return E_INVALIDARG;
    }

    D3D11BufferSW* buf = nullptr;
    HRESULT hr = CreateAndInit(&buf, pDesc, pInitialData);
    if (FAILED(hr))
    {
        return hr;
    }

    //https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11device-createbuffer
    //[out, optional] ppBuffer: Set this parameter to NULL to validate the other input parameters (S_FALSE indicates a pass).
    if (!ppBuffer)
    {
        buf->Release();
        return S_FALSE;
    }

    *ppBuffer = buf;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateTexture1D(
    const D3D11_TEXTURE1D_DESC* pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData,
    ID3D11Texture1D** ppTexture1D)
{
    if (!pDesc)
    {
        return E_INVALIDARG;
    }

    if (pDesc->Format == DXGI_FORMAT_UNKNOWN)
    {
        return E_INVALIDARG;
    }

    HRESULT flagsHr = ValidateTextureBindFlags(pDesc->Format, pDesc->BindFlags, false);
    if (FAILED(flagsHr))
    {
        return flagsHr;
    }

    //https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11device-createtexture1d
    //Applications can't specify NULL for pInitialData when creating IMMUTABLE resources
    if (pDesc->Usage == D3D11_USAGE_IMMUTABLE && !pInitialData)
    {
        return E_INVALIDARG;
    }

    D3D11Texture1DSW* tex = nullptr;
    HRESULT hr = CreateAndInit(&tex, pDesc, pInitialData);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppTexture1D)
    {
        tex->Release();
        return S_FALSE;
    }
    
    *ppTexture1D = tex;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateTexture2D(
    const D3D11_TEXTURE2D_DESC*   pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData,
    ID3D11Texture2D**             ppTexture2D)
{
    if (!pDesc)
    {
        return E_INVALIDARG;
    }

    if (pDesc->Format == DXGI_FORMAT_UNKNOWN)
    {
        return E_INVALIDARG;
    }

    HRESULT flagsHr = ValidateTextureBindFlags(pDesc->Format, pDesc->BindFlags, /*depthStencilAllowed=*/true);
    if (FAILED(flagsHr))
    {
        return flagsHr;
    }

    //https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11device-createtexture2d
    //Applications can't specify NULL for pInitialData when creating IMMUTABLE resources
    if (pDesc->Usage == D3D11_USAGE_IMMUTABLE && !pInitialData)
    {
        return E_INVALIDARG;
    }

    //Multisampled resources cannot be initialized with data when they are created
    if (pDesc->SampleDesc.Count > 1 && pInitialData)
    {
        return E_INVALIDARG;
    }

    D3D11_TEXTURE2D_DESC1 desc1 = {};
    std::memcpy(&desc1, pDesc, sizeof(D3D11_TEXTURE2D_DESC));
    desc1.TextureLayout  = D3D11_TEXTURE_LAYOUT_UNDEFINED;

    D3D11Texture2DSW* tex = nullptr;
    HRESULT hr = CreateAndInit(&tex, &desc1, pInitialData);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppTexture2D)
    {
        tex->Release();
        return S_FALSE;
    }

    *ppTexture2D = tex;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateTexture3D(
    const D3D11_TEXTURE3D_DESC*   pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData,
    ID3D11Texture3D**             ppTexture3D)
{
    if (!pDesc)
    {
        return E_INVALIDARG;
    }

    if (pDesc->Format == DXGI_FORMAT_UNKNOWN)
    {
        return E_INVALIDARG;
    }

    HRESULT flagsHr = ValidateTextureBindFlags(pDesc->Format, pDesc->BindFlags, /*depthStencilAllowed=*/false);
    if (FAILED(flagsHr))
    {
        return flagsHr;
    }

    //https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11device-createtexture3d
    //Applications cannot specify NULL for pInitialData when creating IMMUTABLE resources
    if (pDesc->Usage == D3D11_USAGE_IMMUTABLE && !pInitialData)
    {
        return E_INVALIDARG;
    }

    D3D11_TEXTURE3D_DESC1 desc1 = {};
    std::memcpy(&desc1, pDesc, sizeof(D3D11_TEXTURE3D_DESC));
    desc1.TextureLayout = D3D11_TEXTURE_LAYOUT_UNDEFINED;

    D3D11Texture3DSW* tex = nullptr;
    HRESULT hr = CreateAndInit(&tex, &desc1, pInitialData);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppTexture3D)
    {
        tex->Release();
        return S_FALSE;
    }

    *ppTexture3D = tex;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateShaderResourceView(
    ID3D11Resource* pResource,
    const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc,
    ID3D11ShaderResourceView** ppSRView)
{
    if (!pResource)
    {
        return E_INVALIDARG;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC1 desc1{};
    if (pDesc)
    {
        std::memcpy(&desc1, pDesc, sizeof(*pDesc));
    }

    D3D11ShaderResourceViewSW* view = nullptr;
    HRESULT hr = CreateAndInit(&view, pResource, pDesc ? &desc1 : nullptr);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppSRView)
    {
        view->Release();
        return S_FALSE;
    }

    *ppSRView = view;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateUnorderedAccessView(
    ID3D11Resource* pResource,
    const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc,
    ID3D11UnorderedAccessView** ppUAView)
{
    if (!pResource)
    {
        return E_INVALIDARG;
    }

    D3D11_UNORDERED_ACCESS_VIEW_DESC1 desc1{};
    if (pDesc)
    {
        std::memcpy(&desc1, pDesc, sizeof(*pDesc));
    }

    D3D11UnorderedAccessViewSW* view = nullptr;
    HRESULT hr = CreateAndInit(&view, pResource, pDesc ? &desc1 : nullptr);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppUAView)
    {
        view->Release();
        return S_FALSE;
    }

    *ppUAView = view;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateRenderTargetView(
    ID3D11Resource* pResource,
    const D3D11_RENDER_TARGET_VIEW_DESC* pDesc,
    ID3D11RenderTargetView** ppRTView)
{
    if (!pResource)
    {
        return E_INVALIDARG;
    }

    D3D11_RENDER_TARGET_VIEW_DESC1 desc1{};
    if (pDesc)
    {
        std::memcpy(&desc1, pDesc, sizeof(*pDesc));
    }

    D3D11RenderTargetViewSW* view = nullptr;
    HRESULT hr = CreateAndInit(&view, pResource, pDesc ? &desc1 : nullptr);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppRTView)
    {
        view->Release();
        return S_FALSE;
    }

    *ppRTView = view;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateDepthStencilView(
    ID3D11Resource* pResource,
    const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc,
    ID3D11DepthStencilView** ppDepthStencilView)
{
    if (!pResource)
    {
        return E_INVALIDARG;
    }

    D3D11DepthStencilViewSW* view = nullptr;
    HRESULT hr = CreateAndInit(&view, pResource, pDesc);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppDepthStencilView)
    {
        view->Release();
        return S_FALSE;
    }

    *ppDepthStencilView = view;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateInputLayout(
    const D3D11_INPUT_ELEMENT_DESC* pInputElementDescs,
    UINT NumElements,
    const void* pShaderBytecodeWithInputSignature,
    SIZE_T BytecodeLength,
    ID3D11InputLayout** ppInputLayout)
{
    if (!pInputElementDescs || NumElements == 0)
    {
        return E_INVALIDARG;
    }

    D3D11InputLayoutSW* layout = nullptr;
    HRESULT hr = CreateAndInit(&layout, pInputElementDescs, NumElements);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppInputLayout)
    {
        layout->Release();
        return S_FALSE;
    }

    *ppInputLayout = layout;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateVertexShader(
    const void* pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D11ClassLinkage* pClassLinkage,
    ID3D11VertexShader** ppVertexShader)
{
    D3D11VertexShaderSW* shader = nullptr;
    HRESULT hr = CreateAndInit(&shader, pShaderBytecode, BytecodeLength);
    if (FAILED(hr))
    {
        return hr;
    }
    if (!ppVertexShader)
    {
        shader->Release();
        return S_FALSE;
    }
    *ppVertexShader = shader;
    return S_OK;
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
    D3D11PixelShaderSW* shader = nullptr;
    HRESULT hr = CreateAndInit(&shader, pShaderBytecode, BytecodeLength);
    if (FAILED(hr))
    {
        return hr;
    }
    if (!ppPixelShader)
    {
        shader->Release();
        return S_FALSE;
    }
    *ppPixelShader = shader;
    return S_OK;
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
    D3D11ComputeShaderSW* shader = nullptr;
    HRESULT hr = CreateAndInit(&shader, pShaderBytecode, BytecodeLength);
    if (FAILED(hr))
    {
        return hr;
    }
    if (!ppComputeShader)
    {
        shader->Release();
        return S_FALSE;
    }
    *ppComputeShader = shader;
    return S_OK;
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
    if (!pBlendStateDesc)
    {
        return E_INVALIDARG;
    }

    D3D11_BLEND_DESC1 desc1{};
    desc1.AlphaToCoverageEnable  = pBlendStateDesc->AlphaToCoverageEnable;
    desc1.IndependentBlendEnable = pBlendStateDesc->IndependentBlendEnable;
    for (Int i = 0; i < 8; i++)
    {
        const auto& src = pBlendStateDesc->RenderTarget[i];
        auto&       dst = desc1.RenderTarget[i];
        dst.BlendEnable           = src.BlendEnable;
        dst.SrcBlend              = src.SrcBlend;
        dst.DestBlend             = src.DestBlend;
        dst.BlendOp               = src.BlendOp;
        dst.SrcBlendAlpha         = src.SrcBlendAlpha;
        dst.DestBlendAlpha        = src.DestBlendAlpha;
        dst.BlendOpAlpha          = src.BlendOpAlpha;
        dst.RenderTargetWriteMask = src.RenderTargetWriteMask;
    }

    D3D11BlendStateSW* bs = nullptr;
    HRESULT hr = CreateAndInit(&bs, &desc1);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppBlendState)
    {
        bs->Release();
        return S_FALSE;
    }

    *ppBlendState = bs;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateDepthStencilState(
    const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc,
    ID3D11DepthStencilState** ppDepthStencilState)
{
    if (!pDepthStencilDesc)
    {
        return E_INVALIDARG;
    }

    D3D11DepthStencilStateSW* dss = nullptr;
    HRESULT hr = CreateAndInit(&dss, pDepthStencilDesc);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppDepthStencilState)
    {
        dss->Release();
        return S_FALSE;
    }

    *ppDepthStencilState = dss;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateRasterizerState(
    const D3D11_RASTERIZER_DESC* pRasterizerDesc,
    ID3D11RasterizerState** ppRasterizerState)
{
    if (!pRasterizerDesc)
    {
        return E_INVALIDARG;
    }

    D3D11_RASTERIZER_DESC2 desc2{};
    std::memcpy(&desc2, pRasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

    D3D11RasterizerStateSW* rs = nullptr;
    HRESULT hr = CreateAndInit(&rs, &desc2);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppRasterizerState)
    {
        rs->Release();
        return S_FALSE;
    }

    *ppRasterizerState = rs;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateSamplerState(
    const D3D11_SAMPLER_DESC* pSamplerDesc,
    ID3D11SamplerState** ppSamplerState)
{
    if (!pSamplerDesc)
    {
        return E_INVALIDARG;
    }

    D3D11SamplerStateSW* ss = nullptr;
    HRESULT hr = CreateAndInit(&ss, pSamplerDesc);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppSamplerState)
    {
        ss->Release();
        return S_FALSE;
    }

    *ppSamplerState = ss;
    return S_OK;
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
    if (!pFormatSupport)
    {
        return E_INVALIDARG;
    }
    *pFormatSupport = GetFormatSupport(Format);
    return S_OK;
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
    if (!pBlendStateDesc)
    {
        return E_INVALIDARG;
    }

    D3D11BlendStateSW* bs = nullptr;
    HRESULT hr = CreateAndInit(&bs, pBlendStateDesc);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppBlendState)
    {
        bs->Release();
        return S_FALSE;
    }

    *ppBlendState = bs;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateRasterizerState1(const D3D11_RASTERIZER_DESC1* pRasterizerDesc, ID3D11RasterizerState1** ppRasterizerState)
{
    if (!pRasterizerDesc)
    {
        return E_INVALIDARG;
    }

    D3D11_RASTERIZER_DESC2 desc2{};
    std::memcpy(&desc2, pRasterizerDesc, sizeof(D3D11_RASTERIZER_DESC1));

    D3D11RasterizerStateSW* rs = nullptr;
    HRESULT hr = CreateAndInit(&rs, &desc2);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppRasterizerState)
    {
        rs->Release();
        return S_FALSE;
    }

    *ppRasterizerState = rs;
    return S_OK;
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
    if (!pDesc)
    {
        return E_INVALIDARG;
    }

    if (pDesc->Format == DXGI_FORMAT_UNKNOWN)
    {
        return E_INVALIDARG;
    }

    HRESULT flagsHr = ValidateTextureBindFlags(pDesc->Format, pDesc->BindFlags, /*depthStencilAllowed=*/true);
    if (FAILED(flagsHr))
    {
        return flagsHr;
    }

    if (pDesc->Usage == D3D11_USAGE_IMMUTABLE && !pInitialData)
    {
        return E_INVALIDARG;
    }

    //Multisampled resources cannot be initialized with data when they are created
    if (pDesc->SampleDesc.Count > 1 && pInitialData)
    {
        return E_INVALIDARG;
    }

    D3D11Texture2DSW* tex = nullptr;
    HRESULT hr = CreateAndInit(&tex, pDesc, pInitialData);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppTexture2D)
    {
        tex->Release();
        return S_FALSE;
    }

    *ppTexture2D = tex;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateTexture3D1(const D3D11_TEXTURE3D_DESC1* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture3D1** ppTexture3D)
{
    if (!pDesc)
    {
        return E_INVALIDARG;
    }

    if (pDesc->Format == DXGI_FORMAT_UNKNOWN)
    {
        return E_INVALIDARG;
    }

    HRESULT flagsHr = ValidateTextureBindFlags(pDesc->Format, pDesc->BindFlags, /*depthStencilAllowed=*/false);
    if (FAILED(flagsHr))
    {
        return flagsHr;
    }

    if (pDesc->Usage == D3D11_USAGE_IMMUTABLE && !pInitialData)
    {
        return E_INVALIDARG;
    }

    D3D11Texture3DSW* tex = nullptr;
    HRESULT hr = CreateAndInit(&tex, pDesc, pInitialData);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppTexture3D)
    {
        tex->Release();
        return S_FALSE;
    }

    *ppTexture3D = tex;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateRasterizerState2(const D3D11_RASTERIZER_DESC2* pRasterizerDesc, ID3D11RasterizerState2** ppRasterizerState)
{
    if (!pRasterizerDesc)
    {
        return E_INVALIDARG;
    }

    D3D11RasterizerStateSW* rs = nullptr;
    HRESULT hr = CreateAndInit(&rs, pRasterizerDesc);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppRasterizerState)
    {
        rs->Release();
        return S_FALSE;
    }

    *ppRasterizerState = rs;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateShaderResourceView1(ID3D11Resource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC1* pDesc, ID3D11ShaderResourceView1** ppSRView)
{
    if (!pResource)
    {
        return E_INVALIDARG;
    }

    D3D11ShaderResourceViewSW* view = nullptr;
    HRESULT hr = CreateAndInit(&view, pResource, pDesc);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppSRView)
    {
        view->Release();
        return S_FALSE;
    }

    *ppSRView = view;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateUnorderedAccessView1(ID3D11Resource* pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC1* pDesc, ID3D11UnorderedAccessView1** ppUAView)
{
    if (!pResource)
    {
        return E_INVALIDARG;
    }

    D3D11UnorderedAccessViewSW* view = nullptr;
    HRESULT hr = CreateAndInit(&view, pResource, pDesc);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppUAView)
    {
        view->Release();
        return S_FALSE;
    }

    *ppUAView = view;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE D3D11DeviceSW::CreateRenderTargetView1(ID3D11Resource* pResource, const D3D11_RENDER_TARGET_VIEW_DESC1* pDesc, ID3D11RenderTargetView1** ppRTView)
{
    if (!pResource)
    {
        return E_INVALIDARG;
    }

    D3D11RenderTargetViewSW* view = nullptr;
    HRESULT hr = CreateAndInit(&view, pResource, pDesc);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppRTView)
    {
        view->Release();
        return S_FALSE;
    }

    *ppRTView = view;
    return S_OK;
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
