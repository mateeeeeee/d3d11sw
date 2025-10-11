#include "dxgi/swapchain.h"

namespace d3d11sw {


DXGISwapChainSW::DXGISwapChainSW(ID3D11Device* device, const DXGI_SWAP_CHAIN_DESC& desc)
    : _device(device), _desc(desc)
{
    if (_device) _device->AddRef();
}

DXGISwapChainSW::~DXGISwapChainSW()
{
    if (_device) _device->Release();
}


HRESULT STDMETHODCALLTYPE DXGISwapChainSW::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = nullptr;

    if (riid == __uuidof(IUnknown) || riid == __uuidof(IDXGISwapChain1))
        *ppv = static_cast<IDXGISwapChain1*>(this);
    else if (riid == __uuidof(IDXGISwapChain))
        *ppv = static_cast<IDXGISwapChain*>(this);
    else if (riid == __uuidof(IDXGIDeviceSubObject))
        *ppv = static_cast<IDXGIDeviceSubObject*>(this);
    else if (riid == __uuidof(IDXGIObject))
        *ppv = static_cast<IDXGIObject*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}


HRESULT STDMETHODCALLTYPE DXGISwapChainSW::SetPrivateData(REFGUID Name, UINT DataSize, const void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetParent(REFIID riid, void** ppParent)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetDevice(REFIID riid, void** ppDevice)
{
    return _device->QueryInterface(riid, ppDevice);
}


HRESULT STDMETHODCALLTYPE DXGISwapChainSW::Present(UINT SyncInterval, UINT Flags)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetBuffer(UINT Buffer, REFIID riid, void** ppSurface)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::SetFullscreenState(BOOL Fullscreen, IDXGIOutput* pTarget)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetFullscreenState(BOOL* pFullscreen, IDXGIOutput** ppTarget)
{
    if (pFullscreen)
    {
        *pFullscreen = FALSE;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetDesc(DXGI_SWAP_CHAIN_DESC* pDesc)
{
    if (!pDesc)
    {
        return E_INVALIDARG;
    }
    *pDesc = _desc;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetContainingOutput(IDXGIOutput** ppOutput)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetFrameStatistics(DXGI_FRAME_STATISTICS* pStats)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetLastPresentCount(UINT* pLastPresentCount)
{
    if (pLastPresentCount)
    {
        *pLastPresentCount = 0;
    }
    return S_OK;
}


HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetDesc1(DXGI_SWAP_CHAIN_DESC1* pDesc)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pDesc)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetHwnd(HWND* pHwnd)
{
    if (pHwnd)
    {
        *pHwnd = _desc.OutputWindow;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetCoreWindow(REFIID refiid, void** ppUnk)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::Present1(UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters)
{
    return S_OK;
}

BOOL STDMETHODCALLTYPE DXGISwapChainSW::IsTemporaryMonoSupported()
{
    return FALSE;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetRestrictToOutput(IDXGIOutput** ppRestrictToOutput)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::SetBackgroundColor(const DXGI_RGBA* pColor)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetBackgroundColor(DXGI_RGBA* pColor)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::SetRotation(DXGI_MODE_ROTATION Rotation)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE DXGISwapChainSW::GetRotation(DXGI_MODE_ROTATION* pRotation)
{
    return E_NOTIMPL;
}

}
