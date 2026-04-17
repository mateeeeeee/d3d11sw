#include <new>
#include "device.h"
#include "common/log.h"
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
#include "shaders/geometry_shader.h"
#include "shaders/compute_shader.h"
#include "states/blend_state.h"
#include "states/depth_stencil_state.h"
#include "states/rasterizer_state.h"
#include "states/sampler_state.h"
#include "misc/query.h"
#include "util/format.h"
#include "context/context_util.h"
#include "dxgi/device.h"

namespace d3d11sw {


template<Bool IsDebug>
template<typename T, typename... ArgsT>
HRESULT D3D11DeviceSWImpl<IsDebug>::CreateAndInit(T** ppOut, ArgsT&&... args)
{
	try
	{
		T* obj = new(std::nothrow) T(this);
		HRESULT hr = obj->Init(std::forward<ArgsT>(args)...);
		if (FAILED(hr)) { delete obj; return hr; }
		*ppOut = obj;
		return S_OK;
	}
	catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
	catch (...) { return E_FAIL; }
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::QueryInterface(REFIID riid, void** ppv)
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
    else if (riid == __uuidof(IDXGIDevice1) || riid == __uuidof(IDXGIDevice))
    {
        auto* dxgiDevice = new DXGIDeviceSW(this);
        *ppv = static_cast<IDXGIDevice1*>(dxgiDevice);
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

template<Bool IsDebug>
D3D11DeviceSWImpl<IsDebug>::D3D11DeviceSWImpl()
{
}

template<Bool IsDebug>
D3D11DeviceSWImpl<IsDebug>::~D3D11DeviceSWImpl()
{
}

template<Bool IsDebug>
void D3D11DeviceSWImpl<IsDebug>::SetImmediateContext(ID3D11DeviceContext* ctx)
{
    _immediateContext = ctx;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateBuffer(
    const D3D11_BUFFER_DESC*      pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData,
    ID3D11Buffer**                ppBuffer)
{
    if (!pDesc)
    {
        DebugMsg("CreateBuffer: pDesc is null");
        return E_INVALIDARG;
    }

    if (pDesc->ByteWidth == 0)
    {
        DebugMsg("CreateBuffer: ByteWidth is 0");
        return E_INVALIDARG;
    }

    if (pDesc->BindFlags & D3D11_BIND_DEPTH_STENCIL)
    {
        DebugMsg("CreateBuffer: BIND_DEPTH_STENCIL is not valid for buffers");
        return E_INVALIDARG;
    }

    if ((pDesc->BindFlags & D3D11_BIND_CONSTANT_BUFFER) && (pDesc->ByteWidth % 16 != 0))
    {
        DebugMsg("CreateBuffer: constant buffer ByteWidth ({}) must be a multiple of 16", pDesc->ByteWidth);
        return E_INVALIDARG;
    }

    if ((pDesc->MiscFlags & D3D11_RESOURCE_MISC_BUFFER_STRUCTURED) && pDesc->StructureByteStride == 0)
    {
        DebugMsg("CreateBuffer: structured buffer requires StructureByteStride > 0");
        return E_INVALIDARG;
    }

    if (pDesc->Usage == D3D11_USAGE_IMMUTABLE && !pInitialData)
    {
        DebugMsg("CreateBuffer: IMMUTABLE usage requires pInitialData");
        return E_INVALIDARG;
    }

    if (pDesc->Usage == D3D11_USAGE_DEFAULT && pDesc->CPUAccessFlags != 0)
    {
        DebugMsg("CreateBuffer: DEFAULT usage cannot have CPUAccessFlags");
        return E_INVALIDARG;
    }

    if (pDesc->Usage == D3D11_USAGE_IMMUTABLE && pDesc->CPUAccessFlags != 0)
    {
        DebugMsg("CreateBuffer: IMMUTABLE usage cannot have CPUAccessFlags");
        return E_INVALIDARG;
    }

    if (pDesc->Usage == D3D11_USAGE_DYNAMIC && pDesc->CPUAccessFlags != D3D11_CPU_ACCESS_WRITE)
    {
        DebugMsg("CreateBuffer: DYNAMIC usage requires CPUAccessFlags = D3D11_CPU_ACCESS_WRITE");
        return E_INVALIDARG;
    }

    if (pDesc->Usage == D3D11_USAGE_STAGING && pDesc->BindFlags != 0)
    {
        DebugMsg("CreateBuffer: STAGING usage cannot be bound to the pipeline (BindFlags must be 0)");
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateTexture1D(
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateTexture2D(
    const D3D11_TEXTURE2D_DESC*   pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData,
    ID3D11Texture2D**             ppTexture2D)
{
    if (!pDesc)
    {
        DebugMsg("CreateTexture2D: pDesc is null");
        return E_INVALIDARG;
    }

    if (pDesc->Width == 0 || pDesc->Width > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)
    {
        DebugMsg("CreateTexture2D: Width ({}) must be in range [1, {}]", pDesc->Width, D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION);
        return E_INVALIDARG;
    }

    if (pDesc->Height == 0 || pDesc->Height > D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION)
    {
        DebugMsg("CreateTexture2D: Height ({}) must be in range [1, {}]", pDesc->Height, D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION);
        return E_INVALIDARG;
    }

    if (pDesc->ArraySize == 0 || pDesc->ArraySize > D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION)
    {
        DebugMsg("CreateTexture2D: ArraySize ({}) must be in range [1, {}]", pDesc->ArraySize, D3D11_REQ_TEXTURE2D_ARRAY_AXIS_DIMENSION);
        return E_INVALIDARG;
    }

    if (pDesc->MiscFlags & D3D11_RESOURCE_MISC_TEXTURECUBE)
    {
        if (pDesc->ArraySize < 6 || pDesc->ArraySize % 6 != 0 || pDesc->ArraySize > 2046)
        {
            DebugMsg("CreateTexture2D: cube map ArraySize ({}) must be a multiple of 6 in range [6, 2046]", pDesc->ArraySize);
            return E_INVALIDARG;
        }
    }

    if (pDesc->Format == DXGI_FORMAT_UNKNOWN)
    {
        DebugMsg("CreateTexture2D: Format is DXGI_FORMAT_UNKNOWN");
        return E_INVALIDARG;
    }

    HRESULT flagsHr = ValidateTextureBindFlags(pDesc->Format, pDesc->BindFlags, /*depthStencilAllowed=*/true);
    if (FAILED(flagsHr))
    {
        DebugMsg("CreateTexture2D: invalid bind flags for format");
        return flagsHr;
    }

    if (pDesc->Usage == D3D11_USAGE_IMMUTABLE && !pInitialData)
    {
        DebugMsg("CreateTexture2D: IMMUTABLE usage requires pInitialData");
        return E_INVALIDARG;
    }

    if (pDesc->Usage == D3D11_USAGE_DEFAULT && pDesc->CPUAccessFlags != 0)
    {
        DebugMsg("CreateTexture2D: DEFAULT usage cannot have CPUAccessFlags");
        return E_INVALIDARG;
    }

    if (pDesc->Usage == D3D11_USAGE_IMMUTABLE && pDesc->CPUAccessFlags != 0)
    {
        DebugMsg("CreateTexture2D: IMMUTABLE usage cannot have CPUAccessFlags");
        return E_INVALIDARG;
    }

    if (pDesc->Usage == D3D11_USAGE_DYNAMIC && pDesc->CPUAccessFlags != D3D11_CPU_ACCESS_WRITE)
    {
        DebugMsg("CreateTexture2D: DYNAMIC usage requires CPUAccessFlags = D3D11_CPU_ACCESS_WRITE");
        return E_INVALIDARG;
    }

    if (pDesc->Usage == D3D11_USAGE_STAGING && pDesc->BindFlags != 0)
    {
        DebugMsg("CreateTexture2D: STAGING usage cannot be bound to the pipeline (BindFlags must be 0)");
        return E_INVALIDARG;
    }

    if (pDesc->SampleDesc.Count > 1 && pInitialData)
    {
        DebugMsg("CreateTexture2D: multisampled resources cannot have pInitialData");
        return E_INVALIDARG;
    }

    D3D11_TEXTURE2D_DESC1 desc1 = {};
    std::memcpy(&desc1, pDesc, sizeof(D3D11_TEXTURE2D_DESC));
    desc1.TextureLayout  = D3D11_TEXTURE_LAYOUT_UNDEFINED;

    //#todo: force non-MS texture for now, implement later
    desc1.SampleDesc.Count   = 1;
    desc1.SampleDesc.Quality = 0;

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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateTexture3D(
    const D3D11_TEXTURE3D_DESC*   pDesc,
    const D3D11_SUBRESOURCE_DATA* pInitialData,
    ID3D11Texture3D**             ppTexture3D)
{
    if (!pDesc)
    {
        DebugMsg("CreateTexture3D: pDesc is null");
        return E_INVALIDARG;
    }

    if (pDesc->Width == 0 || pDesc->Width > D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION)
    {
        DebugMsg("CreateTexture3D: Width ({}) must be in range [1, {}]", pDesc->Width, D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
        return E_INVALIDARG;
    }

    if (pDesc->Height == 0 || pDesc->Height > D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION)
    {
        DebugMsg("CreateTexture3D: Height ({}) must be in range [1, {}]", pDesc->Height, D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
        return E_INVALIDARG;
    }

    if (pDesc->Depth == 0 || pDesc->Depth > D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION)
    {
        DebugMsg("CreateTexture3D: Depth ({}) must be in range [1, {}]", pDesc->Depth, D3D11_REQ_TEXTURE3D_U_V_OR_W_DIMENSION);
        return E_INVALIDARG;
    }

    if (pDesc->Format == DXGI_FORMAT_UNKNOWN)
    {
        DebugMsg("CreateTexture3D: Format is DXGI_FORMAT_UNKNOWN");
        return E_INVALIDARG;
    }

    HRESULT flagsHr = ValidateTextureBindFlags(pDesc->Format, pDesc->BindFlags, /*depthStencilAllowed=*/false);
    if (FAILED(flagsHr))
    {
        DebugMsg("CreateTexture3D: invalid bind flags for format");
        return flagsHr;
    }

    if (pDesc->Usage == D3D11_USAGE_IMMUTABLE && !pInitialData)
    {
        DebugMsg("CreateTexture3D: IMMUTABLE usage requires pInitialData");
        return E_INVALIDARG;
    }

    if (pDesc->Usage == D3D11_USAGE_DEFAULT && pDesc->CPUAccessFlags != 0)
    {
        DebugMsg("CreateTexture3D: DEFAULT usage cannot have CPUAccessFlags");
        return E_INVALIDARG;
    }

    if (pDesc->Usage == D3D11_USAGE_IMMUTABLE && pDesc->CPUAccessFlags != 0)
    {
        DebugMsg("CreateTexture3D: IMMUTABLE usage cannot have CPUAccessFlags");
        return E_INVALIDARG;
    }

    if (pDesc->Usage == D3D11_USAGE_DYNAMIC && pDesc->CPUAccessFlags != D3D11_CPU_ACCESS_WRITE)
    {
        DebugMsg("CreateTexture3D: DYNAMIC usage requires CPUAccessFlags = D3D11_CPU_ACCESS_WRITE");
        return E_INVALIDARG;
    }

    if (pDesc->Usage == D3D11_USAGE_STAGING && pDesc->BindFlags != 0)
    {
        DebugMsg("CreateTexture3D: STAGING usage cannot be bound to the pipeline (BindFlags must be 0)");
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateShaderResourceView(
    ID3D11Resource* pResource,
    const D3D11_SHADER_RESOURCE_VIEW_DESC* pDesc,
    ID3D11ShaderResourceView** ppSRView)
{
    if (!pResource)
    {
        DebugMsg("CreateShaderResourceView: pResource is null");
        return E_INVALIDARG;
    }

    if (!(GetSWResourceBindFlags(pResource) & D3D11_BIND_SHADER_RESOURCE))
    {
        DebugMsg("CreateShaderResourceView: resource was not created with D3D11_BIND_SHADER_RESOURCE");
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateUnorderedAccessView(
    ID3D11Resource* pResource,
    const D3D11_UNORDERED_ACCESS_VIEW_DESC* pDesc,
    ID3D11UnorderedAccessView** ppUAView)
{
    if (!pResource)
    {
        DebugMsg("CreateUnorderedAccessView: pResource is null");
        return E_INVALIDARG;
    }

    if (!(GetSWResourceBindFlags(pResource) & D3D11_BIND_UNORDERED_ACCESS))
    {
        DebugMsg("CreateUnorderedAccessView: resource was not created with D3D11_BIND_UNORDERED_ACCESS");
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateRenderTargetView(
    ID3D11Resource* pResource,
    const D3D11_RENDER_TARGET_VIEW_DESC* pDesc,
    ID3D11RenderTargetView** ppRTView)
{
    if (!pResource)
    {
        DebugMsg("CreateRenderTargetView: pResource is null");
        return E_INVALIDARG;
    }

    if (!(GetSWResourceBindFlags(pResource) & D3D11_BIND_RENDER_TARGET))
    {
        DebugMsg("CreateRenderTargetView: resource was not created with D3D11_BIND_RENDER_TARGET");
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateDepthStencilView(
    ID3D11Resource* pResource,
    const D3D11_DEPTH_STENCIL_VIEW_DESC* pDesc,
    ID3D11DepthStencilView** ppDepthStencilView)
{
    if (!pResource)
    {
        DebugMsg("CreateDepthStencilView: pResource is null");
        return E_INVALIDARG;
    }

    if (!(GetSWResourceBindFlags(pResource) & D3D11_BIND_DEPTH_STENCIL))
    {
        DebugMsg("CreateDepthStencilView: resource was not created with D3D11_BIND_DEPTH_STENCIL");
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateInputLayout(
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateVertexShader(
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateGeometryShader(
    const void* pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D11ClassLinkage* pClassLinkage,
    ID3D11GeometryShader** ppGeometryShader)
{
    D3D11GeometryShaderSW* shader = nullptr;
    HRESULT hr = CreateAndInit(&shader, pShaderBytecode, BytecodeLength);
    if (FAILED(hr))
    {
        return hr;
    }
    if (!ppGeometryShader)
    {
        shader->Release();
        return S_FALSE;
    }
    *ppGeometryShader = shader;
    return S_OK;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateGeometryShaderWithStreamOutput(
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
    D3D11GeometryShaderSW* shader = new(std::nothrow) D3D11GeometryShaderSW(this);
    if (!shader)
    {
        return E_OUTOFMEMORY;
    }
    
    HRESULT hr = shader->InitWithStreamOutput(
        pShaderBytecode, BytecodeLength,
        pSODeclaration, NumEntries,
        pBufferStrides, NumStrides,
        RasterizedStream);
    if (FAILED(hr))
    {
        shader->Release();
        return hr;
    }
    if (!ppGeometryShader)
    {
        shader->Release();
        return S_FALSE;
    }
    *ppGeometryShader = shader;
    return S_OK;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreatePixelShader(
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateHullShader(
    const void* pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D11ClassLinkage* pClassLinkage,
    ID3D11HullShader** ppHullShader)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateDomainShader(
    const void* pShaderBytecode,
    SIZE_T BytecodeLength,
    ID3D11ClassLinkage* pClassLinkage,
    ID3D11DomainShader** ppDomainShader)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateComputeShader(
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateClassLinkage(
    ID3D11ClassLinkage** ppLinkage)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateBlendState(
    const D3D11_BLEND_DESC* pBlendStateDesc,
    ID3D11BlendState** ppBlendState)
{
    if (!pBlendStateDesc)
    {
        DebugMsg("CreateBlendState: pBlendStateDesc is null");
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateDepthStencilState(
    const D3D11_DEPTH_STENCIL_DESC* pDepthStencilDesc,
    ID3D11DepthStencilState** ppDepthStencilState)
{
    if (!pDepthStencilDesc)
    {
        DebugMsg("CreateDepthStencilState: pDepthStencilDesc is null");
        return E_INVALIDARG;
    }

    if (pDepthStencilDesc->DepthWriteMask > D3D11_DEPTH_WRITE_MASK_ALL)
    {
        DebugMsg("CreateDepthStencilState: DepthWriteMask ({}) must be ZERO (0) or ALL (1)", (unsigned)pDepthStencilDesc->DepthWriteMask);
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateRasterizerState(
    const D3D11_RASTERIZER_DESC* pRasterizerDesc,
    ID3D11RasterizerState** ppRasterizerState)
{
    if (!pRasterizerDesc)
    {
        DebugMsg("CreateRasterizerState: pRasterizerDesc is null");
        return E_INVALIDARG;
    }

    if (pRasterizerDesc->FillMode != D3D11_FILL_WIREFRAME && pRasterizerDesc->FillMode != D3D11_FILL_SOLID)
    {
        DebugMsg("CreateRasterizerState: FillMode ({}) must be WIREFRAME (2) or SOLID (3)", (unsigned)pRasterizerDesc->FillMode);
        return E_INVALIDARG;
    }

    if (pRasterizerDesc->CullMode < D3D11_CULL_NONE || pRasterizerDesc->CullMode > D3D11_CULL_BACK)
    {
        DebugMsg("CreateRasterizerState: CullMode ({}) must be NONE (1), FRONT (2), or BACK (3)", (unsigned)pRasterizerDesc->CullMode);
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateSamplerState(
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateQuery(
    const D3D11_QUERY_DESC* pQueryDesc,
    ID3D11Query** ppQuery)
{
    if (!pQueryDesc)
    {
        return E_INVALIDARG;
    }

    D3D11_QUERY_DESC1 desc1{};
    desc1.Query     = pQueryDesc->Query;
    desc1.MiscFlags = pQueryDesc->MiscFlags;

    D3D11QuerySW* query = nullptr;
    HRESULT hr = CreateAndInit(&query, &desc1);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppQuery)
    {
        query->Release();
        return S_FALSE;
    }

    *ppQuery = query;
    return S_OK;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreatePredicate(
    const D3D11_QUERY_DESC* pPredicateDesc,
    ID3D11Predicate** ppPredicate)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateCounter(
    const D3D11_COUNTER_DESC* pCounterDesc,
    ID3D11Counter** ppCounter)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateDeferredContext(
    UINT ContextFlags,
    ID3D11DeviceContext** ppDeferredContext)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::OpenSharedResource(
    HANDLE hResource,
    REFIID ReturnedInterface,
    void** ppResource)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CheckFormatSupport(
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CheckMultisampleQualityLevels(
    DXGI_FORMAT Format,
    UINT SampleCount,
    UINT* pNumQualityLevels)
{
    if (!pNumQualityLevels)
    {
        return E_INVALIDARG;
    }
    *pNumQualityLevels = (SampleCount >= 1 && SampleCount <= 16 && (SampleCount & (SampleCount - 1)) == 0) ? 1 : 0;
    return S_OK;
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CheckCounterInfo(
    D3D11_COUNTER_INFO* pCounterInfo)
{
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CheckCounter(
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CheckFeatureSupport(
    D3D11_FEATURE Feature,
    void* pFeatureSupportData,
    UINT FeatureSupportDataSize)
{
    if (!pFeatureSupportData)
    {
        return E_INVALIDARG;
    }

    switch (Feature)
    {
    case D3D11_FEATURE_THREADING:
    {
        if (FeatureSupportDataSize != sizeof(D3D11_FEATURE_DATA_THREADING))
        {
            return E_INVALIDARG;
        }
        D3D11_FEATURE_DATA_THREADING* data = static_cast<D3D11_FEATURE_DATA_THREADING*>(pFeatureSupportData);
        data->DriverConcurrentCreates = FALSE;
        data->DriverCommandLists      = FALSE;
        return S_OK;
    }
    case D3D11_FEATURE_DOUBLES:
    {
        if (FeatureSupportDataSize != sizeof(D3D11_FEATURE_DATA_DOUBLES))
        {
            return E_INVALIDARG;
        }
        D3D11_FEATURE_DATA_DOUBLES* data = static_cast<D3D11_FEATURE_DATA_DOUBLES*>(pFeatureSupportData);
        data->DoublePrecisionFloatShaderOps = FALSE;
        return S_OK;
    }
    case D3D11_FEATURE_FORMAT_SUPPORT:
    {
        if (FeatureSupportDataSize != sizeof(D3D11_FEATURE_DATA_FORMAT_SUPPORT))
        {
            return E_INVALIDARG;
        }
        D3D11_FEATURE_DATA_FORMAT_SUPPORT* data = static_cast<D3D11_FEATURE_DATA_FORMAT_SUPPORT*>(pFeatureSupportData);
        data->OutFormatSupport = GetFormatSupport(data->InFormat);
        return S_OK;
    }
    case D3D11_FEATURE_FORMAT_SUPPORT2:
    {
        if (FeatureSupportDataSize != sizeof(D3D11_FEATURE_DATA_FORMAT_SUPPORT2))
        {
            return E_INVALIDARG;
        }
        D3D11_FEATURE_DATA_FORMAT_SUPPORT2* data = static_cast<D3D11_FEATURE_DATA_FORMAT_SUPPORT2*>(pFeatureSupportData);
        data->OutFormatSupport2 = 0;
        return S_OK;
    }
    case D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS:
    {
        if (FeatureSupportDataSize != sizeof(D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS))
        {
            return E_INVALIDARG;
        }
        D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS* data = static_cast<D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS*>(pFeatureSupportData);
        data->ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x = TRUE;
        return S_OK;
    }
    case D3D11_FEATURE_D3D11_OPTIONS:
    {
        if (FeatureSupportDataSize != sizeof(D3D11_FEATURE_DATA_D3D11_OPTIONS))
        {
            return E_INVALIDARG;
        }
        D3D11_FEATURE_DATA_D3D11_OPTIONS* data = static_cast<D3D11_FEATURE_DATA_D3D11_OPTIONS*>(pFeatureSupportData);
        memset(data, 0, sizeof(*data));
        return S_OK;
    }
    case D3D11_FEATURE_ARCHITECTURE_INFO:
    {
        if (FeatureSupportDataSize != sizeof(D3D11_FEATURE_DATA_ARCHITECTURE_INFO))
        {
            return E_INVALIDARG;
        }
        D3D11_FEATURE_DATA_ARCHITECTURE_INFO* data = static_cast<D3D11_FEATURE_DATA_ARCHITECTURE_INFO*>(pFeatureSupportData);
        data->TileBasedDeferredRenderer = FALSE;
        return S_OK;
    }
    case D3D11_FEATURE_SHADER_MIN_PRECISION_SUPPORT:
    {
        if (FeatureSupportDataSize != sizeof(D3D11_FEATURE_DATA_SHADER_MIN_PRECISION_SUPPORT))
        {
            return E_INVALIDARG;
        }
        D3D11_FEATURE_DATA_SHADER_MIN_PRECISION_SUPPORT* data = static_cast<D3D11_FEATURE_DATA_SHADER_MIN_PRECISION_SUPPORT*>(pFeatureSupportData);
        data->PixelShaderMinPrecision              = 0;
        data->AllOtherShaderStagesMinPrecision     = 0;
        return S_OK;
    }
    case D3D11_FEATURE_D3D9_OPTIONS:
    {
        if (FeatureSupportDataSize != sizeof(D3D11_FEATURE_DATA_D3D9_OPTIONS))
        {
            return E_INVALIDARG;
        }
        D3D11_FEATURE_DATA_D3D9_OPTIONS* data = static_cast<D3D11_FEATURE_DATA_D3D9_OPTIONS*>(pFeatureSupportData);
        data->FullNonPow2TextureSupport = TRUE;
        return S_OK;
    }
    case D3D11_FEATURE_D3D9_SHADOW_SUPPORT:
    {
        if (FeatureSupportDataSize != sizeof(D3D11_FEATURE_DATA_D3D9_SHADOW_SUPPORT))
        {
            return E_INVALIDARG;
        }
        D3D11_FEATURE_DATA_D3D9_SHADOW_SUPPORT* data = static_cast<D3D11_FEATURE_DATA_D3D9_SHADOW_SUPPORT*>(pFeatureSupportData);
        data->SupportsDepthAsTextureWithLessEqualComparisonFilter = TRUE;
        return S_OK;
    }
    case D3D11_FEATURE_D3D11_OPTIONS1:
    {
        if (FeatureSupportDataSize != sizeof(D3D11_FEATURE_DATA_D3D11_OPTIONS1))
        {
            return E_INVALIDARG;
        }
        D3D11_FEATURE_DATA_D3D11_OPTIONS1* data = static_cast<D3D11_FEATURE_DATA_D3D11_OPTIONS1*>(pFeatureSupportData);
        memset(data, 0, sizeof(*data));
        return S_OK;
    }
    case D3D11_FEATURE_D3D9_SIMPLE_INSTANCING_SUPPORT:
    {
        if (FeatureSupportDataSize != sizeof(D3D11_FEATURE_DATA_D3D9_SIMPLE_INSTANCING_SUPPORT))
        {
            return E_INVALIDARG;
        }
        D3D11_FEATURE_DATA_D3D9_SIMPLE_INSTANCING_SUPPORT* data = static_cast<D3D11_FEATURE_DATA_D3D9_SIMPLE_INSTANCING_SUPPORT*>(pFeatureSupportData);
        data->SimpleInstancingSupported = TRUE;
        return S_OK;
    }
    case D3D11_FEATURE_MARKER_SUPPORT:
    {
        if (FeatureSupportDataSize != sizeof(D3D11_FEATURE_DATA_MARKER_SUPPORT))
        {
            return E_INVALIDARG;
        }
        D3D11_FEATURE_DATA_MARKER_SUPPORT* data = static_cast<D3D11_FEATURE_DATA_MARKER_SUPPORT*>(pFeatureSupportData);
        data->Profile = FALSE;
        return S_OK;
    }
    case D3D11_FEATURE_D3D9_OPTIONS1:
    {
        if (FeatureSupportDataSize != sizeof(D3D11_FEATURE_DATA_D3D9_OPTIONS1))
        {
            return E_INVALIDARG;
        }
        D3D11_FEATURE_DATA_D3D9_OPTIONS1* data = static_cast<D3D11_FEATURE_DATA_D3D9_OPTIONS1*>(pFeatureSupportData);
        data->FullNonPow2TextureSupported = TRUE;
        data->DepthAsTextureWithLessEqualComparisonFilterSupported = TRUE;
        data->SimpleInstancingSupported = TRUE;
        data->TextureCubeFaceRenderTargetWithNonCubeDepthStencilSupported = TRUE;
        return S_OK;
    }
    case D3D11_FEATURE_D3D11_OPTIONS2:
    {
        if (FeatureSupportDataSize != sizeof(D3D11_FEATURE_DATA_D3D11_OPTIONS2))
        {
            return E_INVALIDARG;
        }
        D3D11_FEATURE_DATA_D3D11_OPTIONS2* data = static_cast<D3D11_FEATURE_DATA_D3D11_OPTIONS2*>(pFeatureSupportData);
        memset(data, 0, sizeof(*data));
        return S_OK;
    }
    case D3D11_FEATURE_D3D11_OPTIONS3:
    {
        if (FeatureSupportDataSize != sizeof(D3D11_FEATURE_DATA_D3D11_OPTIONS3))
        {
            return E_INVALIDARG;
        }
        D3D11_FEATURE_DATA_D3D11_OPTIONS3* data = static_cast<D3D11_FEATURE_DATA_D3D11_OPTIONS3*>(pFeatureSupportData);
        data->VPAndRTArrayIndexFromAnyShaderFeedingRasterizer = FALSE;
        return S_OK;
    }
    default:
        return E_INVALIDARG;
    }
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::GetPrivateData(
    REFGUID guid,
    UINT* pDataSize,
    void* pData)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::SetPrivateData(
    REFGUID guid,
    UINT DataSize,
    const void* pData)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::SetPrivateDataInterface(
    REFGUID guid,
    const IUnknown* pData)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
D3D_FEATURE_LEVEL STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::GetFeatureLevel()
{
    return D3D_FEATURE_LEVEL_11_1;
}

template<Bool IsDebug>
UINT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::GetCreationFlags()
{
    return 0;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::GetDeviceRemovedReason()
{
    return S_OK;
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::GetImmediateContext(
    ID3D11DeviceContext** ppImmediateContext)
{
    if (ppImmediateContext && _immediateContext)
    {
        *ppImmediateContext = static_cast<ID3D11DeviceContext*>(_immediateContext);
        _immediateContext->AddRef();
    }
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::SetExceptionMode(
    UINT RaiseFlags)
{
    return S_OK;
}

template<Bool IsDebug>
UINT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::GetExceptionMode()
{
    return 0;
}

// ---- ID3D11Device1 ----

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::GetImmediateContext1(ID3D11DeviceContext1** ppImmediateContext)
{
    if (ppImmediateContext && _immediateContext)
    {
        *ppImmediateContext = static_cast<ID3D11DeviceContext1*>(_immediateContext);
        _immediateContext->AddRef();
    }
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateDeferredContext1(UINT ContextFlags, ID3D11DeviceContext1** ppDeferredContext)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateBlendState1(const D3D11_BLEND_DESC1* pBlendStateDesc, ID3D11BlendState1** ppBlendState)
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateRasterizerState1(const D3D11_RASTERIZER_DESC1* pRasterizerDesc, ID3D11RasterizerState1** ppRasterizerState)
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateDeviceContextState(UINT Flags, const D3D_FEATURE_LEVEL* pFeatureLevels, UINT FeatureLevels, UINT SDKVersion, REFIID EmulatedInterface, D3D_FEATURE_LEVEL* pChosenFeatureLevel, ID3DDeviceContextState** ppContextState)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::OpenSharedResource1(HANDLE hResource, REFIID returnedInterface, void** ppResource)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::OpenSharedResourceByName(LPCWSTR lpName, DWORD dwDesiredAccess, REFIID returnedInterface, void** ppResource)
{
    return E_NOTIMPL;
}

// ---- ID3D11Device2 ----

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::GetImmediateContext2(ID3D11DeviceContext2** ppImmediateContext)
{
    if (ppImmediateContext && _immediateContext)
    {
        *ppImmediateContext = static_cast<ID3D11DeviceContext2*>(_immediateContext);
        _immediateContext->AddRef();
    }
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateDeferredContext2(UINT ContextFlags, ID3D11DeviceContext2** ppDeferredContext)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::GetResourceTiling(ID3D11Resource* pTiledResource, UINT* pNumTilesForEntireResource, D3D11_PACKED_MIP_DESC* pPackedMipDesc, D3D11_TILE_SHAPE* pStandardTileShapeForNonPackedMips, UINT* pNumSubresourceTilings, UINT FirstSubresourceTilingToGet, D3D11_SUBRESOURCE_TILING* pSubresourceTilingsForNonPackedMips)
{
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CheckMultisampleQualityLevels1(DXGI_FORMAT Format, UINT SampleCount, UINT Flags, UINT* pNumQualityLevels)
{
    return CheckMultisampleQualityLevels(Format, SampleCount, pNumQualityLevels);
}

// ---- ID3D11Device3 ----

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateTexture2D1(const D3D11_TEXTURE2D_DESC1* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture2D1** ppTexture2D)
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

    D3D11_TEXTURE2D_DESC1 desc = *pDesc;
    desc.SampleDesc.Count   = 1;
    desc.SampleDesc.Quality = 0;

    D3D11Texture2DSW* tex = nullptr;
    HRESULT hr = CreateAndInit(&tex, &desc, pInitialData);
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateTexture3D1(const D3D11_TEXTURE3D_DESC1* pDesc, const D3D11_SUBRESOURCE_DATA* pInitialData, ID3D11Texture3D1** ppTexture3D)
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateRasterizerState2(const D3D11_RASTERIZER_DESC2* pRasterizerDesc, ID3D11RasterizerState2** ppRasterizerState)
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateShaderResourceView1(ID3D11Resource* pResource, const D3D11_SHADER_RESOURCE_VIEW_DESC1* pDesc, ID3D11ShaderResourceView1** ppSRView)
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateUnorderedAccessView1(ID3D11Resource* pResource, const D3D11_UNORDERED_ACCESS_VIEW_DESC1* pDesc, ID3D11UnorderedAccessView1** ppUAView)
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateRenderTargetView1(ID3D11Resource* pResource, const D3D11_RENDER_TARGET_VIEW_DESC1* pDesc, ID3D11RenderTargetView1** ppRTView)
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

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateQuery1(const D3D11_QUERY_DESC1* pQueryDesc, ID3D11Query1** ppQuery)
{
    if (!pQueryDesc)
    {
        return E_INVALIDARG;
    }

    D3D11QuerySW* query = nullptr;
    HRESULT hr = CreateAndInit(&query, pQueryDesc);
    if (FAILED(hr))
    {
        return hr;
    }

    if (!ppQuery)
    {
        query->Release();
        return S_FALSE;
    }

    *ppQuery = query;
    return S_OK;
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::GetImmediateContext3(ID3D11DeviceContext3** ppImmediateContext)
{
    if (ppImmediateContext && _immediateContext)
    {
        *ppImmediateContext = static_cast<ID3D11DeviceContext3*>(_immediateContext);
        _immediateContext->AddRef();
    }
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateDeferredContext3(UINT ContextFlags, ID3D11DeviceContext3** ppDeferredContext)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::WriteToSubresource(ID3D11Resource* pDstResource, UINT DstSubresource, const D3D11_BOX* pDstBox, const void* pSrcData, UINT SrcRowPitch, UINT SrcDepthPitch)
{
    if (!pDstResource || !pSrcData)
    {
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
void STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::ReadFromSubresource(void* pDstData, UINT DstRowPitch, UINT DstDepthPitch, ID3D11Resource* pSrcResource, UINT SrcSubresource, const D3D11_BOX* pSrcBox)
{
    if (!pDstData || !pSrcResource)
    {
        return;
    }

    RunOnSWResource(pSrcResource, [&](auto* src)
    {
        auto         layout  = src->GetSubresourceLayout(SrcSubresource);
        const Uint8* srcBase = static_cast<const Uint8*>(src->GetDataPtr()) + layout.Offset;
        Uint8*       dst     = static_cast<Uint8*>(pDstData);
        CopySubresourceData(dst, DstRowPitch, DstDepthPitch,
                            srcBase, layout.RowPitch, layout.DepthPitch,
                            layout, pSrcBox);
    });
}

// ---- ID3D11Device4 ----

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::RegisterDeviceRemovedEvent(HANDLE hEvent, DWORD* pdwCookie)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
void STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::UnregisterDeviceRemoved(DWORD dwCookie)
{
}

// ---- ID3D11Device5 ----

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::OpenSharedFence(HANDLE hFence, REFIID ReturnedInterface, void** ppFence)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
HRESULT STDMETHODCALLTYPE D3D11DeviceSWImpl<IsDebug>::CreateFence(UINT64 InitialValue, D3D11_FENCE_FLAG Flags, REFIID ReturnedInterface, void** ppFence)
{
    return E_NOTIMPL;
}

template<Bool IsDebug>
template<typename... ArgsT>
void D3D11DeviceSWImpl<IsDebug>::DebugMsg(const Char* fmt, ArgsT&&... args) const
{
    if constexpr (IsDebug) 
    { 
        D3D11SW_ERROR(fmt, std::forward<ArgsT>(args)...); 
    }
}

template class D3D11DeviceSWImpl<false>;
template class D3D11DeviceSWImpl<true>;

}
