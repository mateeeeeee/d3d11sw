#include "dxgi/factory.h"

MarsDXGIFactory::MarsDXGIFactory() {}
MarsDXGIFactory::~MarsDXGIFactory() {}


HRESULT STDMETHODCALLTYPE MarsDXGIFactory::SetPrivateData(REFGUID Name, UINT DataSize, const void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIFactory::SetPrivateDataInterface(REFGUID Name, const IUnknown* pUnknown)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIFactory::GetPrivateData(REFGUID Name, UINT* pDataSize, void* pData)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIFactory::GetParent(REFIID riid, void** ppParent)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE MarsDXGIFactory::EnumAdapters(UINT Adapter, IDXGIAdapter** ppAdapter)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIFactory::MakeWindowAssociation(HWND WindowHandle, UINT Flags)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIFactory::GetWindowAssociation(HWND* pWindowHandle)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIFactory::CreateSwapChain(IUnknown* pDevice, DXGI_SWAP_CHAIN_DESC* pDesc, IDXGISwapChain** ppSwapChain)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIFactory::CreateSoftwareAdapter(HMODULE Module, IDXGIAdapter** ppAdapter)
{
    return E_NOTIMPL;
}


HRESULT STDMETHODCALLTYPE MarsDXGIFactory::EnumAdapters1(UINT Adapter, IDXGIAdapter1** ppAdapter)
{
    return E_NOTIMPL;
}

BOOL STDMETHODCALLTYPE MarsDXGIFactory::IsCurrent()
{
    return TRUE;
}


BOOL STDMETHODCALLTYPE MarsDXGIFactory::IsWindowedStereoEnabled()
{
    return FALSE;
}

HRESULT STDMETHODCALLTYPE MarsDXGIFactory::CreateSwapChainForHwnd(IUnknown* pDevice, HWND hWnd, const DXGI_SWAP_CHAIN_DESC1* pDesc, const DXGI_SWAP_CHAIN_FULLSCREEN_DESC* pFullscreenDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIFactory::CreateSwapChainForCoreWindow(IUnknown* pDevice, IUnknown* pWindow, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIFactory::GetSharedResourceAdapterLuid(HANDLE hResource, LUID* pLuid)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIFactory::RegisterStereoStatusWindow(HWND WindowHandle, UINT wMsg, DWORD* pdwCookie)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIFactory::RegisterStereoStatusEvent(HANDLE hEvent, DWORD* pdwCookie)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE MarsDXGIFactory::UnregisterStereoStatus(DWORD dwCookie)
{
}

HRESULT STDMETHODCALLTYPE MarsDXGIFactory::RegisterOcclusionStatusWindow(HWND WindowHandle, UINT wMsg, DWORD* pdwCookie)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MarsDXGIFactory::RegisterOcclusionStatusEvent(HANDLE hEvent, DWORD* pdwCookie)
{
    return E_NOTIMPL;
}

void STDMETHODCALLTYPE MarsDXGIFactory::UnregisterOcclusionStatus(DWORD dwCookie)
{
}

HRESULT STDMETHODCALLTYPE MarsDXGIFactory::CreateSwapChainForComposition(IUnknown* pDevice, const DXGI_SWAP_CHAIN_DESC1* pDesc, IDXGIOutput* pRestrictToOutput, IDXGISwapChain1** ppSwapChain)
{
    return E_NOTIMPL;
}
