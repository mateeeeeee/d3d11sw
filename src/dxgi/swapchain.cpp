#include "dxgi/swapchain.h"

MarsDXGISwapChain::MarsDXGISwapChain(ID3D11Device* device, const DXGI_SWAP_CHAIN_DESC& desc)
    : m_device(device), m_desc(desc)
{
    if (m_device) m_device->AddRef();
}

MarsDXGISwapChain::~MarsDXGISwapChain()
{
    if (m_device) m_device->Release();
}


HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::SetPrivateData(REFGUID Name, UINT DataSize, const void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::GetParent(REFIID riid, void** ppParent)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::GetDevice(REFIID riid, void** ppDevice)
{
    return m_device->QueryInterface(riid, ppDevice);
}


HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::Present(UINT SyncInterval, UINT Flags)
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::GetBuffer(UINT Buffer, REFIID riid, void** ppSurface)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::SetFullscreenState(BOOL Fullscreen, IDXGIOutput* pTarget)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::GetFullscreenState(BOOL* pFullscreen, IDXGIOutput** ppTarget)
{
    if (pFullscreen)
    {
        *pFullscreen = FALSE;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::GetDesc(DXGI_SWAP_CHAIN_DESC* pDesc)
{
    if (!pDesc)
    {
        return E_INVALIDARG;
    }
    *pDesc = m_desc;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::ResizeBuffers(UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::ResizeTarget(const DXGI_MODE_DESC* pNewTargetParameters)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::GetContainingOutput(IDXGIOutput** ppOutput)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::GetFrameStatistics(DXGI_FRAME_STATISTICS* pStats)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::GetLastPresentCount(UINT* pLastPresentCount)
{
    if (pLastPresentCount)
    {
        *pLastPresentCount = 0;
    }
    return S_OK;
}


HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::GetDesc1(DXGI_SWAP_CHAIN_DESC1* pDesc)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::GetFullscreenDesc(DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pDesc)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::GetHwnd(HWND* pHwnd)
{
    if (pHwnd)
    {
        *pHwnd = m_desc.OutputWindow;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::GetCoreWindow(REFIID refiid, void** ppUnk)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::Present1(UINT SyncInterval, UINT PresentFlags, const DXGI_PRESENT_PARAMETERS* pPresentParameters)
{
    return S_OK;
}

BOOL STDMETHODCALLTYPE MarsDXGISwapChain::IsTemporaryMonoSupported()
{
    return FALSE;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::GetRestrictToOutput(IDXGIOutput** ppRestrictToOutput)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::SetBackgroundColor(const DXGI_RGBA* pColor)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::GetBackgroundColor(DXGI_RGBA* pColor)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::SetRotation(DXGI_MODE_ROTATION Rotation)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGISwapChain::GetRotation(DXGI_MODE_ROTATION* pRotation)
{
    return E_NOTIMPL;
}
