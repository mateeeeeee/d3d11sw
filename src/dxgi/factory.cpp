#include "dxgi/factory.h"
#include "dxgi/adapter.h"
#include "dxgi/swapchain.h"

namespace d3d11sw {


DXGIFactorySW::DXGIFactorySW() {}
DXGIFactorySW::~DXGIFactorySW() {}


HRESULT STDMETHODCALLTYPE DXGIFactorySW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
    {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == __uuidof(IUnknown) || riid == __uuidof(IDXGIFactory2))
    {
        *ppv = static_cast<IDXGIFactory2*>(this);
    }
    else if (riid == __uuidof(IDXGIFactory1))
    {
        *ppv = static_cast<IDXGIFactory1*>(this);
    }
    else if (riid == __uuidof(IDXGIFactory))
    {
        *ppv = static_cast<IDXGIFactory*>(this);
    }
    else if (riid == __uuidof(IDXGIObject))
    {
        *ppv = static_cast<IDXGIObject*>(this);
    }
    else
    {
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}


HRESULT STDMETHODCALLTYPE DXGIFactorySW::SetPrivateData(REFGUID Name, UINT DataSize, const void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::GetParent(REFIID riid, void** ppParent)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE DXGIFactorySW::EnumAdapters(UINT Adapter, IDXGIAdapter** ppAdapter)
{
    if (!ppAdapter)
    {
        return DXGI_ERROR_INVALID_CALL;
    }
    if (Adapter > 0)
    {
        return DXGI_ERROR_NOT_FOUND;
    }
    *ppAdapter = new DXGIAdapterSW();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::MakeWindowAssociation(HWND WindowHandle, UINT Flags)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::GetWindowAssociation(HWND* pWindowHandle)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::CreateSwapChain(IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
{
    if (!pDevice || !pDesc || !ppSwapChain)
    {
        return DXGI_ERROR_INVALID_CALL;
    }

    ID3D11Device* d3dDevice = nullptr;
    HRESULT hr = pDevice->QueryInterface(__uuidof(ID3D11Device), (void**)&d3dDevice);
    if (FAILED(hr))
    {
        return hr;
    }

    DXGISwapChainSW* sc = new DXGISwapChainSW(d3dDevice, *pDesc);
    *ppSwapChain = sc;

    d3dDevice->Release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::CreateSoftwareAdapter(HMODULE Module, IDXGIAdapter** ppAdapter)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE DXGIFactorySW::EnumAdapters1(UINT Adapter, IDXGIAdapter1** ppAdapter)
{
    if (!ppAdapter)
    {
        return DXGI_ERROR_INVALID_CALL;
    }
    if (Adapter > 0)
    {
        return DXGI_ERROR_NOT_FOUND;
    }
    *ppAdapter = new DXGIAdapterSW();
    return S_OK;
}

BOOL STDMETHODCALLTYPE DXGIFactorySW::IsCurrent()
{
    return TRUE;
}


BOOL STDMETHODCALLTYPE DXGIFactorySW::IsWindowedStereoEnabled()
{
    return FALSE;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::CreateSwapChainForHwnd(IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain)
{
    if (!pDevice || !pDesc || !ppSwapChain)
    {
        return DXGI_ERROR_INVALID_CALL;
    }

    ID3D11Device* d3dDevice = nullptr;
    HRESULT hr = pDevice->QueryInterface(__uuidof(ID3D11Device), (void**)&d3dDevice);
    if (FAILED(hr))
    {
        return hr;
    }

    DXGI_SWAP_CHAIN_DESC legacyDesc{};
    legacyDesc.BufferDesc.Width   = pDesc->Width;
    legacyDesc.BufferDesc.Height  = pDesc->Height;
    legacyDesc.BufferDesc.Format  = pDesc->Format;
    legacyDesc.SampleDesc         = pDesc->SampleDesc;
    legacyDesc.BufferUsage        = pDesc->BufferUsage;
    legacyDesc.BufferCount        = pDesc->BufferCount;
    legacyDesc.OutputWindow       = hWnd;
    legacyDesc.Windowed           = TRUE;
    legacyDesc.SwapEffect         = pDesc->SwapEffect;
    legacyDesc.Flags              = pDesc->Flags;

    DXGISwapChainSW* sc = new DXGISwapChainSW(d3dDevice, legacyDesc);
    *ppSwapChain = sc;

    d3dDevice->Release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::CreateSwapChainForCoreWindow(IUnknown* pDevice, IUnknown* pWindow, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::GetSharedResourceAdapterLuid(HANDLE hResource, LUID* pLuid)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::RegisterStereoStatusWindow(HWND WindowHandle, UINT wMsg, DWORD* pdwCookie)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::RegisterStereoStatusEvent(HANDLE hEvent, DWORD* pdwCookie)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE DXGIFactorySW::UnregisterStereoStatus(DWORD dwCookie)
{
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::RegisterOcclusionStatusWindow(HWND WindowHandle, UINT wMsg, DWORD* pdwCookie)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::RegisterOcclusionStatusEvent(HANDLE hEvent, DWORD* pdwCookie)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE DXGIFactorySW::UnregisterOcclusionStatus(DWORD dwCookie)
{
}

HRESULT STDMETHODCALLTYPE DXGIFactorySW::CreateSwapChainForComposition(IUnknown* pDevice, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain)
{
    return E_NOTIMPL;
}

}
